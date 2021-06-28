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

G_DEFINE_INTERFACE(GeyeEyetracker, geye_eyetracker, G_TYPE_OBJECT)

enum EyeTrackerProperties {
    PROP_0,
    PROP_CONNECTED,
    N_PROPERTIES 
};

GParamSpec* obj_properties[N_PROPERTIES] = {NULL,};

static void
geye_eyetracker_default_init(GeyeEyetrackerInterface *iface)
{

    obj_properties[PROP_CONNECTED] = g_param_spec_boolean (
        "connected",
        "Connected",
        "Whether or not there is a connection with the eyetracker.",
        FALSE,
        G_PARAM_CONSTRUCT | G_PARAM_READABLE
    );

    g_object_class_install_properties(G_OBJECT_CLASS(iface), N_PROPERTIES, obj_properties);
}

void
geye_eyetracker_connect(GeyeEyetracker* et, GError** error)
{
    GeyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->connect != NULL);
    iface->connect(et, error);
}

void
geye_eyetracker_start_tracking(GeyeEyetracker* et, GError** error)
{
    GeyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->start_tracking != NULL);
    iface->start_tracking(et, error);
}

void
geye_eyetracker_calibrate(GeyeEyetracker* et, GError** error)
{
    GeyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->calibrate != NULL);
    iface->calibrate(et, error);
}

void
geye_eyetracker_validate(GeyeEyetracker* et, GError** error)
{
    GeyeEyetrackerInterface* iface;

    g_return_if_fail(GEYE_IS_EYETRACKER(et));

    iface = GEYE_EYETRACKER_GET_IFACE(et);
    g_return_if_fail(iface->validate != NULL);
    iface->validate(et, error);
}


