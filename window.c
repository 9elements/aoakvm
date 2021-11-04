#include <SDL2/SDL.h>

#include "aoakvm.h"
#include "window.h"
#include <unistd.h>

int
window_initWindow(  struct aoakvmMSGScreens *msgScreen,
                    struct aoakvmWindowProperties_t *props,
                    SDL_Window *window,
                    SDL_Renderer **renderer) {

    window = SDL_CreateWindow(props->titel, props->x, props->y, props->width , props->height, props->flags);
    if (window == NULL) {
        log_error("Could not create window: %s", SDL_GetError());
        return -1;
    }

    *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        log_error("Could not create renderer: %s", SDL_GetError());
        return -1;
        log_debug("Renderer Created");
    }

    int dw;
    int dh;
    SDL_GL_GetDrawableSize(window, &dw, &dh);

    SDL_Rect rect;
    SDL_GetDisplayUsableBounds(0, &rect);

    log_trace("Display Bounds: \t %d x %d", rect.w, rect.h);

    log_debug("create renderer");
    window_changeMsgscreenTo(msgScreen ,*renderer, window, WAIT_FOR_DEVICE);
    mainwindow = window;
    return 0;
}

int
window_setMsgscreens(struct aoakvmMSGScreens *msgScreen,
                    const char *waitDevice,
                    const char *aoaInit,
                    const char *dataTrans) {

    if( access(waitDevice, F_OK) != 0 ) {
        log_error("\"Wait for Device\"-Screen does not exist.");
        return -1;
    }
    msgScreen->waitForDevice = SDL_LoadBMP(waitDevice);

    if( access(aoaInit, F_OK) != 0 ) {
        log_error("\"AOA Initialized\"-Screen does not exist.");
        return -1;
    }
    msgScreen->aoaInitialized = SDL_LoadBMP(aoaInit);

    if( access(dataTrans, F_OK) != 0 ) {
        log_error("\"Wait for Data Transmission\"-Screen does not exist.");
        return -1;
    }
    msgScreen->waitForDataTransmission = SDL_LoadBMP(dataTrans);

    return 0;
}

int
window_changeMsgscreenTo(struct aoakvmMSGScreens *msgScreens,
                        SDL_Renderer *renderer,
                        SDL_Window *window,
                        enum aoakvm_msgscreen_states_enum img_num){

    SDL_Surface *image = NULL;

    switch (img_num) {
    case WAIT_FOR_DEVICE:
        image = msgScreens->waitForDevice;
    break;
    case AOA_INITIALIZED:
        image = msgScreens->aoaInitialized;
    break;
    case WAIT_FOR_DATA_TRANSMISSION:
        image = msgScreens->waitForDataTransmission;
    break;
    default:
        log_error("window_changeMsgscreenTo called without proper argument %d", img_num);
        return -1;
    break;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
    SDL_SetWindowSize(window, image->w / 2, image->h / 2);
    SDL_SetWindowResizable(window, false);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_DestroyTexture(texture);
    SDL_RenderPresent(renderer);
    return 0;
}
