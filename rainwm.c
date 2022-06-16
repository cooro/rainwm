#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <inttypes.h>
#include <xcb/xcb.h>
//#include <xcb/xcb_keysyms.h>

#define AUTHOR "Rayne Blake"
#define VERSION "0.1"

static xcb_connection_t *dpy;
static xcb_screen_t *scre;
static xcb_drawable_t win;

static uint32_t values[3];

/* String comparison. Return 0 if equal, else return nonzero int */
static int strcmp(char *str1, char *str2)
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

int main(int argc, char *argv[])
{
    int ret = 0;

    /* If crwm -v, print the version info */
    if (( argc == 2) && (strcmp("-v", argv[1]) == 0))
    {
        ret = 1;
        printf("rainwm-%s, Copyright 2022 %s, GPLv3\n", VERSION, AUTHOR);
    }
    /* If any parameters other than the above, print usage info */
    if ((ret == 0) && (argc != 1))
    {
        ret = 2;
        printf("usage: rainwm [-v]\n");
    }
    /* If nothing has broken us out yet, connect to the X server */
    if (ret == 0)
    {
        dpy = xcb_connect(NULL, NULL);
        ret = xcb_connection_has_error(dpy);
        if (ret > 0) printf("xcb_connection_has_error\n");
    }
    /* If the connection didn't return an error, continue setting up everything */
    if (ret == 0)
    {
        scre = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;  // screen object
        values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT  // mask values to register as a window manager
	          | XCB_EVENT_MASK_STRUCTURE_NOTIFY
		  | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		  | XCB_EVENT_MASK_PROPERTY_CHANGE;
        xcb_change_window_attributes_checked(dpy, scre->root, XCB_CW_EVENT_MASK, values);
        xcb_ungrab_key(dpy, XCB_GRAB_ANY, scre->root, XCB_MOD_MASK_ANY);
        int key_table_size = sizeof(keys) / sizeof(*keys);
    }


    return ret;
}
