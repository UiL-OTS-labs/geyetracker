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

#include "eyelink-et.h"
#include "eyelink-et-private.h"
#include "eyetracker.h"
#include "eyetracker-error.h"

static void
geye_eyetracker_interface_init(GEyeEyetrackerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(GEyeEyelinkEt,
                        geye_eyelink_et,
                        G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GEYE_TYPE_EYETRACKER,
                                              geye_eyetracker_interface_init)
                        )
static void
geye_eyelink_et_init(GEyeEyelinkEt* self) {
    self->stop_thread           = FALSE;
    self->connected             = FALSE;
    self->simulated             = FALSE;
    self->recording             = FALSE;
    self->tracking              = FALSE;
    self->instance_to_thread    = g_async_queue_new_full(g_free);
    self->thread_to_instance    = g_async_queue_new_full(g_free);

    self->main_context          = g_main_context_ref_thread_default();

    g_rec_mutex_init(&self->lock);
    // keep this last, otherwise the queue might be NULL
    self->eyelink_thread        = eyelink_thread_start(self);
}

static void
eyelink_et_connect(GEyeEyetracker* self, GError** error)
{
    eyelink_thread_connect(GEYE_EYELINK_ET(self), error);
}

static void
eyelink_et_disconnect(GEyeEyetracker* self)
{
    eyelink_thread_disconnect(GEYE_EYELINK_ET(self));
}

static void
eyelink_et_start_tracking(GEyeEyetracker* self, GError** error)
{
    eyelink_thread_start_tracking(GEYE_EYELINK_ET(self), error);
}

static void
eyelink_et_stop_tracking(GEyeEyetracker* self)
{
    eyelink_thread_stop_tracking(GEYE_EYELINK_ET(self));
}

static void
eyelink_et_start_recording(GEyeEyetracker* self, GError** error)
{
    eyelink_thread_start_recording(GEYE_EYELINK_ET(self), error);
}

static void
eyelink_et_stop_recording(GEyeEyetracker* self)
{
    eyelink_thread_stop_recording(GEYE_EYELINK_ET(self));
}

static void
eyelink_et_start_setup(GEyeEyetracker* self)
{
    eyelink_thread_start_setup(GEYE_EYELINK_ET(self));
}

static void
eyelink_et_stop_setup(GEyeEyetracker* self)
{
    eyelink_thread_stop_setup(GEYE_EYELINK_ET(self));
}

static void
eyelink_et_calibrate(GEyeEyetracker* self, GError **error)
{
    eyelink_thread_calibrate(GEYE_EYELINK_ET(self), error);
}

static void
eyelink_et_validate(GEyeEyetracker* self, GError **error)
{
    eyelink_thread_validate(GEYE_EYELINK_ET(self), error);
}

static guint
eyelink_et_get_num_calpoints(GEyeEyetracker* et)
{
    GEyeEyelinkEt *self = GEYE_EYELINK_ET(et);
    return self->num_calpoints;
}

static void
eyelink_et_set_num_calpoints(GEyeEyetracker* et, guint n)
{
    GEyeEyelinkEt *self = GEYE_EYELINK_ET(et);
    // Eyelink API supports 3, 5, 9 and 13
    if (n <= 3)
        self->num_calpoints = 3;
    else if (n <= 5)
        self->num_calpoints = 5;
    else if (n <= 9)
        self->num_calpoints = 9;
    else
        self->num_calpoints = 13;
}

static void
eyelink_et_set_calibration_start_cb (GEyeEyetracker     *et,
                                     geye_start_cal_func cb,
                                     gpointer            data)
{
    GEyeEyelinkEt *self = GEYE_EYELINK_ET(et);
    self->cb_start_calibration = cb;
    self->cb_start_calibration_data = data;
}

static void
eyelink_et_set_calibration_stop_cb(GEyeEyetracker    *et,
                                   geye_stop_cal_func cb,
                                   gpointer           data)
{
    GEyeEyelinkEt *self = GEYE_EYELINK_ET(et);
    self->cb_stop_calibration = cb;
    self->cb_stop_calibration_data = data;
}

static void
eyelink_et_set_calpoint_start_cb(GEyeEyetracker          *et,
                                 geye_calpoint_start_func cb,
                                 gpointer                 data)
{
    GEyeEyelinkEt *self = GEYE_EYELINK_ET(et);
    self->cb_calpoint_start = cb;
    self->cb_calpoint_start_data = data;
}

