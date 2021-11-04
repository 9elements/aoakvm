#include <unistd.h>
#include <SDL2/SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libusb-1.0/libusb.h>

#include "aoakvm.h"
#include "usb.h"
#include "window.h"
#include "video.h"


// Local Variables
Uint32 DEVICE_CONNECTION_EVENT = ((Uint32)-1);
Uint32 DEVICE_DISCONNECTION_EVENT = ((Uint32)-1);

/*
    libav related structures and variables
*/
struct aoakvmAVCtx_t avCtx;
AVIOContext *reader;

/*
    usb related structures and variables
*/
struct aoakvmUSBConnection_t *usbCon;

SDL_Window *mainwindow;
SDL_Renderer *renderer;

SDL_Thread *process_event_thread_handler;
SDL_Thread *read_from_usb_thread_handler;

/*
    MsgScreen
*/
struct aoakvmMSGScreens *screens;


int invoke_aoakvm(struct aoakvmConfig_t *cfg, struct aoakvmWindowProperties_t *windowProps, int (*eventThread)()) {

    // Setup all the things
    struct aoakvmUSBConnection_t con;
    usbCon = &con;
    struct aoakvmMSGScreens msgscr;
    screens = &msgscr;

    log_info("AOAKV initializing...");
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        log_info("SLD init failed");
        return -1;
    }

    if (window_setMsgscreens(&msgscr, cfg->waitForDevice, cfg->aoaInit, cfg->waitForDataTransmission) < 0) {
        log_error("Setting message screens failed");
        return -1;
    }

    if (window_initWindow(screens, windowProps, mainwindow, &renderer) < 0) {
        log_error("Can't open window");
        return -1;
    }

    if (window_changeMsgscreenTo(screens, renderer, mainwindow, WAIT_FOR_DEVICE) < 0) {
        log_error("Cant set 'WAIT_FOR_DEVICE' screen");
        return 0;
    }

    process_event_thread_handler = SDL_CreateThread(eventThread, "eventThread", NULL);
    if (process_event_thread_handler == NULL) {
            log_error("Failed to create event_thread.");
            return -1;
    }

    // Main loop for the program.
    while(1) {
		if (window_changeMsgscreenTo(screens, renderer, mainwindow, WAIT_FOR_DEVICE) < 0) {
            log_error("Cant set 'WAIT_FOR_DEVICE' screen");
            return 0;
        }
        do {
            con.handle = usb_getHandle(cfg);
        } while (con.handle == NULL);

        log_info("Gerät gefunden, initialisiere.");

        // Get AVContext to open stream
        reader = video_setupAVContext(con.handle);
        if (reader == NULL) {
            log_info("Failed to set up AVContext");
            libusb_close(con.handle);
            con.handle = NULL;
            continue;
        }

        // Register keyboard and mouse with AOA-device
        if (usb_registerHIDS(con.handle) < 0) {
            log_info("Register usb device as AOA failed");
            libusb_close(con.handle);
            con.handle = NULL;
            continue;
        }

        log_debug("changeMsgScreen");
        if (window_changeMsgscreenTo(screens, renderer, mainwindow, WAIT_FOR_DATA_TRANSMISSION) < 0) {
                log_info("Cant set 'WAIT_FOR_DEVICE' screen");
                return -1;
        }

        // Initiate video transfer via usb connection
        log_debug("video_openStream");
        if (video_openStream(reader, &((&avCtx)->fmt_ctx), &((&avCtx)->codec_ctx)) < 0) {
            log_info("Failed to open stream");
            avio_context_free(&reader);
            libusb_close(con.handle);
            con.handle = NULL;
            continue;
        }

        // Start reading from stream thread
        read_from_usb_thread_handler = SDL_CreateThread(usb_read_stream, "readPackagesFromStream", (void *) &avCtx);
        if (!read_from_usb_thread_handler) {
            log_error("Could not start read_from_stream thread!");
        }

        // Connect data stream with renderer
        if (video_initRenderer(&avCtx, &renderer) < 0) {
            log_info("Failed to init renderer");
            libusb_close(con.handle);
            avio_context_free(&reader);
            con.handle = NULL;
            continue;
        }


        usb_setConnectionState(CONNECTED);
        // This is the continous rendering loop.
        int err = 0;
        do {
            // Check USB Connection
            if (usbCon->status == NOT_CONNECTED) {
                err = -2;
                break;
            }
            err = video_rendering(renderer);
        } while(err == 0);

        int status = 0;
        switch (err) {
            case -1:
                usb_setConnectionState(NOT_CONNECTED);
                libusb_close(con.handle);
                SDL_WaitThread(read_from_usb_thread_handler, &status);

            break;
            case -2:
                usb_setConnectionState(NOT_CONNECTED);
                libusb_close(con.handle);
                SDL_WaitThread(read_from_usb_thread_handler, &status);
            break;
            default:
            break;
        }

    }

    return 0;
}

int aoakvm_push_event(Uint32 *eventType, void *data1, void *data2) {
  log_debug("aoakvm_push_event");
  if (*eventType == ((Uint32)-1)) { //New Event
    *eventType = SDL_RegisterEvents(1);
  }
  if (*eventType != ((Uint32)-1)) {
    SDL_Event event;
    SDL_memset(&event, 0, sizeof(event));
    event.type = *eventType;
    event.user.code = 1;
    event.user.data1 = data1;
    event.user.data2 = data2;
    SDL_PushEvent(&event);
  } else {//Too many Custom events registered
    return -1;
  }
  return 0;
}

int aoakvm_wait_for_device_connection() {
  log_debug("event_loop: Wait for device Connection.");
  SDL_Event event;
  while (true)
  {
    while (SDL_PollEvent(&event)) {
      if (event.type == DEVICE_CONNECTION_EVENT) {
        //Device has been connected
        log_debug("event_loop: device connected");
        return 0;
      } else if (event.type == SDL_KEYDOWN) {
        SDL_KeyboardEvent *keyevent = (SDL_KeyboardEvent *)&event;
        if (keyevent->repeat == 0)
        { // no repeated key
          if (keyevent->keysym.scancode == 0x29){
            //ESC-Key
            exit_request();
          }
        }
      } else if (event.type == SDL_QUIT) {
        //Window x key pressed
        exit_request();
      }
    }
  }
  return -1;
}

void exit_request() {
  // Close down the Avio Context
  avio_context_free(&reader);
  exit(1);
}
