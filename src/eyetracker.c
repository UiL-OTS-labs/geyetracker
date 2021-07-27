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

#include "eyetracker.h"
#include "eye-event.h"

G_DEFINE_INTERFACE(GEyeEyetracker, geye_eyetracker, G_TYPE_OBJECT)

enum signals {
    CONNECTED,
    CAL_POINT_START,
    CAL_POINT_STOP,
    SAMPLE,
    ERROR,
    N_SIGNALS,
};

static gint signals [N_SIGNALS];

static void
geye_eyetracker_default_init(GEyeEyetrackerInterface *iface)
{
    GParamSpec* spec;
    spec = g_param_spec_boolean (
        "connected",
        "Connected",
        "Whether or not there is a connection with the eyetracker.",
        FALSE,
        G_PARAM_READABLE
    );
    g_object_interface_install_property(iface, spec);

    spec = g_param_spec_boolean (
            "recording",
            "Recording",
            "Whether or not the eyetracker is recording to the eyetracking log.",
            FALSE,
            G_PARAM_READABLE
    );
    g_object_interface_install_property(iface, spec);

    spec = g_param_spec_boolean (
            "tracking",
            "Tracking",
            "Whether or not the eyetracker is tracking.",
            FALSE,
            G_PARAM_READABLE
    );
    g_object_interface_install_property(iface, spec);

    spec = g_param_spec_uint(
            "num-calpoints",
            "NumCalpoints",
            "The number of calibration points used for calibration",
            0,
            G_MAXUINT,
            9,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );
    g_object_interface_install_property(iface, spec);

    spec = g_param_spec_string(
            "tracker-info",
            "Tracker information",
            "After being connected provides information about the eyetracker.",
            NULL,
            G_PARAM_READABLE
            );
    g_object_interface_install_property(iface, spec);

    /**
     * GEyeEyetracker::connected:
     * @eyetracker: the object that received this signal.
     * @connected: True if the eyetracker is in a connected status.
     *
     * Emitted once the eyetracker is (dis-)connected to its host.
     * Although, it is possible that the eyetracker makes an connection in another
     * thread, the signal should be emitted in the thread in which the eyetracker
     * was created.
     */
    signals[CONNECTED] = g_signal_new(
            "connected",
            GEYE_TYPE_EYETRACKER,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
            0, //G_STRUCT_OFFSET(GEyeEyetrackerInterface, set_connected),
            NULL,
            NULL,
            NULL,
            G_TYPE_NONE,
            1,
            G_TYPE_BOOLEAN);

    /**
     * GEyeEyetracker::calpoint-start:
     * @eyetracker: the object that received this signal
     * @x: the x coordinate at which this dot should be presented
     * @y: the y coordinate at which this dot should be presented
     *
     * An eyetracker "learns" where a participant is looking at the screen
     * by presenting calibrations dots at specific points at the screen and
     * keeping track of where the participant is looking for each calibration
     * point.
     * This signal is emitted during calibration and validation. At tells the
     * user of where a calibration point should be presented.
     */
    signals[CAL_POINT_START] = g_signal_new(
            "calpoint-start",
            GEYE_TYPE_EYETRACKER,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
            0, //G_STRUCT_OFFSET(GEyeEyetrackerInterface, set_connected),
            NULL, NULL,
            NULL,
            G_TYPE_NONE,
            2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

    /**
     * GEyeEyetracker::calpoint-stop:
     * @eyetracker: the object that received this signal
     *
     * This signal is emitted during calibration and validation. This signal
     * tells the user that the previously presented dot isn't necessary
     * anymore and can be erased.
     */
    signals[CAL_POINT_STOP] = g_signal_new(
            "calpoint-stop",
            GEYE_TYPE_EYETRACKER,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
            0, //G_STRUCT_OFFSET(GEyeEyetrackerInterface, set_connected),
            NULL, NULL,
            NULL,
            G_TYPE_NONE,
            0);

    /**
     * GEyeEyetracker::sample:
     * @eyetracker: the object that received this signal
     * @sample:(transfer none): A sample of the left, right eye or an average.
     *
     * This signal is emitted while tracking and is signalled as soon as it
     * is received, it depends on the eyetracker for which `GEyeEyeTypes` can be
     * received. E.g. some eyetrackers do not support an average coordinate of
     * both eyes together.
     */
    signals[SAMPLE] = g_signal_new(
            "sample",
            GEYE_TYPE_EYETRACKER,
            G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
            0,
            NULL, NULL,
            NULL,
            G_TYPE_NONE,
            1, GEYE_TYPE_SAMPLE
            );

    /**
     * GEyeEyetracker::error
     * @eyetracker: the object that received this signal,
     * @error:transfer none: an error that might be helpful in diagnosing
     *                       a problem in the backend of the eyetracker.
     *
     * Sometimes things go bad, due to the design of geye_eyetrackers this
     * can happen in a thread that inferfaces with the eyetracker. The caller
     * should generally not wait for the thread to finish its business. However,
     * something might go wrong in the thread and since the caller isn't waiting
     * for the error it can't perform subsequent procedures. This signal might
     * be helpful in diagnosing those problems.
     */
     signals[ERROR] = g_signal_new(
             "error",
             GEYE_TYPE_EYETRACKER,
             G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
             0,
             NULL, NULL,
             NULL,
             G_TYPE_NONE,
             1,
             G_TYPE_STRING
             );

}

void
geye_eyetracker_connect(GEyeEyetracker* et, GError** error)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->connect != NULL);
    iface->connect(et, error);
}

