#ifndef AOA_KVM
#define AOA_KVM

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include <libusb-1.0/libusb.h>
#include <SDL2/SDL.h>

#include "aoakvm_log.h"

#define HID_EVENT_MOUSE_SIZE (sizeof(HID_EVENT_MOUSE) / sizeof(HID_EVENT_MOUSE[0]))
#define HID_EVENT_TOUCHPAD_SIZE (sizeof(HID_EVENT_TOUCHPAD) / sizeof(HID_EVENT_TOUCHPAD[0]))

#define HID_EVENT_KB_SIZE (sizeof(HID_EVENT_KB) / sizeof(HID_EVENT_KB[0]))

#define AOA_REGISTER_HID 54
#define AOA_UNREGISTER_HID 55
#define AOA_SET_HID_REPORT_DESC 56
#define AOA_SEND_HID_EVENT 57

#define DEFAULT_TIMEOUT 1000

/*
    aoakvmMSGScreens_t

    This struct holds three surfaces to show for the different states of the usb connection
    Fields:
        SDL_Surface *waitForDevice;
        SDL_Surface *aoaInitialized;
        SDL_Surface *waitForDataTransmission;
*/
struct aoakvmMSGScreens {
  SDL_Surface *waitForDevice;
  SDL_Surface *aoaInitialized;
  SDL_Surface *waitForDataTransmission;
};

/*
    aoakvmUsbConfig_t

    This struct holds information for the initialization of Android Open Accessory via USB
    Fields:
        const char *waitForDevice;
        const char *aoaInit;
        const char *waitForDataTransmission;
        const char *manufacturer;
        const char *modelName;
        const char *description;
        const char *version;
        const char *uri;
        const char *serialNumber;
*/
struct aoakvmConfig_t {
    const char *waitForDevice;
    const char *aoaInit;
    const char *waitForDataTransmission;
    const char *manufacturer;
    const char *modelName;
    const char *description;
    const char *version;
    const char *uri;
    const char *serialNumber;
};

/*
    aoakvm_msgscreen_states_enum

    This enum represents the three states the application can have.
    Values:
        WAIT_FOR_DEVICE = 1,
        AOA_INITIALIZED = 2,
        WAIT_FOR_DATA_TRANSMISSION = 3,
*/
enum aoakvm_msgscreen_states_enum {
  /** Success (no error) */
  WAIT_FOR_DEVICE = 1,
  AOA_INITIALIZED = 2,
  WAIT_FOR_DATA_TRANSMISSION = 3,
};

/*
    aoakvmWindowProperties_t

    This struct holds configuration data for the window to be created.
    Fields:
        const char title;
        const int x;        x-position to screen    - Of type SDL_WINDOWPOS_xxx
        const int y;        y-position to screen    - Of type SDL_WINDOWPOS_xxx
        const int width;    width of window in px   - user specified
        const int height;   height of window in px  - user specified
        const Uint32 flags; Flags for the window.   - Of type SDL_WINDOW_xxx.
*/
struct aoakvmWindowProperties_t {
    const char *titel;
    int x;
    int y;
    int width;
    int height;
    uint32_t flags;
};

/*
    aoakvmAVCtx_t

    This struct represents a struct type for holding the AVFormatContext
    and AVCodecContext required for rendering
*/
struct aoakvmAVCtx_t {
    AVFormatContext *fmt_ctx;
    AVCodecContext *codec_ctx;
};

/*
    aoakvm_usb_status_e

    This enum represents states a usb connection can have
    Entries:
        NOT_CONNECTED = 0
        CONNECTED = 1
*/

enum aoakvm_usb_status_e {
    NOT_CONNECTED,
    CONNECTED,
};

/*
    aoakvmUSBConnection_t

    This struct repesents a USB device and the state of the connection
*/

struct aoakvmUSBConnection_t {
    libusb_device_handle *handle;
    enum aoakvm_usb_status_e status;
};

/*
    extern struct aoakvmUSBConnection_t *usbCon;

    Defined in aoakvm.c

    This variable gives public access to the usb connection variable.
*/
extern struct aoakvmUSBConnection_t *usbCon;

/*

    DEVICE_CONNECTION_EVENT;
    DEVICE_DISCONNECTION_EVENT

    This variables are defined in aoakvm.c and accessable from any file

*/

extern Uint32 DEVICE_CONNECTION_EVENT;
extern Uint32 DEVICE_DISCONNECTION_EVENT;

/*
    int aoakvm_push_event(Uint32 *eventType, void *data1, void *data2);

    This function takes an custom event type and creates a new event for the event queue.
*/
int aoakvm_push_event(Uint32 *eventType, void *data1, void *data2);

/*
    aoakvm_wait_for_device_connection

    This should be called by the event_loop when the usb device is not available yet,
    or the connection has been lost

*/
int aoakvm_wait_for_device_connection();

/*
    struct usbRequest_t

    Fields:
        uint8_t requestType;
        uint8_t request;
        uint16_t value;
        uint16_t index;
        unsigned char *buffer;
        uint16_t length;
        unsigned int timeout;

        This structure is used to pass data to the usb_writeToPhone(...)
*/
struct usbRequest_t {
  uint8_t requestType;
  uint8_t request;
  uint16_t value;
  uint16_t index;
  unsigned char *buffer;
  uint16_t length;
  unsigned int timeout;
};


/*
    extern SDL_Window *mainwindow;

    Defined in aoakvm.c

    This variable gives public access to the main window variable.
*/
extern SDL_Window *mainwindow;

/*
    extern struct aoakvmUSBConnection_t *usbCon;

    Defined in aoakvm.c

    This variable gives public access to the usb connection variable.
*/
extern SDL_Renderer *renderer;

/*
    extern struct aoakvmMSGScreens *screens;

    Defined in aoakvm.c

    This variable gives public access to the message screen variable.
*/
extern struct aoakvmMSGScreens *screens;

/*
    invoke_aoakvm(struct aoakvmUsbConfig_t *cfg, int (*eventThread)());

    This function invokes the whole functionality of aoakvm.
    User needs to supply a pointer to aoakvmUsbConfig_t struct and a pointer to
    a function which will be invoked as the event loop. Further a pointer to

*/
int invoke_aoakvm(struct aoakvmConfig_t *, struct aoakvmWindowProperties_t*, int (*fn)());

/*

    exit_request
    cleans up on exit

*/
void exit_request();



#endif