static void
eyelink_et_set_calpoint_stop_cb(GEyeEyetracker          *et,
                                geye_calpoint_stop_func  cb,
                                gpointer                 data)
{
    GEyeEyelinkEt *self = GEYE_EYELINK_ET(et);
    self->cb_calpoint_stop = cb;
    self->cb_calpoint_stop_data = data;
}

static void
eyelink_et_set_image_data_cb(GEyeEyetracker* et,
                             geye_image_data_func cb,
                             gpointer data)
{
    eyelink_thread_set_image_data_cb(
            GEYE_EYELINK_ET(et), cb, data
            );
}

static gboolean
eyelink_et_send_key_press(GEyeEyetracker* et, guint16 key, guint modifiers)
{
    GEyeEyelinkEt *self = GEYE_EYELINK_ET(et);
    eyelink_thread_send_key_press(GEYE_EYELINK_ET(self), key, modifiers);
    return TRUE;
}

static void
geye_eyetracker_interface_init(GEyeEyetrackerInterface* iface)
{
    iface->connect          = eyelink_et_connect;
    iface->disconnect       = eyelink_et_disconnect;

    iface->start_tracking   = eyelink_et_start_tracking;
    iface->stop_tracking    = eyelink_et_stop_tracking;

    iface->start_recording  = eyelink_et_start_recording;
    iface->stop_recording   = eyelink_et_stop_recording;

    iface->start_setup      = eyelink_et_start_setup;
    iface->stop_setup       = eyelink_et_stop_setup;

    iface->calibrate        = eyelink_et_calibrate;
    iface->validate         = eyelink_et_validate;

    iface->get_num_calpoints= eyelink_et_get_num_calpoints;
    iface->set_num_calpoints= eyelink_et_set_num_calpoints;

    iface->set_calibration_start_cb = eyelink_et_set_calibration_start_cb;
    iface->set_calibration_stop_cb  = eyelink_et_set_calibration_stop_cb;
    iface->set_calpoint_start_cb    = eyelink_et_set_calpoint_start_cb;
    iface->set_calpoint_stop_cb     = eyelink_et_set_calpoint_stop_cb;

    iface->set_image_data_cb = eyelink_et_set_image_data_cb;

    iface->send_key_press   = eyelink_et_send_key_press;
}

static void
eyelink_et_dispose(GObject* gobject)
{
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(gobject);

    // stop eyelink thread
    if (self->eyelink_thread)
        eyelink_thread_stop(self);
    self->eyelink_thread = NULL;

    if (self->thread_to_instance) {
        g_async_queue_unref(self->thread_to_instance);
        self->thread_to_instance = NULL;
    }

    if (self->instance_to_thread) {
        g_async_queue_unref(self->instance_to_thread);
        self->instance_to_thread = NULL;
    }

    if (self->main_context) {
        g_main_context_unref(self->main_context);
        self->main_context = NULL;
    }

    G_OBJECT_CLASS(geye_eyelink_et_parent_class)->dispose(gobject);
}

static void
eyelink_et_finalize(GObject* gobject)
{
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(gobject);
    g_free(self->ip_address);
    g_rec_mutex_clear(&self->lock);

    G_OBJECT_CLASS(geye_eyelink_et_parent_class)->finalize(gobject);
}

typedef enum {
    PROP_NULL,
    PROP_SIMULATED,
    N_PROPERTIES,
    PROP_CONNECTED,
    PROP_TRACKING,
    PROP_RECORDING,
    PROP_NUM_CALPOINTS
}GEyeEyelinkEtProperty;

static GParamSpec* obj_properties[N_PROPERTIES] = {NULL, };

static void
geye_eyelink_et_set_property(GObject       *obj,
                             guint          property_id,
                             const GValue  *value,
                             GParamSpec    *pspec
                             )
{
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(obj);
    GEyeEyetracker* et = GEYE_EYETRACKER(self);
    (void) self;
    (void) value;

    switch((GEyeEyelinkEtProperty) property_id) {
        case PROP_NUM_CALPOINTS:
            geye_eyetracker_set_num_calpoints(et, g_value_get_uint(value));
            break;
        case PROP_SIMULATED:
        case PROP_CONNECTED:
        case PROP_NULL:
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
    }
}
static void
geye_eyelink_et_get_property(GObject       *obj,
                             guint          property_id,
                             GValue        *value,
                             GParamSpec    *pspec
                             )
{
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(obj);

    switch((GEyeEyelinkEtProperty) property_id) {
        case PROP_CONNECTED:
            g_value_set_boolean(value, self->connected);
            break;
        case PROP_SIMULATED:
            g_value_set_boolean(value, geye_eyelink_et_get_simulated(self));
            break;
        case PROP_RECORDING:
            g_value_set_boolean(value, self->recording);
            break;
        case PROP_TRACKING:
            g_value_set_boolean(value, self->tracking);
            break;
        case PROP_NUM_CALPOINTS:
            g_value_set_uint(value, self->num_calpoints);
            break;
        case PROP_NULL:
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
    }

}

