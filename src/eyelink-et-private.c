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
    ET_FAIL,
    ET_CONNECT,
    ET_DISCONNECT,
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
        InputEvent      event;
        guint           num_calpoints;
    } content;
} ThreadMsg;

static void
et_send_message(GEyeEyelinkEt* self, ThreadMsg* msg) {
    g_async_queue_push(self->instance_to_thread, msg);
}

/*
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
*/

/*
 * Helpers for to communicate the signals to the main thread.
 */

typedef struct connect_info {
    GEyeEyetracker *self;
    gboolean        connected;
}connect_info;

static connect_info*
connect_info_create(GEyeEyetracker* self, gboolean connected)
{
    struct connect_info *ret = g_new0(connect_info, 1);
    ret->self = g_object_ref(self);
    ret->connected = connected;
    return ret;
}

static void
connect_info_free (gpointer data) {
    struct connect_info* info = data;
    g_object_unref(info->self);
    g_free(info);
}

typedef struct calpoint_info {
    GEyeEyetracker *self;
    gdouble x, y;   // the coordinate for calpoint start
} calpoint_info;

static calpoint_info*
calpoint_info_create (GEyeEyetracker* et, gdouble x, gdouble y)
{
    calpoint_info *info = g_new0(calpoint_info, 1);
    info->self = g_object_ref(et);
    info->x = x;
    info->y = y;
    return info;
}

static void
calpoint_info_free(gpointer data)
{
    calpoint_info *info = data;
    g_object_unref(info->self);
    g_free(info);
}

typedef struct error_info {
    GEyeEyetracker *et;
    char * error_message;
} error_info;

static error_info*
error_info_create_vsnprintf(GEyeEyelinkEt* et, const char* format, va_list args)
{
    error_info* info = g_new0(error_info, 1);
    info->et = g_object_ref(et);
    info->error_message = g_strdup_vprintf(format, args);
    return info;
}

static void
error_info_free(gpointer data)
{
    error_info* info = data;
    g_free(info->error_message);
    g_object_unref(info->et);
    g_free(info);
}

static gint
emit_connected(gpointer data)
{
    struct connect_info* info = data;
    g_assert(g_main_context_is_owner(
            GEYE_EYELINK_ET(info->self)->main_context));

    g_signal_emit_by_name(
            info->self,
            "connected", info->connected);
    return G_SOURCE_REMOVE;
}

static gint
emit_calpoint_start(gpointer data)
{
    calpoint_info *info = data;
    g_assert(g_main_context_is_owner(
            GEYE_EYELINK_ET(info->self)->main_context));

    g_signal_emit_by_name(info->self,
                          "calpoint-start",
                          info->x, info->y
                          );
    return G_SOURCE_REMOVE;
}

static gint
emit_calpoint_stop(gpointer data)
{
    calpoint_info *info = data;
    g_assert(g_main_context_is_owner(
            GEYE_EYELINK_ET(info->self)->main_context));

    g_signal_emit_by_name(info->self,
                          "calpoint-stop");
    return G_SOURCE_REMOVE;
}

static gint
emit_error(gpointer data)
{
    error_info *info = data;
    g_assert(g_main_context_is_owner(
            GEYE_EYELINK_ET(info->et)->main_context));

    g_signal_emit_by_name(info->et, "error", info->error_message);
    return G_SOURCE_REMOVE;
}

static void
et_signal_error_printf(GEyeEyelinkEt* self, const gchar* format, ...)
{
    if (self->main_context) {
        va_list args;
        va_start(args, format);
        error_info *info = error_info_create_vsnprintf(self, format, args);
        va_end(args);
        g_main_context_invoke_full(
                self->main_context,
                G_PRIORITY_DEFAULT,
                emit_error,
                info,
                error_info_free
        );
    }
}

