#ifndef AOAKVM_USB
#define AOAKVM_USB

#include "aoakvm.h"

/*
    libusb_device_handle usb_getHandle(libusb_context *ctx);

    Initializes the given context and returns a libusb_device_handle which is found by the
    properties of accessory vid and accessory pid or altenative accessory pid.
*/
libusb_device_handle *usb_getHandle(struct aoakvmConfig_t*);

/*
    libusb_device_handle *usb_get_aoa_handle();

    Initializes Android Open Accessory mode on the phone. It does so by sending data held in *cfg
    via libusb_control_transfer(...) to the phone and returns a new handle which represents the new state
    of the usb connection.
*/
libusb_device_handle *usb_get_aoa_handle();

/*
    int usb_registerHIDS(libusb_device_handle*);

    This function takes the libusb_device_handle which points to an usb device with AOA initialized and
    registers the desktop app as human interface devices (mouse, keyboard, touchpad)
*/
int usb_registerHIDS(libusb_device_handle*);

/*
    void usb_setConnectionState(enum aoakvm_usb_status_e);

    This function takes an enum of aoakvm_usb_status_e and sets the connecntion state accordingly
*/
void usb_setConnectionState(enum aoakvm_usb_status_e);

/*
    int usb_read_stream(*void)

    This function is used internally to read packets from the usb video stream supplied by the android ScrCpy app.
*/
int usb_read_stream(void*);

/*
    void usb_writeToPhone(struct usbRequest_t);

    This function sends requests to the phone depending on the event from the SDL event queue.
*/
void usb_writeToPhone(struct usbRequest_t);

#endif