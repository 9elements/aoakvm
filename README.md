# AOAKVM

AOAKVM is a library for streaming and remote accessing an usb-attached android
phone. AOAKVM does *not* require elevated rights like `adb` in order to function
properly. It uses the USB accessibility protocols to stream and remote control
android.

## How to use it

### Requirements

Before you start, make sure you have [libsdl](https://www.libsdl.org/), [libav](https://www.ffmpeg.org/download.html), [libusb](https://libusb.info/) installed for development. Check your favorite package manager for the dev packages accordingly.

The package exposes two structures to supply required data to it.

```
aoakvmConfig_t
```
This structure is used to tell the application paths to bmp files which will be shown in the process of connecting the computer to the phone via usb.
Also it holds information about the AOA configuration which is used to kickstart the mobile app on the android phone.
Information of manufacturer, modelName, version, description and serialNumber can be used to filter for the connection on the android app.
```
aoakvmWindowProperties_t
```
This is used to configure the properties of the SDL window like titel, window height and width, position in x and y coordinates and SDL specific flags.

For fieldnames of the structures see the documentation in aoakvm.h

### Event handling
You are required to implement an event loop function and pass its name to the `invoke_aoakvm` function.
The function will run in a seperate thread and handles all SDL events for the application like keystrokes, mouse movement and
touch events. Also important that the the first function inside the outer loop must be `aoakvm_wait_for_device_connection()` which blocks the
loop until usb connection setup is complete. E.g:
```
int event_loop() {
    while(1) {
        aoakvm_wait_for_device_connection();
        while(SDL_WaitEvent(&event)) {
            switch (event.type) {
                case <<SDL_EVENT_TYPES>>:
                break:
                default:
                    if (event.type == DEVICE_DISCONNECT_EVENT) {
                        aoakvm_wait_for_device_connection();
                    }
                break;
            }
        }
    }
}
```


## How to contribute
You want to contribute? We're happy to have you! Open a pull request and help us make this software a success.
Thank you in advance for your time and effort.
## Roadmap
tba.

## Supporters

This project has been sponsored by [Secunet](https://www.secunet.com)
