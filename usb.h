#ifndef AOAKVM_USB
#define AOAKVM_USB

#include "aoakvm.h"

/*
    libusb_device_handle usb_getHandle(libusb_context *ctx);

    Initializes the given context and returns a libusb_device_handle which is found by the
    properties of accessory vid and accessory pid or altenative accessory pid.
*/
libusb_device_handle *usb_getHandle(struct aoakvmConfig_t*);
libusb_device_handle *usb_get_aoa_handle();

int usb_registerHIDS(libusb_device_handle*);

void usb_setConnectionState(enum aoakvm_usb_status_e);

int usb_read_stream(void*);

void usb_writeToPhone(struct usbRequest_t);

#endif