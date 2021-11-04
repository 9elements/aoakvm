#ifndef HID
#define HID

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

static const uint8_t REPORT_DESC_TOUCHPAD[] =
    {
        0x05, 0x0d, // USAGE_PAGE (Digitizer)
        0x09, 0x02, // USAGE (Pen)
        0xa1, 0x01, // COLLECTION (Application)

        // declare a finger collection
        0x09, 0x20, //   Usage (Stylus)
        0xA1, 0x00, //   Collection (Physical)

        // Declare a finger touch (finger up/down)
        0x09, 0x42, //     Usage (Tip Switch)
        0x09, 0x32, //     USAGE (In Range)
        0x15, 0x00, //     LOGICAL_MINIMUM (0)
        0x25, 0x01, //     LOGICAL_MAXIMUM (1)
        0x75, 0x01, //     REPORT_SIZE (1)
        0x95, 0x02, //     REPORT_COUNT (2)
        0x81, 0x02, //     INPUT (Data,Var,Abs)

        // Declare the remaining 6 bits of the first data byte as constant -> the driver will ignore them
        0x75, 0x01, //     REPORT_SIZE (1)
        0x95, 0x06, //     REPORT_COUNT (6)
        0x81, 0x01, //     INPUT (Cnst,Ary,Abs)

        // Define absolute X and Y coordinates of 16 bit each (percent values multiplied with 100)
        // http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
        // Chapter 16.2 says: "In the Stylus collection a Pointer physical collection will contain the axes reported by the stylus."
        0x05, 0x01,       //     Usage Page (Generic Desktop)
        0x09, 0x01,       //     Usage (Pointer)
        0xA1, 0x00,       //     Collection (Physical)
        0x09, 0x30,       //        Usage (X)
        0x09, 0x31,       //        Usage (Y)
        0x16, 0x00, 0x00, //        Logical Minimum (0)
        0x26, 0x10, 0x27, //        Logical Maximum (10000)
        0x36, 0x00, 0x00, //        Physical Minimum (0)
        0x46, 0x10, 0x27, //        Physical Maximum (10000)
        0x66, 0x00, 0x00, //        UNIT (None)
        0x75, 0x10,       //        Report Size (16),
        0x95, 0x02,       //        Report Count (2),
        0x81, 0x02,       //        Input (Data,Var,Abs)
        0xc0,             //     END_COLLECTION

        0xc0, //   END_COLLECTION
        0xc0  // END_COLLECTION

        // With this declaration a data packet must be sent as:
        // byte 1   -> "touch" state          (bit 0 = pen up/down, bit 1 = In Range)
        // byte 2,3 -> absolute X coordinate  (0...10000)
        // byte 4,5 -> absolute Y coordinate  (0...10000)

};