static void
et_connect(GEyeEyelinkEt* self) {

    int ret;
    g_rec_mutex_lock(&self->lock);

    if (self->ip_address) {
        // this initializes the library.
        ret = open_eyelink_connection(-1);
        if (ret) {
            g_warning("Unable to initialize the library");
            et_signal_error_printf(
                    self,
                    "unable to initialize the eyelink lib"
                    "open_eyelink_connection(-1) returned %d",
                    ret);
        }

        ret = set_eyelink_address(self->ip_address);
        if (ret) {
            g_warning("Eyelink-Thread:Invalid ip address: \"%s\".",
                      self->ip_address);
            et_signal_error_printf(
                    self,
                    "Unable to set eyelink ip address to\"%s\", "
                    "set_eyelink_address returned %d",
                    self->ip_address,
                    ret);
        }
    }

    if (self->simulated)
        ret = open_eyelink_connection(1);
    else
        ret = eyelink_open();

    if (ret != OK_RESULT) {
        const char* msg = NULL;
        switch (ret) {
            case LINK_INITIALIZE_FAILED:
                msg = "a link could not be established";
                break;
            case CONNECT_TIMEOUT_FAILED:
                msg = "timeout, no tracker responded";
                break;
            case WRONG_LINK_VERSION:
                msg = "the version of the EyeLink library and tracker are incompatible";
                break;
            case OK_RESULT:
                g_assert_not_reached();
            default:
                break;
        }
        if (msg) {
            et_signal_error_printf(
                    self,
                    "Unable to open eyelink connection: %s",
                    msg);
        } else {
            et_signal_error_printf(
                    self,"Unable to open eyelink connection, "
                         "open_eyelink_connection() failed with %d",
                         ret
                    );
        }
    }

    self->connected = ret == 0;

    if (self->connected) {
        char eyelink_version_software[256];
        int type = eyelink_get_tracker_version(eyelink_version_software);
        char * tracker_info = g_strdup_printf(
                "EyeLink %d version %s",
                type,
                eyelink_version_software);
        if (self->info)
            g_free(self->info);
        self->info = tracker_info;
    }

    g_rec_mutex_unlock(&self->lock);

    if (self->main_context) {
        connect_info *connected = connect_info_create(
            GEYE_EYETRACKER(self), ret == 0);

        g_main_context_invoke_full(
            self->main_context,
            G_PRIORITY_DEFAULT,
            emit_connected,
            connected,
            connect_info_free
        );
    }
}

static void
et_disconnect(GEyeEyelinkEt* self)
{
    g_rec_mutex_lock(&self->lock);

    close_eyelink_connection();

    self->connected = FALSE;
    g_free(self->info);
    self->info = NULL;

    g_rec_mutex_unlock(&self->lock);

    if (self->main_context) {
        struct connect_info *connected = g_malloc0(sizeof(gboolean));
        connected->connected = self->connected;
        connected->self = g_object_ref(self);

        g_main_context_invoke_full(
            self->main_context,
            G_PRIORITY_DEFAULT,
            emit_connected,
            connected,
            connect_info_free
        );
    }
}

static void
et_start_tracking(GEyeEyelinkEt* self)
{
    int result;
    gint16 rec_samples = 0, rec_events = 0;

    g_rec_mutex_lock(&self->lock);

    if (self->recording)
        rec_samples = 1, rec_events =1;

    result = start_recording(rec_samples, rec_events, 1, 1);
    if (result != OK_RESULT) {
        g_critical("Unable to start tracking");
        et_signal_error_printf(
                self,
                "Unable to start tracking, start_recording returned %d",
                result
                );
    }
    else
        self->tracking = TRUE;

    g_rec_mutex_unlock(&self->lock);
}

static void
et_stop_tracking(GEyeEyelinkEt* self)
{
    int result;
    gint16 rec_samples = 0, rec_events = 0;

    g_rec_mutex_lock(&self->lock);

    if (self->recording)
        rec_samples = 1, rec_events =1;

    result = start_recording(rec_samples, rec_events, 0, 0);
    if (result != OK_RESULT)
        g_critical("Unable to stop tracking");
    else
        self->tracking = FALSE;

    g_rec_mutex_unlock(&self->lock);
}

