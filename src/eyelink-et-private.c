/*
 * A gobject style eyetracker library
 * Copyright (C) 2021  Maarten Duijndam
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "eyelink-et-private.h"
#include "eyetracker-error.h"
#include <EyeLink/core_expt.h>

static const char* EYELINK_THREAD_NAME = "Eyelink-thread";
static gsize       EYELINK_PIXEL_SIZE = 4; //RGBA


typedef enum {
    ET_STOP,
    ET_ACKNOWLEDGE,
    ET_FAIL,
    ET_CONNECT,
    ET_CONNECTED,
    ET_CONNECTED_ERROR,
    ET_DISCONNECT,
    ET_DISCONNECTED,
    ET_START_TRACKING,
    ET_STOP_TRACKING,
    ET_START_RECORDING,
    ET_STOP_RECORDING,
    ET_START_SETUP,
    ET_STOP_SETUP,
    ET_SETUP_KEY,
    ET_CALIBRATE,
    ET_VALIDATE
} ThreadMsgType;


typedef struct {
    ThreadMsgType type;
    union ThreadContent {
        InputEvent event;
        guint      num_calpoints;
    } content;
} ThreadMsg;

static void
et_send_message(GEyeEyelinkEt* self, ThreadMsg* msg) {
    g_async_queue_push(self->instance_to_thread, msg);
}

static ThreadMsg*
et_receive_reply(GEyeEyelinkEt* self)
{
    ThreadMsg* msg = g_async_queue_pop(self->thread_to_instance);
    return msg;
}

static ThreadMsg*
et_receive_reply_timeout(GEyeEyelinkEt* self, guint64 timeout_us)
{
    ThreadMsg* msg = g_async_queue_timeout_pop(
            self->thread_to_instance,
            timeout_us
            );
    return msg;
}
static void
et_reply(GEyeEyelinkEt* self, ThreadMsg* msg) {
    g_async_queue_push(self->thread_to_instance, msg);
}

static void
et_connect(GEyeEyelinkEt* self) {

    int ret, mode;

    if (self->ip_address) {
        // this initializes the library.
        ret = open_eyelink_connection(-1);
        if (ret)
            g_warning("Unable to initialize the library");


        ret = set_eyelink_address(self->ip_address);
        if (ret)
            g_warning("Eyelink-Thread:Invalid ip address: \"%s\".",
                      self->ip_address);
    }

    if (self->simulated)
        mode = 1;
    else
        mode = 0;

    if (open_eyelink_connection(mode)) {
        //g_warning("Unable to connect to the eyelink eyetracker");
        ThreadMsg* msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_CONNECTED_ERROR;
        et_reply(self, msg);
    }
    else {
        ThreadMsg* msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_CONNECTED;
        et_reply(self, msg);
    }
}

static void
et_disconnect(GEyeEyelinkEt* self)
{
    close_eyelink_connection();
    ThreadMsg* msg = g_malloc0(sizeof(ThreadMsgType));
    msg->type = ET_DISCONNECTED;
    et_reply(self, msg);
}

static void
et_start_tracking(GEyeEyelinkEt* self)
{
    int result;
    ThreadMsg *msg;
    gint16 rec_samples = 0, rec_events = 0;
    if (self->recording)
        rec_samples = 1, rec_events =1;

    result = start_recording(rec_samples, rec_events, 1, 1);
    msg = g_malloc0(sizeof(ThreadMsg));
    if (result != 0)
        msg->type = ET_FAIL;
    else
        msg->type = ET_ACKNOWLEDGE;

    et_reply(self, msg);
}

static void
et_stop_tracking(GEyeEyelinkEt* self)
{
    ThreadMsg *msg;
    int result;
    gint16 rec_samples = 0, rec_events = 0;
    if (self->recording)
        rec_samples = 1, rec_events =1;

    result = start_recording(rec_samples, rec_events, 0, 0);
    msg = g_malloc0(sizeof(ThreadMsg));
    if (result != 0)
        msg->type = ET_FAIL;
    else
        msg->type = ET_ACKNOWLEDGE;
    et_reply(self, msg);
}

static void
et_start_recording(GEyeEyelinkEt* self)
{
    ThreadMsg *msg;
    gint16 track_samples = 0, track_events = 0;
    if (self->tracking)
        track_samples = 1, track_events =1;

    start_recording(1, 1, track_samples, track_events);
    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_ACKNOWLEDGE;
    et_reply(self, msg);
}

static void
et_stop_recording(GEyeEyelinkEt* self)
{
    ThreadMsg *msg;
    gint16 track_samples = 0, track_events = 0;
    if (self->tracking)
        track_samples = 1, track_events = 1;

    start_recording(0, 0, track_samples, track_events);
    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_ACKNOWLEDGE;
    et_reply(self, msg);
}

static void
et_start_setup(GEyeEyelinkEt* self)
{
    (void) self;
    eyelink_set_tracker_setup_default(1); // 1 = image, 0 = menu
    int ret = eyelink_start_setup();
    g_assert(ret == 0);
    ret = eyelink_wait_for_mode_ready(100);
    g_assert(ret == 0);
    do_tracker_setup();
}

static void
et_calibrate(GEyeEyelinkEt* self, guint ncaldots)
{
    (void) self;
    int ret;
    if (ncaldots > 3)
        ret = eyecmd_printf("calibration_type = HV%d", ncaldots);
    else
        ret = eyecmd_printf("calibration_type = H%d", ncaldots);

    if (ret != OK_RESULT)
        g_critical("Unable to setup calibration type ncaldots=%d", ncaldots);

    eyelink_set_tracker_setup_default(0); // 1 = image, 0 menu
    eyelink_send_keybutton('c', 0, KB_PRESS);
    do_tracker_setup();
}

static void
et_validate(GEyeEyelinkEt* self, guint ncaldots)
{
    (void) self;
    int ret;
    if (ncaldots > 3)
        ret = eyecmd_printf("calibration_type = HV%d", ncaldots);
    else
        ret = eyecmd_printf("calibration_type = H%d", ncaldots);

    if (ret != OK_RESULT)
        g_critical("Unable to setup calibration type ncaldots=%d", ncaldots);

    eyelink_set_tracker_setup_default(0); // 1 = image, 0 menu
    eyelink_send_keybutton('v', 0, KB_PRESS);
    do_tracker_setup();
}



static void
handle_msg(GEyeEyelinkEt* self, ThreadMsg* msg)
{
    ThreadMsgType type = msg->type;
    switch(type) {
        case ET_STOP:
            self->stop_thread = TRUE;
            if (self->connected)
                et_disconnect(self);
            break;
        case ET_CONNECT:
            et_connect(self);
            break;
        case ET_DISCONNECT:
            et_disconnect(self);
            break;
        case ET_START_TRACKING:
            et_start_tracking(self);
            break;
        case ET_STOP_TRACKING:
            et_stop_tracking(self);
            break;
        case ET_START_RECORDING:
            et_start_recording(self);
            break;
        case ET_STOP_RECORDING:
            et_stop_recording(self);
            break;
        case ET_START_SETUP:
            et_start_setup(self);
            break;
        case ET_CALIBRATE:
            et_calibrate(self, msg->content.num_calpoints);
            break;
        case ET_VALIDATE:
            et_validate(self, msg->content.num_calpoints);
            break;
        case ET_STOP_SETUP:
        case ET_ACKNOWLEDGE:
        case ET_CONNECTED:
        case ET_CONNECTED_ERROR:
        case ET_DISCONNECTED:
        default:
            g_warning("Unexpected message type %d", type);
    }
}

static gboolean
monitor_main_thread(GEyeEyelinkEt* self, gboolean sleep)
{
    ThreadMsg* msg;
    if (sleep) {
        g_assert(self->instance_to_thread);
        msg  = g_async_queue_timeout_pop(self->instance_to_thread, 1000);
    }
    else
        msg  = g_async_queue_try_pop(self->instance_to_thread);

    if (msg) {
        handle_msg(self, msg);
        g_free(msg);
        return TRUE;
    }
    return FALSE;
}

static gboolean
handle_events(GEyeEyelinkEt* self)
{
    int eye;
    gboolean received_something = FALSE;
    int event_type;
    ALLD_DATA event;
    while ((event_type = eyelink_get_next_data(NULL)) != 0) {
        received_something = TRUE;
        eye = eyelink_eye_available();
        eyelink_get_double_data(&event);
        g_print("We've gotten an event %d\n", event_type);
        switch (event_type) {
            case SAMPLE_TYPE:
                if (eye == LEFT) {

                }
                else if (eye == RIGHT) {

                }
            
        }
    }
    return received_something;
}

/* *********** eyelink hookv2 functions ************ */

