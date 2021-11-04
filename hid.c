#include "hid.h"
#include <errno.h>
#include <error.h>
#include <libusb-1.0/libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <linux/input.h>
#include <fcntl.h>
#include <pthread.h>

const char *mousefile = NULL;
const char *keyboardfile = NULL;

#define REPORT_DESC_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define HID_EVENT_MOUSE_SIZE (sizeof(HID_EVENT_MOUSE) / sizeof(HID_EVENT_MOUSE[0]))
#define HID_EVENT_KB_SIZE (sizeof(HID_EVENT_KB) / sizeof(HID_EVENT_KB[0]))

// <https://source.android.com/devices/accessories/aoa2#hid-support>
#define AOA_REGISTER_HID 54
#define AOA_UNREGISTER_HID 55
#define AOA_SET_HID_REPORT_DESC 56
#define AOA_SEND_HID_EVENT 57

#define DEFAULT_TIMEOUT 1000

static void print_libusb_error(enum libusb_error errcode)
{
    fprintf(stderr, "%s\n", libusb_strerror(errcode));
}

int register_hid(libusb_device_handle *handle, uint16_t descriptor_size, int hid_index)
{
    const uint8_t requestType = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR;
    const uint8_t request = AOA_REGISTER_HID;
    // <https://source.android.com/devices/accessories/aoa2.html#hid-support>
    // value (arg0): accessory assigned ID for the HID device
    // index (arg1): total length of the HID report descriptor
    const uint16_t value = hid_index;
    const uint16_t index = descriptor_size;
    unsigned char *const buffer = NULL;
    const uint16_t length = 0;
    const unsigned int timeout = DEFAULT_TIMEOUT;
    int r = libusb_control_transfer(handle, requestType, request,
                                    value, index, buffer, length, timeout);
    if (r < 0)
    {
        print_libusb_error(r);
        return 1;
    }
    return 0;
}

int send_hid_descriptor(libusb_device_handle *handle,
                        const unsigned char *descriptor, uint16_t size,
                        uint8_t max_packet_size_0, int hid_index)
{
    const uint8_t requestType = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR;
    const uint8_t request = AOA_SET_HID_REPORT_DESC;
    // <https://source.android.com/devices/accessories/aoa2.html#hid-support>
    // value (arg0): accessory assigned ID for the HID device
    const uint16_t value = hid_index;
    // libusb_control_transfer expects non-const but should not modify it
    unsigned char *const buffer = (unsigned char *)descriptor;
    const unsigned int timeout = DEFAULT_TIMEOUT;
    /*
     * If the HID descriptor is longer than the endpoint zero max packet size,
     * the descriptor will be sent in multiple ACCESSORY_SET_HID_REPORT_DESC
     * commands. The data for the descriptor must be sent sequentially
     * if multiple packets are needed.
     *
     * <https://source.android.com/devices/accessories/aoa2.html#hid-support>
     */
    // index (arg1): offset of data (buffer) in descriptor
    uint16_t offset = 0;
    while (offset < size)
    {
        uint16_t packet_length = size - offset;
        if (packet_length > max_packet_size_0)
        {
            packet_length = max_packet_size_0;
        }
        int r = libusb_control_transfer(handle, requestType, request, value,
                                        offset, buffer + offset, packet_length,
                                        timeout);
        offset += packet_length;
        if (r < 0)
        {
            print_libusb_error(r);
            return 1;
        }
    }

    return 0;
}