static void
et_start_recording(GEyeEyelinkEt* self)
{
    gint ret;
    gint16 track_samples = 0, track_events = 0;

    g_rec_mutex_lock(&self->lock);
    if (self->tracking)
        track_samples = 1, track_events = 1;

    ret = start_recording(1, 1, track_samples, track_events);
    if (ret != OK_RESULT)
        g_critical("Unable to start recording");
    else
        self->recording = TRUE;

    g_rec_mutex_unlock(&self->lock);
}

static void
et_stop_recording(GEyeEyelinkEt* self)
{
    gint ret;
    gint16 track_samples = 0, track_events = 0;

    g_rec_mutex_lock(&self->lock);

    if (self->tracking)
        track_samples = 1, track_events = 1;

    ret = start_recording(0, 0, track_samples, track_events);
    if (ret != OK_RESULT)
        g_critical("Unable to stop recording");
    else
        self->recording = TRUE;

    g_rec_mutex_unlock(&self->lock);
}

static void
et_start_setup(GEyeEyelinkEt* self)
{
    int result = eyelink_request_image(ELIMAGE_128HVX, 192*2, 160*2);
    if (result != OK_RESULT)
        g_critical("Unable to handle eyelink_request image.");
    self->quit_hooks = FALSE;
    eyelink_set_tracker_setup_default(0); // 1 = image, 0 = menu
    do_tracker_setup();
}

static gboolean
et_calibration_setup(GEyeEyelinkEt* self)
{
    int ret;

    g_rec_mutex_lock(&self->lock);

    guint ndots = self->num_calpoints;
    const char* screen_pixel_coords = "screen_pixel_coords = 0 0 %d %d";

    ret = eyecmd_printf(
            screen_pixel_coords,
            (int) self->disp_width, (int) self->disp_height
        );
    if (ret != OK_RESULT)
        g_critical("Unable to setup calibration display");

    if (ndots > 3)
        ret = eyecmd_printf("calibration_type = HV%d", ndots);
    else
        ret = eyecmd_printf("calibration_type = H%d", ndots);

    if (ret != OK_RESULT) {
        g_critical("Unable to setup calibration type ncaldots=%d", ndots);
        return FALSE;
    }

    g_rec_mutex_unlock(&self->lock);

    return ret == OK_RESULT;
}

// Used for calibration and validation.
static void
et_calibrate(GEyeEyelinkEt* self)
{
    et_calibration_setup(self);
    eyelink_set_tracker_setup_default(0); // 1 = image, 0 menu
    self->quit_hooks = FALSE;
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
        case ET_VALIDATE:
            et_calibrate(self);
            break;
        case ET_STOP_SETUP:
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
    (void) self;
    int eye;
    gboolean received_something = FALSE;
    int event_type;
    ALLD_DATA event;
    while ((event_type = eyelink_get_next_data(NULL)) != 0) {
        received_something = TRUE;
        eye = eyelink_eye_available();
        eyelink_get_double_data(&event);
        switch (event_type) {
            case SAMPLE_TYPE:
                if (eye == LEFT) {

                }
                else if (eye == RIGHT) {

                }
            default:
                ;
        }
    }
    return received_something;
}

/* *********** eyelink hookv2 functions ************ */

static gint16
eyelink_hook_setup_cal_display(void* data)
{
    g_print("Trace %s:%d\n", __func__ , __LINE__);
    GEyeEyelinkEt *self = data;
    GEyeEyetracker *et = data;

    g_rec_mutex_lock(&self->lock);

    if (self->cb_start_calibration) {
        self->cb_start_calibration(
                et, self->cb_start_calibration_data
                );
    }

    g_rec_mutex_unlock(&self->lock);

    return 0;
}

static gint16
eyelink_hook_clear_cal_display(void* data)
{
    g_print("Trace %s:%d\n", __func__ , __LINE__);
    GEyeEyelinkEt *self = data;

    g_rec_mutex_lock(&self->lock);

    if (self->cb_stop_calibration) {
        self->cb_stop_calibration (
                GEYE_EYETRACKER(self), self->cb_stop_calibration_data
        );
    }

    g_rec_mutex_unlock(&self->lock);

    return 0;
}