void
geye_eyetracker_disconnect(GEyeEyetracker* et)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->disconnect != NULL);
    iface->disconnect(et);
}

/**
 * geye_eyetracker_get_tracker_info:
 * @et The GEyeEyetracker instance whose tracker information one would like to
 *     know.
 *
 * This method provides a string describing which eyetracker you are using
 * and which software if all this information is made available by the
 * library of the vendor.
 *
 * Returns:transfer full:
 *         A string describing which eyetracker you are using. Or NULL when
 *         it is not available e.g. when the eyetracker isn't connected yet.
 */
gchar*
geye_eyetracker_get_tracker_info(GEyeEyetracker* et)
{
    GEyeEyetrackerInterface *iface;

    g_return_val_if_fail(GEYE_IS_EYETRACKER(et), NULL);

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_val_if_fail(iface->tracker_info, NULL);

    return iface->tracker_info(et);
}

void
geye_eyetracker_start_tracking(GEyeEyetracker* et, GError** error)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->start_tracking != NULL);
    iface->start_tracking(et, error);
}

void
geye_eyetracker_stop_tracking(GEyeEyetracker* et)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->stop_tracking != NULL);
    iface->stop_tracking(et);
}

void
geye_eyetracker_start_recording(GEyeEyetracker* et, GError** error)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->start_recording != NULL);
    iface->start_recording(et, error);
}

void
geye_eyetracker_stop_recording(GEyeEyetracker* et)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->stop_recording != NULL);
    iface->stop_recording(et);
}

void
geye_eyetracker_start_setup(GEyeEyetracker* et)
{
    GEyeEyetrackerInterface *iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->start_setup != NULL);
    iface->start_setup(et);
}

void
geye_eyetracker_stop_setup(GEyeEyetracker* et)
{
    GEyeEyetrackerInterface *iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->stop_setup != NULL);
    iface->stop_setup(et);
}

void
geye_eyetracker_calibrate(GEyeEyetracker* et, GError** error)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->calibrate != NULL);
    iface->calibrate(et, error);
}

void
geye_eyetracker_validate(GEyeEyetracker* et, GError** error)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->validate != NULL);
    iface->validate(et, error);
}

void
geye_eyetracker_set_num_calpoints(GEyeEyetracker* et, guint n)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->set_num_calpoints != NULL);
    iface->set_num_calpoints(et, n);
}

guint
geye_eyetracker_get_num_calpoints(GEyeEyetracker* et)
{
    GEyeEyetrackerInterface* iface;

    g_return_val_if_fail(GEYE_IS_EYETRACKER(et), 0);

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_val_if_fail(iface->get_num_calpoints != NULL, 0);
    return iface->get_num_calpoints(et);
}

void
geye_eyetracker_trigger_calpoint(GEyeEyetracker* et, GError** error)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->trigger_calpoint != NULL);
    iface->trigger_calpoint(et, error);
}

void
geye_eyetracker_set_calibration_start_cb(GEyeEyetracker        *et,
                                         geye_start_cal_func    cb,
                                         gpointer               data)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->set_calibration_start_cb != NULL);
    iface->set_calibration_start_cb(et, cb, data);
}

void
geye_eyetracker_set_calibration_stop_cb (GEyeEyetracker        *et,
                                         geye_stop_cal_func     cb,
                                         gpointer               data)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->set_calibration_stop_cb != NULL);
    iface->set_calibration_stop_cb(et, cb, data);
}

void
geye_eyetracker_set_calpoint_start_cb (GEyeEyetracker           *et,
                                       geye_calpoint_start_func  cb,
                                       gpointer                  data)
{
    GEyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->set_calpoint_start_cb != NULL);
    iface->set_calpoint_start_cb(et, cb, data);
}

void
geye_eyetracker_set_calpoint_stop_cb  (GEyeEyetracker          *et,
                                       geye_calpoint_stop_func  cb,
                                       gpointer                 data)
{
    GEyeEyetrackerInterface *iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->set_calpoint_stop_cb != NULL);
    iface->set_calpoint_stop_cb(et, cb, data);
}

void
geye_eyetracker_set_image_data_cb(GEyeEyetracker            *et,
                                  geye_image_data_func       cb,
                                  gpointer                   data)
{
    GEyeEyetrackerInterface *iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->set_image_data_cb != NULL);
    iface->set_image_data_cb(et, cb, data);
}

gboolean
geye_eyetracker_send_key_press(GEyeEyetracker*  et,
                               guint16          key,
                               guint            modifiers)
{
    GEyeEyetrackerInterface *iface;

    g_return_val_if_fail(GEYE_IS_EYETRACKER(et), FALSE);
    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_val_if_fail(iface->send_key_press != NULL, FALSE);

    return iface->send_key_press(et, key, modifiers);
}