static gint16
eyelink_hook_setup_cal_display(void* data)
{
    g_print("in setup cal display\n");
    GEyeEyelinkEt *self = data;
    GEyeEyetracker *et = data;
    if (self->cb_start_calibration) {
        self->cb_start_calibration(
                et, self->cb_start_calibration_data
                );
    }
    return 0;
}

static gint16
eyelink_hook_clear_cal_display(void* data)
{
    GEyeEyelinkEt *self = data;
    GEyeEyetracker *et = data;
    if (self->cb_stop_calibration) {
        self->cb_stop_calibration(
                et, self->cb_stop_calibration_data
        );
    }
    return 0;
}

static gint16
eyelink_hook_draw_cal_target(void *data, float x, float y)
{
    GEyeEyelinkEt *self = data;
    GEyeEyetracker *et = data;
    if (self->cb_calpoint_start) {
        self->cb_calpoint_start(
                et, x, y, self->cb_calpoint_start_data
        );
    }
}

static gint16
eyelink_hook_erase_cal_target(void *data)
{
    GEyeEyelinkEt *self = data;
    GEyeEyetracker *et = data;
    if (self->cb_calpoint_start) {
        self->cb_calpoint_stop(
                et, self->cb_calpoint_start_data
        );
    }
    return 0;
}

