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

 /*
	int window_setMsgscreens(struct aoakvmMSGScreens*, const char*, const char*, const char*);

	The function takes the message screen structure and three const char pointer and sets the paths to
	the background pictures which are used to indicate status of usb connection.

	Returns 0 on success and -1 on error
 */
 int window_setMsgscreens(struct aoakvmMSGScreens*, const char*, const char*, const char*);

 /*
	 int window_changeMsgscreenTo(struct aoakvmMSGScreens*, SDL_Renderer*, SDL_Window*, enum aoakvm_msgscreen_states_enum);

	 Function takes message screen structure to select the correct picture depending on the enum to render the picture in the given window.

	 Returns 0 on success and -1 on error.
 */
 int window_changeMsgscreenTo(struct aoakvmMSGScreens*, SDL_Renderer*, SDL_Window*, enum aoakvm_msgscreen_states_enum);

 #endif