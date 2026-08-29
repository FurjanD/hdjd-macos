#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "coremidi.h"
#include "usb.h"
#include "log.h"

static MIDIClientRef midi_client = 0;
static MIDIEndpointRef midi_source = 0;
static MIDIEndpointRef midi_destination = 0;

static void
midi_read_proc(const MIDIPacketList *packet_list,
               void *read_proc_ref_con,
               void *src_conn_ref_con)
{
    const MIDIPacket *packet = &packet_list->packet[0];

    for (UInt32 i = 0; i < packet_list->numPackets; i++) {

        if (packet->length > 0) {
            printf("CoreMIDI -> USB:");
            for (UInt16 j = 0; j < packet->length; j++) {
                printf(" %02x", packet->data[j]);
            }
            printf("\n");

            usb_write(packet->data, packet->length);
        }

        packet = MIDIPacketNext(packet);
    }
}

int
coremidi_setup(const char *name)
{
    OSStatus status;

    CFStringRef cf_name =
        CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);

    if (!cf_name) {
        warn("Could not create CoreFoundation string");
        return -1;
    }

    status = MIDIClientCreate(
        cf_name,
        NULL,
        NULL,
        &midi_client
    );

    if (status != noErr) {
        warn("MIDIClientCreate failed: %d", (int)status);
        CFRelease(cf_name);
        return -1;
    }

    status = MIDISourceCreate(
        midi_client,
        cf_name,
        &midi_source
    );

    if (status != noErr) {
        warn("MIDISourceCreate failed: %d", (int)status);
        MIDIClientDispose(midi_client);
        midi_client = 0;
        CFRelease(cf_name);
        return -1;
    }

    status = MIDIDestinationCreate(
        midi_client,
        cf_name,
        midi_read_proc,
        NULL,
        &midi_destination
    );

    if (status != noErr) {
        warn("MIDIDestinationCreate failed: %d", (int)status);
        MIDIEndpointDispose(midi_source);
        midi_source = 0;
        MIDIClientDispose(midi_client);
        midi_client = 0;
        CFRelease(cf_name);
        return -1;
    }

    printf("Created CoreMIDI source [%s]\n", name);
    printf("Created CoreMIDI destination [%s]\n", name);

    CFRelease(cf_name);

    return 0;
}

void
coremidi_close(void)
{
    if (midi_source) {
        MIDIEndpointDispose(midi_source);
        midi_source = 0;
    }

    if (midi_destination) {
        MIDIEndpointDispose(midi_destination);
        midi_destination = 0;
    }

    if (midi_client) {
        MIDIClientDispose(midi_client);
        midi_client = 0;
    }
}

void
coremidi_write(uint8_t *data, size_t datalen)
{
    if (!midi_source || datalen == 0) {
        return;
    }

    Byte buffer[1024];

    if (datalen > sizeof(buffer) - sizeof(MIDIPacketList)) {
        warn("MIDI packet too large: %ld bytes", (long)datalen);
        return;
    }

    MIDIPacketList *packet_list =
        (MIDIPacketList *)buffer;

    MIDIPacket *packet =
        MIDIPacketListInit(packet_list);

    packet = MIDIPacketListAdd(
        packet_list,
        sizeof(buffer),
        packet,
        0,
        datalen,
        data
    );

    if (!packet) {
        warn("MIDIPacketListAdd failed");
        return;
    }

    OSStatus status =
        MIDIReceived(midi_source, packet_list);

    if (status != noErr) {
        warn("MIDIReceived failed: %d", (int)status);
    }
}