static gint16
eyelink_hook_setup_image_display(gpointer data, gint16 width, gint16 height)
{
    gsize imbufsz = width * height * EYELINK_PIXEL_SIZE;
    g_print("Setup image display %d %d %lu", width, height, imbufsz);
    return 0;
}

gint16 eyelink_hook_draw_image(
                void* data, gint16 width, gint16 height, guint8* bytes
        )
{
    int w = width;
    int h = height;
    g_print("in draw image %p, %d, %d %p\n", data, w, h, bytes);
    //exit_calibration();
    return 0;
}


static gint16
eyelink_hook_input_key(void* data, InputEvent* key_input)
{
    GEyeEyelinkEt *self = data;

    ThreadMsg *msg = g_async_queue_try_pop(self->tracker_setup_queue);
    if (msg) {
        switch(msg->type) {
            case ET_STOP_SETUP:
                exit_calibration();
                break;
            case ET_SETUP_KEY:
                *key_input = msg->content.event;
                break;
            default:
                g_assert_not_reached();
        }

        g_print("KEY_INPUT_EVENT %4x %4x\n",
                key_input->key.key, key_input->key.modifier);
        g_free(msg);
    }


    return 0;
}


/* ************** thread entry point ************** */

gpointer eyelink_thread(gpointer data) {
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(data);

    HOOKFCNS2 hooks2 = {
        .major = 1,
        .userData = self,
        // calibration / validation
        .setup_cal_display_hook = eyelink_hook_setup_cal_display,
        .clear_cal_display_hook = eyelink_hook_clear_cal_display,
        .draw_cal_target_hook = eyelink_hook_draw_cal_target,
        .erase_cal_target_hook = eyelink_hook_erase_cal_target,
        // setup
        .setup_image_display_hook = eyelink_hook_setup_image_display,
        .image_title_hook = NULL,
        .draw_image = eyelink_hook_draw_image,
        .exit_image_display_hook = NULL,

        // send keys to eyelink.
        .get_input_key_hook = eyelink_hook_input_key
    };

    int ret = setup_graphic_hook_functions_V2(&hooks2);
    if (ret) {
        g_critical("Unable to setup hook functions");
    }

    while (!self->stop_thread) {
        gboolean didsomething = FALSE;

        if (self->tracking) {
            gboolean received_event;
            received_event = handle_events(self);
            if (received_event)
                didsomething = TRUE;
        }

        if (didsomething)
            monitor_main_thread(self, FALSE);
        else
            monitor_main_thread(self, TRUE);
    }

    return NULL;
}

GThread*
eyelink_thread_start(GEyeEyelinkEt* self)
{
    GThread* thread = NULL;
    thread = g_thread_new(EYELINK_THREAD_NAME, eyelink_thread, self);
    return thread;
}

