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

const xcb_mod_mask_t modkey = XCB_MOD_MASK_4;


static void handle_motion_notify(xcb_generic_event_t *event)
{
    (void) ev;
    xcb_query_pointer_cookie_t point_cookie = xcb_query_pointer(dpy, scre->root);
    xcb_query_pointer_reply_t *point = xcb_query_pointer_reply(dpy, point_cookie, 0);

    if ((values[2] == (uint32_t)(1)) && (win != 0))
    {
        xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(dpy, win);
        xcb_get_geometry_reply_t *geom = xcb_get_reply(dpy, geom_cookie, NULL);
        uint16_t geom_x = geom->width + (2 * BORDER_WIDTH);
        uint16_t geom_y = geom->height + (2 * BORDER_WIDTH);
        values[0] = ((point->root_x + geom_x) > scre->width_in_pixels) ?
	        (scre->width_in_pixels - geom_x) : point->root_x;
	values[1] = ((point->root_y + geom_y) > scre->height_in_pixels) ?
	        (scre->height_in_pixels - geom_y) : point->root_y;
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    } else if ((values[2] == (uint32_t)(3)) && (win != 0))
    { 
        xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(dpy, win);
        xcb_get_geometry_reply_t *geom = xcb_get_reply(dpy, geom_cookie, NULL);
        if (!((point->root_x <= geom->x) || (point->root_y <= geom->y)))
        {
            values[0] = point->root_x - geom->x - BORDER_WIDTH;
            values[1] = point->root_y - geom->y - BORDER_WIDTH;
            if ((values[0] >= (uint32_t)(WINDOW_MIN_HEIGHT))
             && (values[1] >= (uint32_t)(WINDOW_MIN_WIDTH)))
            {
                xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_WIDTH 
                                             | XCB_CONFIG_WINDOW_HEIGHT, values);
            }
        }
    }
}

static void handle_button_press(xcb_generic_event_t *event)
{
    xcb_button_press_event_t *e = (xcb_button_press_event_t *) event;
    win = e->child;
    /* Raise the clicked window */
    values[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
    /* set values[2] as 1 if lmb is pressed, 3 if event sent us a window, and 0 if it didn't */
    values[2] = ((e->detail) ? 1 : ((win != 0) ? 3 : 0));
    xcb_grab_pointer(dpy, 0, scre->root, 
            XCB_EVENT_MASK_BUTTON_RELEASE
            | XCB_EVENT_MASK_BUTTON_MOTION
            | XCB_EVENT_MASK_POINTER_MOTION_HINT,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
            scre->root, XCB_NONE, XCB_CURRENT_TIME);
}

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
    xcb_grab_button(dpy, 0, scre->root,  
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,  
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,  
                    scre->root,  
                    XCB_NONE,  
                    1, modkey);  

    xcb_grab_button(dpy, 0, scre->root,  
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,  
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,  
                    scre->root,  
                    XCB_NONE,  
                    3, modkey);  
    xcb_flush(dpy);

    /* Now we start the loop to start dealing with all these windows and such */
    while (ret == 0) ret = event_handler();

    return ret;
}
