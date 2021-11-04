#ifndef AOAKVM_VIDEO
#define AOAKVM_VIDEO

#include "aoakvm.h"

/*
    AVIOContext *usb_setupAVContext(libusb_device_handle *handle);

    Creates AVIOContext for the video stream via libusb_device_handle
*/
AVIOContext *video_setupAVContext(libusb_device_handle*);

int video_initRenderer(struct aoakvmAVCtx_t*, SDL_Renderer**);
int video_rendering(SDL_Renderer *renderer);
int video_openStream(AVIOContext*, AVFormatContext**, AVCodecContext**);

int fq_pushFrameIntoQueue(AVFrame *frame);

#endif