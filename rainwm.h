/* Event Handling */

typedef struct 
{
    uint32_t request;
    void (*func)(xcb_generic_event_t *event);
} handler_func_t;

static int event_handler(void);
//list of the handlers, in the following form:
static void handle_motion_notify(xcb_generic_event_t *event);
static void handle_enter_notify(xcb_generic_event_t *event);
static void handle_destroy_notify(xcb_generic_event_t *event);
static void handle_button_press(xcb_generic_event_t *event);
static void handle_button_release(xcb_generic_event_t *event);
static void handle_map_request(xcb_generic_event_t *event);
static void handle_focus_in(xcb_generic_event_t *event);
static void handle_focus_out(xcb_generic_event_t *event);

// make sure to also register handler functions here.
// this pairs your function with an event type.
static handler_func_t handler_functions[] = {
//    Event Type            Handler Function
    { XCB_MOTION_NOTIFY,    handle_motion_notify  },
    { XCB_ENTER_NOTIFY,     handle_enter_notify   },
    { XCB_DESTROY_NOTIFY,   handle_destroy_notify },
    { XCB_BUTTON_PRESS,     handle_button_press   },
    { XCB_BUTTON_RELEASE,   handle_button_release },
    { XCB_MAP_REQUEST,      handle_map_request    },
    { XCB_FOCUS_IN,         handle_focus_in       },
    { XCB_FOCUS_OUT,        handle_focus_out      },
    // The following should always be the last element. Also, no other func entries can be NULL.
    { XCB_NONE,             NULL } 
};
