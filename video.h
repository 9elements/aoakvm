#ifndef AOAKVM_VIDEO
#define AOAKVM_VIDEO

#include "aoakvm.h"

/*
    AVIOContext *usb_setupAVContext(libusb_device_handle *handle);

    Creates AVIOContext for the video stream via libusb_device_handle
*/
AVIOContext *video_setupAVContext(libusb_device_handle*);

/*
    int video_initRenderer(struct aoakvmAVCtx_t*, SDL_Renderer**);

    This function initilizes a renderer and sets av codec and av format context.
*/
int video_initRenderer(struct aoakvmAVCtx_t*, SDL_Renderer**);

/*
    int video_rendering(SDL_Renderer *renderer);

    Takes a pointer to the previously set renderer.
    This function draws one frame from the queue into the window and is called at the end of the invoke_aoakvm loop before the error checking.
*/
int video_rendering(SDL_Renderer *renderer);

/*
    int video_openStream(AVIOContext*, AVFormatContext**, AVCodecContext**);

    This function takes a pointer to the AVIOContext. This pointer comes from video_setupAVContext(...). Also the address to the renderer
    and  AVCodec context must be supplied. This configures the the video stream and sets everzthing up so that the frames in the queue have the
    correct format to be loaded by the renderer.
*/
int video_openStream(AVIOContext*, AVFormatContext**, AVCodecContext**);


/*
    int fq_pushFrameIntoQueue(AVFrame *frame);

    Pushes the frame into the FrameQueue for further process.
*/
int fq_pushFrameIntoQueue(AVFrame *frame);

#endif