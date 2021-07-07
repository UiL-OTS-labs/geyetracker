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

G_DEFINE_INTERFACE(GEyeEyetracker, geye_eyetracker, G_TYPE_OBJECT)

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
geye_eyetracker_set_calpoint_start_cb (GEyeEyetracker            *et,
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
                                       geye_calpoint_stop_func cb,
                                       gpointer                data)
{
    GEyeEyetrackerInterface *iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->set_calpoint_stop_cb != NULL);
    iface->set_calpoint_stop_cb(et, cb, data);
}

