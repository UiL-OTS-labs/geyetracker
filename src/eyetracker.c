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



