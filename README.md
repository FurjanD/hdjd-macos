# hdjd-mac

A macOS port of [hdjd](https://github.com/nealey/hdjd), a userspace driver for older Hercules DJ controllers.

> **Status:** Working on modern macOS with the Hercules DJ Console Mk4 and Mixxx, including controller input, jog wheels, MIDI output, and controller LEDs.

This project allows unsupported Hercules DJ controllers to be used on modern versions of macOS by communicating with the controller directly over USB and exposing its MIDI data through Apple's CoreMIDI framework.

The primary goal is to make older Hercules hardware usable with modern DJ software such as Mixxx without requiring a Windows installation or virtual machine.

## Supported Hardware

Currently tested:

* Hercules DJ Console Mk4

The original `hdjd` project supports several Hercules controllers. The USB device table from the original project is retained, but not every controller has been tested on macOS.

The Hercules DJ Console Mk4 identifies as:

```text
Vendor:Device = 06f8:b107
```

## How It Works

The DJ Console Mk4 does not present itself to macOS as a standard USB MIDI device.

This project communicates with the controller directly using `libusb`:

```text
Hercules DJ Console Mk4
          |
          | USB
          v
       libusb
          |
          v
     hdjd-mac
          |
          | CoreMIDI
          v
       macOS MIDI
          |
          v
        Mixxx
```

The USB layer handles communication with the controller's bulk endpoints.

Incoming controller data is converted into standard MIDI messages and published through a CoreMIDI source. MIDI messages sent by applications are received through CoreMIDI and transmitted back to the controller over USB.

This allows applications such as Mixxx to see the controller as a MIDI device even though the hardware itself does not natively expose a standard MIDI interface on macOS.

## Requirements

* macOS
* Xcode Command Line Tools
* Homebrew
* `libusb`
* A supported Hercules DJ controller

Install `libusb` with Homebrew:

```bash
brew install libusb
```

Verify that `pkg-config` can find it:

```bash
pkg-config --cflags --libs libusb-1.0
```

You should receive output similar to:

```text
-I/opt/homebrew/Cellar/libusb/.../include/libusb-1.0
-L/opt/homebrew/Cellar/libusb/.../lib
-lusb-1.0
```

## Building

Clone the repository:

```bash
git clone https://github.com/FurjanD/hdjd-macos
cd hdjd-macos
```

Build the project:

```bash
make
```

For a debug build:

```bash
make clean
make DEBUG=1
```

To remove compiled files:

```bash
make clean
```

The build produces:

```text
hdjd
explore
```

`hdjd` is the main driver/bridge.

`explore` is the USB exploration utility inherited from the original project.

## Running

Connect the Hercules controller and run:

```bash
./hdjd
```

The program should locate the controller and report something similar to:

```text
Locating Hercules USB devices...

Vendor:Device = 06f8:b107

Opened [Hercules Hercules DJ Console Mk4]

Created CoreMIDI source [Hercules Hercules DJ Console Mk4]
```

For debug output, build with:

```bash
make clean
make DEBUG=1
```

Then run:

```bash
./hdjd
```

This may print the raw MIDI traffic received from the controller:

```text
Receiving on ep83: 90 24 7f
MIDI: 90 24 7f

Receiving on ep83: b0 39 47
MIDI: b0 39 47
```

Pressing buttons, moving controls, or rotating jog wheels should generate MIDI messages.

## Using With Mixxx

Once `hdjd` is running, macOS exposes a CoreMIDI source representing the controller.

Open Mixxx and configure the controller in:

```text
Preferences
    -> Controllers
```

The controller can then be mapped using Mixxx's MIDI controller mapping system.

For the Hercules DJ Console Mk4, a mapping that expects MIDI input can be used because `hdjd-mac` converts the controller's USB messages into MIDI before Mixxx receives them.

The controller does not need to appear as a native MIDI USB device in System Information. The important part is that CoreMIDI exposes the generated MIDI source to applications.

## MIDI Output and LEDs

The controller also supports MIDI messages sent in the opposite direction.

For example:

```text
Mixxx
  |
  | CoreMIDI
  v
hdjd-mac
  |
  | USB bulk transfer
  v
Hercules DJ Console Mk4
```

This allows software to control hardware features such as LEDs when the appropriate MIDI messages are sent.

MIDI output and LED control have been tested successfully on the Hercules DJ Console Mk4.

## macOS Driver Conflicts

The original Hercules macOS driver may claim the controller's USB interface.

If the original driver is running, `hdjd` may fail to claim the device because the interface is already in use.

Check for the original Hercules daemon:

```bash
ps aux | grep hdjsd
```

The original driver may appear as:

```text
/var/hercules/hd /var/hercules/hdjsd
```

The associated launch daemon is typically:

```text
/Library/LaunchDaemons/hdjsd.plist
```

The original driver should not be actively claiming the controller while `hdjd-mac` is running.

## Architecture

The project is split into several components.

### `usb.c`

Handles direct communication with the Hercules controller using `libusb`.

It is responsible for:

* Locating the controller
* Matching the USB vendor/product ID
* Claiming the required USB interface
* Receiving USB transfers
* Sending USB transfers
* Converting controller traffic into MIDI data
* Handling asynchronous USB transfers

### `coremidi.c`

Provides the macOS MIDI interface.

It uses Apple's CoreMIDI framework to:

* Create a MIDI client
* Create a CoreMIDI source
* Publish MIDI messages received from the controller
* Receive MIDI messages from MIDI applications
* Forward MIDI messages to the USB layer

### `coremidi.h`

Header for the CoreMIDI interface.

### `usb.h`

Header for the USB interface.

### `hdjd.c`

Contains the main program loop.

It coordinates USB and MIDI communication and handles program shutdown.

## Differences From the Original hdjd

The original project uses ALSA's sequencer interface on Linux.

Linux:

```text
USB
 |
libusb
 |
hdjd
 |
ALSA Sequencer
 |
MIDI application
```

This port replaces the ALSA layer with Apple's CoreMIDI framework:

```text
USB
 |
libusb
 |
hdjd-mac
 |
CoreMIDI
 |
MIDI application
```

The USB communication layer remains largely based on the original project.

## Why CoreMIDI?

macOS does not provide ALSA.

CoreMIDI is Apple's native MIDI framework and is the standard interface used by macOS MIDI applications.

Using CoreMIDI means the project does not require an additional MIDI translation application. Applications such as Mixxx can communicate with the CoreMIDI endpoint directly.

## Current Status

The Hercules DJ Console Mk4 has been successfully tested on modern macOS.

Tested functionality includes:

* USB device detection
* USB interface claiming
* Controller input
* MIDI message generation
* Jog wheel input
* Buttons and controls
* MIDI communication with Mixxx
* MIDI output
* Controller LEDs

The controller can be used with Mixxx without running Windows or a virtual machine.

## Limitations

This project is still experimental.

Not every Hercules controller supported by the original `hdjd` project has been tested on macOS.

Other limitations may include:

* Some controllers may use different USB interfaces or endpoints
* Some hardware-specific features may require additional reverse engineering
* macOS updates may affect USB or CoreMIDI behavior
* The original Hercules drivers may conflict with direct USB access

If you encounter a controller that is not working, debug output from:

```bash
make DEBUG=1
./hdjd
```

is useful when investigating the problem.

## Credits

This project is based on the original [hdjd](https://github.com/nealey/hdjd) project by Neale Pickett and retains much of its USB communication code and device information.

The macOS port replaces the original ALSA MIDI implementation with Apple's CoreMIDI framework.

Credit goes to the original `hdjd` developers for the reverse engineering and USB communication work that made this project possible.

## License

The original `hdjd` project is released under the MIT License.

See `LICENSE.md` for the applicable license terms.

If you redistribute or modify this project, please retain the original copyright and license information.

## Contributing

Contributions are welcome.

Useful contributions include:

* Testing additional Hercules controllers
* Adding device definitions
* Improving macOS compatibility
* Improving CoreMIDI handling
* Improving USB error handling
* Adding documentation
* Creating Mixxx mappings
* Reverse engineering additional controller features

When reporting an issue, please include:

1. Controller model
2. macOS version
3. Mac model/architecture
4. `libusb` version
5. Output from `./hdjd`
6. Debug output if applicable

## Disclaimer

This project is an independent community project and is not affiliated with or endorsed by Hercules or Guillemot.

Use it at your own risk. The original Hercules drivers and software are not required for the CoreMIDI functionality provided by this project.