static void
geye_eyelink_et_class_init(GEyeEyelinkEtClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = eyelink_et_dispose;
    object_class->finalize = eyelink_et_finalize;
    object_class->get_property = geye_eyelink_et_get_property;
    object_class->set_property = geye_eyelink_et_set_property;

    obj_properties[PROP_SIMULATED] = g_param_spec_boolean(
            "simulated",
            "Simulated",
            "If set to true connect to a simulated version of the eyelink",
            FALSE,
            G_PARAM_READABLE
            );

    g_object_class_install_properties(
            object_class, N_PROPERTIES, obj_properties
            );

    g_object_class_override_property(object_class, PROP_CONNECTED, "connected");
    g_object_class_override_property(object_class, PROP_TRACKING, "tracking");
    g_object_class_override_property(object_class, PROP_RECORDING, "recording");
    g_object_class_override_property(
            object_class, PROP_NUM_CALPOINTS, "num-calpoints"
            );
}

/* ***************************** public functions *************************** */

/**
 * geye_eyelink_et_new:(constructor)
 *
 * Construct a new GEyeEyelinkEt*
 *
 * returns:(transfer full):a new GEyeEyelinkEt*
 */
GEyeEyelinkEt*
geye_eyelink_et_new()
{
    return g_object_new(GEYE_TYPE_EYELINK_ET, NULL);
}

/**
 * geye_eyelink_et_destroy:(destructor)
 * @self:the #GEyeEyelinkEt instance to destroy.
 *
 * Drops a reference on self.
 */
void
geye_eyelink_et_destroy(GEyeEyelinkEt* self)
{
    g_object_unref(self);
}

/**
 * geye_eyelink_et_set_simulated:
 * @param self : A GEyeEyelinkEt
 * @param simulated : Whether or not to start the eyetracker in simulated mode.
 * @param error:nullable: If an error occurs it can be returned here.
 *
 * Prior to making a connection to the eyetracker, you can set the simulated
 * property to TRUE, this makes it possibe to use the this instance for
 * pilotting without a connected eyetracker.
 */
void
geye_eyelink_et_set_simulated(
        GEyeEyelinkEt  *self,
        gboolean        simulated,
        GError        **error
        )
{
    g_return_if_fail(GEYE_IS_EYELINK_ET(self));
    g_return_if_fail(error == NULL || *error == NULL);

    g_rec_mutex_lock(&self->lock);

    if (self->connected) {
        g_set_error(
                error,
                geye_eyetracker_error_quark(),
                GEYE_EYETRACKER_ERROR_INCORRECT_MODE,
                "Set simulated mode prior to connecting to the eyetracker"
        );
    }
    else {
        self->simulated = simulated;

    }

    g_rec_mutex_unlock(&self->lock);
}

/**
 * geye_eyelink_et_get_simulated:
 * @param self:A GEyeEyelinkEt
 *
 * Returns::whether or not the eyetracker is setup for simulated mode.
 */
gboolean
geye_eyelink_et_get_simulated(GEyeEyelinkEt* self)
{
    g_return_val_if_fail(GEYE_IS_EYELINK_ET(self), FALSE);
    gboolean simulated;

    g_rec_mutex_lock(&self->lock);
    simulated = self->simulated;
    g_rec_mutex_unlock(&self->lock);

    return simulated;
}

/**
 * geye_eyelink_et_set_display_dimensions:
 * @self: the eyelink instance
 * @width: The width of the screen used for eyetracking
 * @height: The height of the screen used for eyetracking
 *
 * It is necessary to call this routine at least prior to calibrating/validating
 * When calibrating the GEyeEyelinkEt sends this info to the eyelink pc, this
 * way the host pc or API can compute the coordinates where to set the
 * calibration dots.
 */
void geye_eyelink_et_set_display_dimensions(
        GEyeEyelinkEt* self, gdouble width, gdouble height
        )
{
    g_return_if_fail(GEYE_IS_EYELINK_ET(self));

    g_rec_mutex_lock(&self->lock);
    self->disp_width = width;
    self->disp_height= height;
    g_rec_mutex_unlock(&self->lock);
}
