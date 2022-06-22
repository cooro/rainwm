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

static int mod_key = XCB_MOD_MASK_1;

static int window_default_width  = 600;
static int window_default_height = 400;
static int window_min_width  = 60;
static int window_min_height = 40;

static int border_width = 5;
static int border_color_focused = 0xff0000;
static int border_color_normal  = 0x777777;


/* Handles dragging of windows. all mouse movement is sent here, but only
 * mod+click is actually captured at the moment. */
static void handle_motion_notify(xcb_generic_event_t *event)
{
    (void) event;
    xcb_query_pointer_cookie_t point_cookie = xcb_query_pointer(dpy, scre->root);
    xcb_query_pointer_reply_t *point = xcb_query_pointer_reply(dpy, point_cookie, 0);

    if ((values[2] == (uint32_t)(1)) && (win != 0))
    {
        xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(dpy, win);
        xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, geom_cookie, NULL);
        uint16_t geom_x = geom->width + (2 * border_width);
        uint16_t geom_y = geom->height + (2 * border_width);
        values[0] = ((point->root_x + geom_x) > scre->width_in_pixels) ?
	        (scre->width_in_pixels - geom_x) : point->root_x;
	values[1] = ((point->root_y + geom_y) > scre->height_in_pixels) ?
	        (scre->height_in_pixels - geom_y) : point->root_y;
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    } else if ((values[2] == (uint32_t)(3)) && (win != 0))
    { 
        xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(dpy, win);
        xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, geom_cookie, NULL);
        if (!((point->root_x <= geom->x) || (point->root_y <= geom->y)))
        {
            values[0] = point->root_x - geom->x - border_width;
            values[1] = point->root_y - geom->y - border_width;
            if ((values[0] >= (uint32_t)(window_min_height))
             && (values[1] >= (uint32_t)(window_min_width)))
            {
                xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_WIDTH 
                                             | XCB_CONFIG_WINDOW_HEIGHT, values);
            }
        }
    }
}

/* Handles mouse clicks. Due to config in setup(), only mod+click is captured. */
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

/* Handles mouse releases. Just unconditionally stops grabbing the pointer. */
static void handle_button_release(xcb_generic_event_t *event)
{
    (void) event;
    xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);
}

/* Does things when a pointer enters a window. */
static void handle_enter_notify(xcb_generic_event_t *event)
{
    xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *) event;
    set_focus(e->event);
}

/* Handles destruction requests. At the moment requests are granted 
 * unconditionally. */
static void handle_destroy_notify(xcb_generic_event_t *event)
{
    xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *) event;
    xcb_kill_client(dpy, e->window);
}

/* Performs actions when focusing on a window */
static void handle_focus_in(xcb_generic_event_t *event)
{
    xcb_focus_in_event_t *e = (xcb_focus_in_event_t *) event;
    set_border_color(e->event, border_color_focused);
}

/* Performs actions when focus leaves a window */
static void handle_focus_out(xcb_generic_event_t *event)
{
    xcb_focus_in_event_t *e = (xcb_focus_in_event_t *) event;
    set_border_color(e->event, border_color_normal);
}

/* Handles putting windows on the screen and decorating them when they spawn */
static void handle_map_request(xcb_generic_event_t *event)
{
    xcb_map_request_event_t *e = (xcb_map_request_event_t *) event;
    xcb_map_window(dpy, e->window);
    uint32_t vals[5];
    vals[0] = (scre->width_in_pixels / 2) - (window_default_width / 2);  // initial x pos
    vals[1] = (scre->height_in_pixels / 2) - (window_default_height / 2);  // initial y pos
    vals[2] = window_default_width;  // initial width
    vals[3] = window_default_height;  // initial height
    vals[4] = border_width;  // initial border width
    xcb_configure_window(dpy, e->window, 
              XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
            | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
            | XCB_CONFIG_WINDOW_BORDER_WIDTH, vals);
    xcb_flush(dpy);
    values[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
    xcb_change_window_attributes_checked(dpy, e->window, XCB_CW_EVENT_MASK, values);
    set_focus(e->window);
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

/* Sets focus on the passed in window */
static void set_focus(xcb_drawable_t window)
{
    if ((window == 0) || (window == scre->root)) return;
    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, window, 
            XCB_CURRENT_TIME);
    printf("Focus set.\n");
}

/* Sets the border color for the passed in window */
static void set_border_color(xcb_window_t window, int color)
{
    if ((border_width <= 0) || (window == scre->root) 
            || (window == 0) || (color > 0xffffff)) return;
    uint32_t vals[1] = {color};
    xcb_change_window_attributes(dpy, window, XCB_CW_BORDER_PIXEL, vals); 
    printf("Border color set.\n");
    xcb_flush(dpy);
}

/* Decides which handler to send each event to. */
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
    printf("xcb_connect()\n");
    /* If nothing has broken us out yet, connect to the X server */
    dpy = xcb_connect(NULL, NULL);
    printf("xcb_connection_has_error\n");
    ret = xcb_connection_has_error(dpy);
    if (ret != 0)
    {
        printf("xcb_connection_has_error, main function\n");
        return ret;
    }

    printf("About to sign up for WM status\n");
    /* Set up the screen object, and sign up to be the window manager */
    scre = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT  // mask values needed to register as a window manager
              | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    	      | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
    	      | XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes_checked(dpy, scre->root, XCB_CW_EVENT_MASK, values);
    xcb_flush(dpy);

    printf("About to do mouse setup.\n");
    /* Register to grab the Mod+lmb and Mod+rmb events for mouse control of windows */
    xcb_grab_button(dpy, 0, scre->root,  
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,  
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,  
                    scre->root,  
                    XCB_NONE,  
                    1, mod_key);  

    xcb_grab_button(dpy, 0, scre->root,  
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,  
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,  
                    scre->root,  
                    XCB_NONE,  
                    3, mod_key);  
    xcb_flush(dpy);

    printf("Starting the event handler loop.\n");
    /* Now we start the loop to start dealing with all these windows and such */
    while (ret == 0) ret = event_handler();

    return ret;
}