/* HID report descriptor (mouse). taken from VBOX */
static const unsigned char REPORT_DESC_MOUSE[] = {
    // /* Usage Page */                0x05, 0x01,     /* Generic Desktop */
    // /* Usage */                     0x09, 0x02,     /* Mouse */
    // /* Collection */                0xA1, 0x01,     /* Application */
    // /* Usage */                     0x09, 0x01,     /* Pointer */
    // /* Collection */                0xA1, 0x00,     /* Physical */
    // /* Usage Page */                0x05, 0x09,     /* Button */
    // /* Usage Minimum */             0x19, 0x01,     /* Button 1 */
    // /* Usage Maximum */             0x29, 0x05,     /* Button 5 */
    // /* Logical Minimum */           0x15, 0x00,     /* 0 */
    // /* Logical Maximum */           0x25, 0x01,     /* 1 */
    // /* Report Count */              0x95, 0x05,     /* 5 */
    // /* Report Size */               0x75, 0x01,     /* 1 */
    // /* Input */                     0x81, 0x02,     /* Data, Value, Absolute, Bit field */
    // /* Report Count */              0x95, 0x01,     /* 1 */
    // /* Report Size */               0x75, 0x03,     /* 3 (padding bits) */
    // /* Input */                     0x81, 0x03,     /* Constant, Value, Absolute, Bit field */

    // /* Usage Page */                0x05, 0x01,     /* Generic Desktop */
    // /* Usage */                     0x09, 0x30,     /* X */
    // /* Usage */                     0x09, 0x31,     /* Y */
    // /* Usage */                     0x09, 0x38,     /* Z (wheel) */ // ignored in test
    // /* Logical Minimum */           0x15, 0x00,     /* 0 */
    // /* Logical Maximum */           0x25, 0x7F,     /* +127 */
    // /* Report Size */               0x75, 0x08,     /* 8 */
    // /* Report Count */              0x95, 0x02,     /* 3 */
    // /* Input */                     0x81, 0x02,     /* Data, Value, Relative, Bit field */
    // /* End Collection */            0xC0,
    // /* End Collection */            0xC0,
    /* Usage Page */ 0x05,
    0x01, /* Generic Desktop */
    /* Usage */ 0x09,
    0x02, /* Mouse */
    /* Collection */ 0xA1,
    0x01, /* Application */
    /* Usage */ 0x09,
    0x01, /* Pointer */
    /* Collection */ 0xA1,
    0x00, /* Physical */
    /* Usage Page */ 0x05,
    0x09, /* Button */
    /* Usage Minimum */ 0x19,
    0x01, /* Button 1 */
    /* Usage Maximum */ 0x29,
    0x05, /* Button 5 */
    /* Logical Minimum */ 0x15,
    0x00, /* 0 */
    /* Logical Maximum */ 0x25,
    0x01, /* 1 */
    /* Report Count */ 0x95,
    0x05, /* 5 */
    /* Report Size */ 0x75,
    0x01, /* 1 */
    /* Input */ 0x81,
    0x02, /* Data, Value, Absolute, Bit field */
    /* Report Count */ 0x95,
    0x01, /* 1 */
    /* Report Size */ 0x75,
    0x03, /* 3 (padding bits) */
    /* Input */ 0x81,
    0x03, /* Constant, Value, Absolute, Bit field */
    /* Usage Page */ 0x05,
    0x01, /* Generic Desktop */
    /* Usage */ 0x09,
    0x30, /* X */
    /* Usage */ 0x09,
    0x31, /* Y */
    /* Usage */ 0x09,
    0x38, /* Z (wheel) */
    /* Logical Minimum */ 0x15,
    0x81, /* -127 */
    /* Logical Maximum */ 0x25,
    0x7F, /* +127 */
    /* Report Size */ 0x75,
    0x08, /* 8 */
    /* Report Count */ 0x95,
    0x03, /* 3 */
    /* Input */ 0x81,
    0x06, /* Data, Value, Relative, Bit field */
    /* End Collection */ 0xC0,
    /* End Collection */ 0xC0,
};

