#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <inttypes.h>
#include <xcb/xcb.h>
//#include <xcb/xcb_keysyms.h>

#include "rainwm.h"

#define AUTHOR "Rayne Blake"
#define VERSION "0.1"

static xcb_connection_t *dpy;
static xcb_screen_t *scre;
static xcb_drawable_t win;

static uint32_t values[3];

/* String comparison. Return 0 if equal, else return nonzero int */
static int str_compare(char *str1, char *str2)
{
    char *c1 = str1;
    char *c2 = str2;
    while ((* c1) && ((* c1) == (* c2)))
    {
        ++c1;
        ++c2;
    }
    int n = (* c1) - (* c2);
  return n;
}

static int event_handler(void)
{
    /* Let's make sure everything's still hooked up right */
    int ret = xcb_connection_has_error(dpy);
    if (ret != 0)
    {
        printf("xcb_connection_has_error, inside event_handler\n");
        return ret;
    }
    xcb_generic_event_t *event = xcb_wait_for_event(dpy);
    if (event == NULL)
    {
        xcb_flush(dpy);
        return ret;
    }
    handler_func_t *handler;
    for (handler = handler_functions; handler->func != NULL; handler++)
    {
        if ((event->response_type & ~0x80) == handler->request) handler->func(event);
    }
    free(event);

    xcb_flush(dpy);
    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    /* If crwm -v, print the version info */
    if (( argc == 2) && (str_compare("-v", argv[1]) == 0))
    {
        printf("rainwm-%s, Copyright 2022 %s, GPLv3\n", VERSION, AUTHOR);
        return 0;
    }
    /* If any parameters other than the above, print usage info */
    if (argc != 1)
    {
        printf("usage: rainwm [-v]\n");
        return 0;
    }
    /* If nothing has broken us out yet, connect to the X server */
    dpy = xcb_connect(NULL, NULL);
    ret = xcb_connection_has_error(dpy);
    if (ret != 0)
    {
        printf("xcb_connection_has_error, main function\n");
        return ret;
    }

    /* Set up the screen object, and sign up to be the window manager */
    scre = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT  // mask values needed to register as a window manager
              | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    	  | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
    	  | XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes_checked(dpy, scre->root, XCB_CW_EVENT_MASK, values);
    xcb_flush(dpy);

    /* Register to grab the Mod+lmb and Mod+rmb events for mouse control of windows */
    xcb_grab_button(dpy, 0, scre->root,  // c, pass through grabbed events?, grab_window
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,  // which events to grab
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,  // pointer grab mode, keyboard grab mode
                    scre->root,  // confine to this screen
                    XCB_NONE,  // change cursor while grabbed (XCB_NONE for no change)
                    1, XCB_MOD_MASK_4);  // button to grab, only grab when this mod is pressed

    xcb_grab_button(dpy, 0, scre->root,  
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,  
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,  
                    scre->root,  
                    XCB_NONE,  
                    3, XCB_MOD_MASK_4);  
    xcb_flush(dpy);

    /* Now we start the loop to start dealing with all these windows and such */
    while (ret == 0) ret = event_handler();

    return ret;
}
