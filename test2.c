#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <xcb/xcb.h>


static xcb_connection_t *dpy;   /* X Server variable */
static xcb_screen_t *scre;      /* Points to the screen */
static xcb_drawable_t win;      /* Points to a window */
static uint32_t values[3];      /* The mask values variable */


static void killclient(char **com)
{
    (void) com; 
    xcb_kill_client(dpy, win);  // kill the focused window
}

static void closewm(char **com)
{
    (void) com;
    if (dpy != NULL) xcb_disconnect(dpy); // if connected to x server, disconnect
}

static void spawn(char **com)
{
    if (fork() == 0)
    {
        setsid();
        if (fork() != 0) _exit(0);
        execvp((char*)com[0], (char**)com);
    }
    wait(NULL);
}

static void fullclient(char **com)
{
    (void) com;
    uint32_t vals[4];
    vals[0] = 0 - BORDER_WIDTH;        /* set position x */ 
    vals[1] = 0 - BORDER_WIDTH;        /* set position y */ 
    vals[2] = scre->width_in_pixels;   /* set window wide to screen wide */ 
    vals[3] = scre->height_in_pixels;  /* set window tall to screen tall */ 
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X     | XCB_CONFIG_WINDOW_Y 
                                 | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, vals);
    xcb_flush(dpy);
}

static void handleButtonPress(xcb_generic_event_t *event)
{
    xcb_button_press_event_t *e = (xcb_button_press_event_t *) event;
    win = e->child;
    values[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
    values[2] = ((1 == e->detail) ? 1 : ((win != 0) ? 3 : 0 ));
    xcb_grab_pointer(dpy, 0, scre->root, XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION 
                                       | XCB_EVENT_MASK_POINTER_MOTION_HINT,
		                         XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
		                         scre->root, XCB_NONE, XCB_CURRENT_TIME);
}

static void handleMotionNotify(xcb_generic_event_t *event)
{
    (void) event;
    xcb_query_pointer_cookie_t coord = xcb_query_pointer(dpy, scre->root);
    xcb_query_pointer_reply_t *poin = xcb_query_pointer_reply(dpy, coord, 0);

    if ((values[2] == (uint32_t)(1)) && (win != 0))
    {
        xcb_get_geometry_cookie_t geom_now = xcb_get_geometry(dpy, win);
        xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, coord, 0);
        uint16_t geom_x = geom->width + (2*BORDER_WIDTH);
    }
}

int main (argc, *argv[])
{
    return 0;
}