// static  const unsigned char REPORT_DESC_MOUSE[] = {
//     /* Usage Page */                0x05, 0x01,     /* Generic Desktop */
//     /* Usage */                     0x09, 0x02,     /* Mouse */
//     /* Collection */                0xA1, 0x01,     /* Application */
//     /* Usage */                     0x09, 0x01,     /* Pointer */
//     /* Collection */                0xA1, 0x00,     /* Physical */
//     /* Usage Page */                0x05, 0x09,     /* Button */
//     /* Usage Minimum */             0x19, 0x01,     /* Button 1 */
//     /* Usage Maximum */             0x29, 0x05,     /* Button 5 */
//     /* Logical Minimum */           0x15, 0x00,     /* 0 */
//     /* Logical Maximum */           0x25, 0x01,     /* 1 */
//     /* Report Count */              0x95, 0x05,     /* 5 */
//     /* Report Size */               0x75, 0x01,     /* 1 */
//     /* Input */                     0x81, 0x02,     /* Data, Value, Absolute, Bit field */
//     /* Report Count */              0x95, 0x01,     /* 1 */
//     /* Report Size */               0x75, 0x03,     /* 3 (padding bits) */
//     /* Input */                     0x81, 0x03,     /* Constant, Value, Absolute, Bit field */
//     /* Usage Page */                0x05, 0x01,     /* Generic Desktop */
//     /* Usage */                     0x09, 0x38,     /* Z (wheel) */
//     /* Logical Minimum */           0x15, 0x81,     /* -127 */
//     /* Logical Maximum */           0x25, 0x7F,     /* +127 */
//     /* Report Size */               0x75, 0x08,     /* 8 */
//     /* Report Count */              0x95, 0x01,     /* 1 */
//     /* Input */                     0x81, 0x06,     /* Data, Value, Relative, Bit field */
//     /* Usage Page */                0x05, 0x0C,     /* Consumer Devices */
//     /* Usage */                     0x0A, 0x38, 0x02,/* AC Pan (horizontal wheel) */
//     /* Report Count */              0x95, 0x01,     /* 1 */
//     /* Input */                     0x81, 0x06,     /* Data, Value, Relative, Bit field */
//     /* Report Size */               0x75, 0x08,     /* 8 (padding byte) */
//     /* Report Count */              0x95, 0x01,     /* 1 */
//     /* Input */                     0x81, 0x03,     /* Constant, Value, Absolute, Bit field */
//     /* Usage Page */                0x05, 0x01,     /* Generic Desktop */
//     /* Usage */                     0x09, 0x30,     /* X */
//     /* Usage */                     0x09, 0x31,     /* Y */
//     /* Logical Minimum */           0x15, 0x00,     /* 0 */
//     /* Logical Maximum */           0x26, 0xFF,0x7F,/* 0x7fff */
//     /* Physical Minimum */          0x35, 0x00,     /* 0 */
//     /* Physical Maximum */          0x46, 0xFF,0x7F,/* 0x7fff */
//     /* Report Size */               0x75, 0x10,     /* 16 */
//     /* Report Count */              0x95, 0x02,     /* 2 */
//     /* Input */                     0x81, 0x02,     /* Data, Value, Absolute, Bit field */
//     /* End Collection */            0xC0,
//     /* End Collection */            0xC0,
// };

static const unsigned char REPORT_DESC_KB[] = {
    /* Usage Page */ 0x05,
    0x01, /* Generic Desktop */
    /* Usage */ 0x09,
    0x06, /* Keyboard */
    /* Collection */ 0xA1,
    0x01, /* Application */
    /* Usage Page */ 0x05,
    0x07, /* Keyboard */
    /* Usage Minimum */ 0x19,
    0xE0, /* Left Ctrl Key */
    /* Usage Maximum */ 0x29,
    0xE7, /* Right GUI Key */
    /* Logical Minimum */ 0x15,
    0x00, /* 0 */
    /* Logical Maximum */ 0x25,
    0x01, /* 1 */
    /* Report Count */ 0x95,
    0x08, /* 8 */
    /* Report Size */ 0x75,
    0x01, /* 1 */
    /* Input */ 0x81,
    0x02, /* Data, Value, Absolute, Bit field */
    /* Report Count */ 0x95,
    0x01, /* 1 */
    /* Report Size */ 0x75,
    0x08, /* 8 (padding bits) */
    /* Input */ 0x81,
    0x01, /* Constant, Array, Absolute, Bit field */
    /* Usage Page */ 0x05,
    0x08, /* LEDs */
    /* Report Count */ 0x95,
    0x06, /* 6 */
    /* Report Size */ 0x75,
    0x08, /* 8 */
    /* Logical Minimum */ 0x15,
    0x00, /* 0 */
    /* Logical Maximum */ 0x26,
    0xFF,
    0x00, /* 255 */
    /* Usage Page */ 0x05,
    0x07, /* Keyboard */
    /* Usage Minimum */ 0x19,
    0x00, /* 0 */
    /* Usage Maximum */ 0x29,
    0xFF, /* 255 */
    /* Input */ 0x81,
    0x00, /* Data, Array, Absolute, Bit field */
    /* End Collection */ 0xC0,
};

#define REPORT_DESC_SIZE(x) (sizeof(x) / sizeof(x[0]))

int register_hid(libusb_device_handle *handle, uint16_t descriptor_size, int hid_index);

int send_hid_descriptor(libusb_device_handle *handle,
                        const unsigned char *descriptor, uint16_t size,
                        uint8_t max_packet_size_0, int hid_index);

#endif