void
eyelink_thread_stop(GEyeEyelinkEt* self)
{
    ThreadMsg* msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_STOP;
    et_send_message(self, msg);
    g_thread_join(self->eyelink_thread);
}

void
eyelink_thread_connect(GEyeEyelinkEt* self, GError** error)
{
    ThreadMsg* msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_CONNECT;
    et_send_message(self, msg);
    msg = et_receive_reply(self);
    if (msg->type != ET_CONNECTED) {
        g_set_error(
                error,
                GEYE_EYETRACKER_ERROR,
                GEYE_EYETRACKER_ERROR_UNABLE_TO_CONNECT,
                "The eyelink thread was not able to connect to the eyetracker."
                );
        g_free(msg);
        return;
    }
    self->connected = TRUE;
}

void
eyelink_thread_disconnect(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;
    if (!self->connected)
        return;

    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_DISCONNECT;
    et_send_message(self, msg);
    msg = et_receive_reply_timeout(self, (guint64) 1e6);
    if (!msg || msg->type != ET_DISCONNECTED) {
        g_warning("Unable to disconnect the eyelink eyetracker");
    }
    else {
        self->connected = FALSE;
    }
    g_free(msg);
}

void eyelink_thread_start_tracking(GEyeEyelinkEt* self, GError** error)
{
    ThreadMsg* msg;
    if (!self->connected) {
        g_set_error(error,
                    geye_eyetracker_error_quark(),
                    GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                    "The eyelink must be connected.");
    }
    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_START_TRACKING;
    et_send_message(self, msg);
    msg = et_receive_reply(self);
    if (msg->type != ET_ACKNOWLEDGE)
        g_warning("The eyelink thread didn't acknowledge start tracking");
    self->tracking = TRUE;
    g_free(msg);
}

void eyelink_thread_stop_tracking(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;
    if (!self->connected)
        return;

    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_STOP_TRACKING;
    et_send_message(self, msg);
    msg = et_receive_reply(self);
    if (msg->type != ET_ACKNOWLEDGE)
        g_warning("The eyelink thread didn't acknowledge stop tracking");
    self->tracking = FALSE;
    g_free(msg);
}

void eyelink_thread_start_recording(GEyeEyelinkEt* self, GError** error)
{
    ThreadMsg* msg;
    if (!self->connected) {
        g_set_error(error,
                    geye_eyetracker_error_quark(),
                    GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                    "The eyelink must be connected.");
    }
    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_START_RECORDING;
    et_send_message(self, msg);
    msg = et_receive_reply(self);
    if (msg->type != ET_ACKNOWLEDGE)
        g_warning("The eyelink thread didn't acknowledge start recording");
    self->recording = TRUE;
    g_free(msg);
}

void eyelink_thread_stop_recording(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;
    if (!self->connected)
        return;

    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_STOP_RECORDING;
    et_send_message(self, msg);
    msg = et_receive_reply(self);
    if (msg->type != ET_ACKNOWLEDGE)
        g_warning("The eyelink thread didn't acknowledge stop recording");
    self->recording = FALSE;
    g_free(msg);
}

void eyelink_thread_start_setup(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;
    if (!self->connected)
        return;

    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_START_SETUP;
    et_send_message(self, msg);
    // The eyetracker is blocking on do_tracker_setup();
    // so don't wait for reply.
}

void
eyelink_thread_stop_setup(GEyeEyelinkEt* self)
{
    ThreadMsg *msg;
    if (!self->connected)
        return;

    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_STOP_SETUP;
    g_async_queue_push(self->tracker_setup_queue, msg);
}

void
eyelink_thread_calibrate(GEyeEyelinkEt* self, GError **error)
{
    ThreadMsg *msg;
    if (!self->connected)
        g_set_error(
                error,
                geye_eyetracker_error_quark(),
                GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                "The eyelink is not connected");

    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_CALIBRATE;
    msg->content.num_calpoints = self->num_calpoints;
    et_send_message(self, msg);
}

void
eyelink_thread_validate(GEyeEyelinkEt* self, GError **error)
{
    ThreadMsg *msg;
    if (!self->connected)
        g_set_error(
                error,
                geye_eyetracker_error_quark(),
                GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                "The eyelink is not connected");

    msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_VALIDATE;
    msg->content.num_calpoints = self->num_calpoints;
    et_send_message(self, msg);
}
