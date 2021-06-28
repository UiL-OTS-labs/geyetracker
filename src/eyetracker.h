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


#ifndef GEYE_TRACKER_H
#define GEYE_TRACKER_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS 

#define GEYE_TYPE_EYETRACKER geye_eyetracker_get_type()
G_DECLARE_INTERFACE(GeyeEyetracker, geye_eyetracker, GEYE, EYETRACKER, GObject)

struct _GeyeEyetrackerInterface {
    GTypeInterface parent_iface;

    void (*connect)         (GeyeEyetracker*    et,
                             GError**           error);

    void (*start_tracking)  (GeyeEyetracker*    et,
                             GError**           error);

    void (*calibrate)       (GeyeEyetracker*    et,
                             GError**           error);

    void (*validate)        (GeyeEyetracker*    et,
                             GError**           error);
};

G_MODULE_EXPORT void
geye_eyetracker_connect(GeyeEyetracker* et, GError** error);
G_MODULE_EXPORT void
geye_eyetracker_start_tracking(GeyeEyetracker* et, GError** error);
G_MODULE_EXPORT void
geye_eyetracker_calibrate(GeyeEyetracker* et, GError** error);
G_MODULE_EXPORT void
geye_eyetracker_validate(GeyeEyetracker* et, GError** error);

G_END_DECLS 

#endif 