static gint16
eyelink_hook_draw_cal_target(void *data, float x, float y)
{
    g_print("Trace %s:%d\n", __func__ , __LINE__);
    GEyeEyelinkEt *self = data;
    GEyeEyetracker *et = data;

    g_rec_mutex_lock(&self->lock);

    if (self->cb_calpoint_start) {
        self->cb_calpoint_start(
                et, x, y, self->cb_calpoint_start_data
        );
    }

    if (self->main_context) {
        calpoint_info * info = calpoint_info_create(GEYE_EYETRACKER(self), x, y);

        g_main_context_invoke_full(self->main_context,
                                   G_PRIORITY_DEFAULT,
                                   emit_calpoint_start,
                                   info,
                                   calpoint_info_free);
    }

    g_rec_mutex_unlock(&self->lock);

    return 0;
}

static gint16
eyelink_hook_erase_cal_target(void *data)
{
    g_print("Trace %s:%d\n", __func__ , __LINE__);
    GEyeEyelinkEt *self = data;
    GEyeEyetracker *et = data;

    g_rec_mutex_lock(&self->lock);

    if (self->cb_calpoint_stop) {
        self->cb_calpoint_stop(
                et, self->cb_calpoint_stop_data
        );
    }

    if (self->main_context) {
        calpoint_info *info = calpoint_info_create(GEYE_EYETRACKER(self), 0, 0);

        g_main_context_invoke_full(self->main_context,
                                   G_PRIORITY_DEFAULT,
                                   emit_calpoint_stop,
                                   info,
                                   calpoint_info_free);
    }

    g_rec_mutex_unlock(&self->lock);

    return 0;
}

static gint16
eyelink_hook_setup_image_display(gpointer data, gint16 width, gint16 height)
{
    GEyeEyelinkEt* et = data;
    gsize imbufsz = width * height * EYELINK_PIXEL_SIZE;
    g_print("Setup image display %d %d %lu\n", width, height, imbufsz);
    eyelink_thread_setup_image_data(et, imbufsz);
    return 1;
}

static gint16
eyelink_hook_clear_image_display(gpointer data)
{
    g_print("Trace %s:%d\n", __func__ , __LINE__);
    GEyeEyelinkEt *et = data;
    eyelink_thread_clear_image_data(et);
    return 0;
}

gint16
eyelink_hook_draw_image(
        void* data, gint16 width, gint16 height, guint8* bytes
        )
{
    GEyeEyelinkEt *self = data;

    g_rec_mutex_lock(&self->lock);

    if (self->cb_image_data) {
        guint w = width;
        guint h = height;
        gsize reqsz = w * h * 4;
        if (reqsz > self->image_size)
            eyelink_thread_setup_image_data(self, reqsz);
        guint8* src, *dest;
        // Change from Eyelink RGBA to Cairo ARGB32
        for (src = bytes, dest = self->image_data;
             src < bytes + reqsz && dest < self->image_data + self->image_size;
             src+=4, dest+=4) {
            //guint8 array [4] = {src[0], src[1], src[2], src[3]};
            //g_print("array = %2.2x  %2.2x %2.2x %2.2x\n",
            //        array[1], array[2], array[3], array[4]);
            dest[0] = src[2];
            dest[1] = src[1];
            dest[2] = src[0];
            dest[3] = 255; //src[4];
        }
        self->cb_image_data(GEYE_EYETRACKER(self),
                            width,
                            height,
                            reqsz,
                            self->image_data,
                            self->cb_image_data_data);
    }

    g_rec_mutex_unlock(&self->lock);

    return 0;
}

