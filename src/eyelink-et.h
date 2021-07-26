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


#ifndef GEYE_EYELINK_ET_H
#define GEYE_EYELINK_ET_H

#include "eyetracker.h"
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GEYE_TYPE_EYELINK_ET geye_eyelink_et_get_type()
G_MODULE_EXPORT
G_DECLARE_FINAL_TYPE(GEyeEyelinkEt, geye_eyelink_et, GEYE, EYELINK_ET, GObject)

struct _GEyeEyelinkEt {
    GObject         parent;

    GRecMutex       lock;

    char*           ip_address;
    char*           info;

    gboolean        simulated;
    GThread*        eyelink_thread;
    gboolean        connected;
    gboolean        tracking;
    gboolean        recording;
    guint           num_calpoints;
    gdouble         disp_width;
    gdouble         disp_height;
    /* Talk from instance to thread */
    GAsyncQueue*    instance_to_thread;
    /* Replies are send back via this queue */
    GAsyncQueue*    thread_to_instance;

    guint8*         image_data; //RGBA
    gsize           image_size; // width * height * 4.

    gboolean        quit_hooks;   // Thread only.
    gboolean        stop_thread;  // Thread only.

    GMainContext   *main_context; // The context in which signal will be emitted.

    /*
     * Callbacks for end users, although using signals is
     * more ideomatic in GNOME/gobject applications.
     *
     * signals aren't implemented yet...
     */
    geye_start_cal_func      cb_start_calibration;
    gpointer                 cb_start_calibration_data;

    geye_stop_cal_func       cb_stop_calibration;
    gpointer                 cb_stop_calibration_data;

    geye_calpoint_start_func cb_calpoint_start;
    gpointer                 cb_calpoint_start_data;

    geye_calpoint_stop_func  cb_calpoint_stop;
    gpointer                 cb_calpoint_stop_data;

    geye_image_data_func     cb_image_data;
    gpointer                 cb_image_data_data;
};


G_MODULE_EXPORT GEyeEyelinkEt*
geye_eyelink_et_new(void);

G_MODULE_EXPORT void
geye_eyelink_et_destroy(GEyeEyelinkEt* self);

G_MODULE_EXPORT void
geye_eyelink_et_set_simulated (
        GEyeEyelinkEt  *self,
        gboolean        simulated,
        GError        **error
        );

G_MODULE_EXPORT gboolean
geye_eyelink_et_get_simulated(GEyeEyelinkEt* self);

G_MODULE_EXPORT void
geye_eyelink_et_set_display_dimensions(
        GEyeEyelinkEt*, gdouble width, gdouble height
        );

G_MODULE_EXPORT char*
geye_eyelink_et_get_ip_address(GEyeEyelinkEt* et);

G_MODULE_EXPORT void
geye_eyelink_et_set_ip_address(GEyeEyelinkEt* et, const char* address);


G_END_DECLS 

#endif 
