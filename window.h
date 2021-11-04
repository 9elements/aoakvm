#ifndef AOAKVM_WINDOW
#define AOAKVM_WINDOW

#include "aoakvm.h"

// ###############################
// window related functions
// ###############################
/*
    int window_init_window(struct WindowProperties *props, SDL_Renderer **renderer);

    Takes properties for the windows to have, a pointer to a SDL_Window and a double pointer of SDL_Renderer to
    create a window with the given properties.
*/
 int window_initWindow(struct aoakvmMSGScreens*, struct aoakvmWindowProperties_t*, SDL_Window*, SDL_Renderer**);
 int window_setMsgscreens(struct aoakvmMSGScreens*, const char*, const char*, const char*);
 int window_changeMsgscreenTo(struct aoakvmMSGScreens*, SDL_Renderer*, SDL_Window*, enum aoakvm_msgscreen_states_enum);

 #endif