static gint16
eyelink_hook_input_key(void* data, InputEvent* key_input)
{
    (void) key_input;
    int result;
    GEyeEyelinkEt *self = data;

    if (self->quit_hooks == TRUE) {
         result = eyelink_send_keybutton(
                ESC_KEY,
                0,
                KB_PRESS
        );
        g_assert(result == OK_RESULT);

        return 0;
    }

    result = eyelink_cal_result();
    char calmsg[256];
    if (result != NO_REPLY) {
        exit_calibration();
        eyelink_cal_message(calmsg);
    }

    ThreadMsg *msg = g_async_queue_try_pop(self->instance_to_thread);
    if (msg) {
        ThreadMsgType type = msg->type;
        switch(type) {
            case ET_STOP_SETUP:
                self->quit_hooks = TRUE;
                break;
            case ET_SETUP_KEY:
                result = eyelink_send_keybutton(
                        msg->content.event.key.key,
                        msg->content.event.key.modifier,
                        KB_PRESS
                        );
                g_assert(result == OK_RESULT);
//                g_print("KEY_INPUT_EVENT %4x %4x\n",
//                        msg->content.event.key.key,
//                        msg->content.event.key.modifier);
                break;
            case ET_STOP:
            case ET_CALIBRATE:
            case ET_VALIDATE:
            case ET_START_SETUP:
            case ET_START_RECORDING:
            case ET_START_TRACKING:
            case ET_DISCONNECT:
                /* These messages are not meaningful in setup mode, hence quit
                 * setup and let the regular handler handle them.
                 */
                //exit_calibration();
                g_async_queue_push_front(self->instance_to_thread, msg);
                msg = NULL;
                self->quit_hooks = TRUE;
                break;
            default:
                g_assert_not_reached();
        }

        g_free(msg);
    }

    return 0;
}


/* ************** thread entry point ************** */

gpointer
eyelink_thread(gpointer data)
{
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
        .exit_image_display_hook = eyelink_hook_clear_image_display,

        // send keys to eyelink.
        .get_input_key_hook = eyelink_hook_input_key
    };

    int ret = setup_graphic_hook_functions_V2(&hooks2);
    if (ret) {
        g_critical("Unable to setup hook functions");
    }

    while (!self->stop_thread) {
        gboolean didsomething = FALSE;

        g_rec_mutex_lock(&self->lock);
        if (self->tracking) {
            gboolean received_event;
            received_event = handle_events(self);
            if (received_event)
                didsomething = TRUE;
        }
        g_rec_mutex_unlock(&self->lock);

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
    (void) error;
    ThreadMsg* msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_CONNECT;
    et_send_message(self, msg);
}

void
eyelink_thread_disconnect(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;
    g_rec_mutex_lock(&self->lock);
    if (self->connected){
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_DISCONNECT;
        et_send_message(self, msg);
    }
    g_rec_mutex_unlock(&self->lock);
}

void eyelink_thread_start_tracking(GEyeEyelinkEt* self, GError** error)
{
    ThreadMsg* msg;

    g_rec_mutex_lock(&self->lock);

    if (self->connected) {
        g_set_error(error,
                    geye_eyetracker_error_quark(),
                    GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                    "The eyelink must be connected.");
    }
    else {
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_START_TRACKING;
        et_send_message(self, msg);
    }

    g_rec_mutex_unlock(&self->lock);
}

void eyelink_thread_stop_tracking(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;

    g_rec_mutex_lock(&self->lock);

    if (self->connected) {
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_STOP_TRACKING;
        et_send_message(self, msg);
    }

    g_rec_mutex_unlock(&self->lock);
}

void eyelink_thread_start_recording(GEyeEyelinkEt* self, GError** error)
{
    ThreadMsg* msg;

    g_rec_mutex_lock(&self->lock);

    if (!self->connected) {
        g_set_error(error,
                    geye_eyetracker_error_quark(),
                    GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                    "The eyelink must be connected.");
    }
    else {
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_START_RECORDING;
        et_send_message(self, msg);
    }

    g_rec_mutex_unlock(&self->lock);
}

void eyelink_thread_stop_recording(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;

    g_rec_mutex_lock(&self->lock);

    if (self->connected) {
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_STOP_RECORDING;
        et_send_message(self, msg);
    }

    g_rec_mutex_unlock(&self->lock);
}

void eyelink_thread_start_setup(GEyeEyelinkEt* self)
{
    ThreadMsg* msg;

    g_rec_mutex_lock(&self->lock);

    if (self->connected) {
        /* make the thread enter setup mode */
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_START_SETUP;
        et_send_message(self, msg);

        /* Once the thread is in setup fetch images */
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_SETUP_KEY;
        msg->content.event.key.key = ENTER_KEY;
        msg->content.event.key.state = KB_PRESS;
        et_send_message(self, msg);
    }
    else {
        g_warning("Starting setup while not being connected.");
    }

    g_rec_mutex_unlock(&self->lock);
}

void
eyelink_thread_stop_setup(GEyeEyelinkEt* self)
{
    ThreadMsg *msg = g_malloc0(sizeof(ThreadMsg));
    msg->type = ET_STOP_SETUP;
    et_send_message(self, msg);
}

void
eyelink_thread_calibrate(GEyeEyelinkEt* self, GError **error)
{
    ThreadMsg *msg;

    g_rec_mutex_lock(&self->lock);

    if (!self->connected)
        g_set_error(
                error,
                geye_eyetracker_error_quark(),
                GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                "The eyelink is not connected");
    else {
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_CALIBRATE;
        et_send_message(self, msg);
        eyelink_thread_send_key_press(self, 'c', 0);
    }

    g_rec_mutex_unlock(&self->lock);
}

void
eyelink_thread_validate(GEyeEyelinkEt* self, GError **error)
{
    ThreadMsg *msg;

    g_rec_mutex_lock(&self->lock);
    if (!self->connected)
        g_set_error(
                error,
                geye_eyetracker_error_quark(),
                GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                "The eyelink is not connected");
    else {
        msg = g_malloc0(sizeof(ThreadMsg));
        msg->type = ET_VALIDATE;
        et_send_message(self, msg);
        eyelink_thread_send_key_press(self, 'v', 0);
    }
    g_rec_mutex_unlock(&self->lock);
}

void
eyelink_thread_set_image_data_cb(GEyeEyelinkEt       *self,
                                 geye_image_data_func cb,
                                 gpointer             data)
{
    g_rec_mutex_lock(&self->lock);
    self->cb_image_data = cb;
    self->cb_image_data_data = data;
    g_rec_mutex_unlock(&self->lock);
}

gboolean
eyelink_thread_send_key_press(GEyeEyelinkEt * self, guint16 key, guint modifiers)
{
    gboolean ret = FALSE;

    g_rec_mutex_lock(&self->lock);

    if (self->connected) {
        ThreadMsg *msg = g_malloc0(sizeof(ThreadMsg));
        guint16 tkey = key; // translated key

        // translate GDK key to what the eyelink seems to understand.
        // see gdk/gdkkeysyms.h
        // I could include that, but that makes gdk a dependency.
        if (key >= 0xffbe && key <= 0xffc9) {// GDK keyvalues for F1 - F12
            int fkeyval = key - 0xffbe;
            tkey = (F1_KEY >> 8) + fkeyval;
            tkey = tkey << 8;
        }

        if (key == 0xff1b ||
            key == 0xff0d) // override enter and escape
            tkey = key - 0xff00;

        msg->type = ET_SETUP_KEY;
        msg->content.event.key.key = tkey;
        msg->content.event.key.modifier = modifiers;
        msg->content.event.key.state = KB_PRESS;

        g_async_queue_push(self->instance_to_thread, msg);
        ret = TRUE;
    }
    g_rec_mutex_unlock(&self->lock);

    return ret;
}

void
eyelink_thread_setup_image_data (
        GEyeEyelinkEt* self, gsize img_size
        )
{
    g_rec_mutex_lock(&self->lock);

    if (self->image_data){
        g_free(self->image_data);
    }
    self->image_data = g_malloc(img_size);
    self->image_size = img_size;
    g_assert(self->image_data);

    g_rec_mutex_unlock(&self->lock);
}

void
eyelink_thread_clear_image_data(GEyeEyelinkEt* self)
{
    g_rec_mutex_lock(&self->lock);
    g_free(self->image_data);
    self->image_data = 0;
    g_rec_mutex_unlock(&self->lock);
}
