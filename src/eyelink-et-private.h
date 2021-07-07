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


#ifndef GEYE_EYELINK_ET_PRIVATE_H
#define GEYE_EYELINK_ET_PRIVATE_H

#include "eyelink-et.h"

G_BEGIN_DECLS

struct _GEyeEyelinkEt {
    GObject      parent;
    char*        ip_address;
    gboolean     simulated;
    GThread*     eyelink_thread;
    gboolean     stop_thread;
    gboolean     connected;
    gboolean     tracking;
    gboolean     recording;
    guint        num_calpoints;
    /* Talk from instance to thread */
    GAsyncQueue* instance_to_thread;
    /* Replies are send back via this queue */
    GAsyncQueue* thread_to_instance;
    /* When the do_tracker_setup_is_called it blocks, this
     * queue can be used to talk from the hooks/callbacks
     */
    GAsyncQueue* tracker_setup_queue;

    /*
     * Callbacks for end users, although using signals is
     * more GNOME ideomatic.
     */

    geye_start_cal_func      cb_start_calibration;
    gpointer                 cb_start_calibration_data;

    geye_stop_cal_func       cb_stop_calibration;
    gpointer                 cb_stop_calibration_data;

    geye_calpoint_start_func cb_calpoint_start;
    gpointer                 cb_calpoint_start_data;

    geye_calpoint_stop_func  cb_calpoint_stop;
    gpointer                 cb_calpoint_stop_data;
};

GThread* eyelink_thread_start(GEyeEyelinkEt *self);
void     eyelink_thread_stop(GEyeEyelinkEt  *self);

void     eyelink_thread_connect(GEyeEyelinkEt *self, GError **error);
void     eyelink_thread_disconnect(GEyeEyelinkEt *self);

void     eyelink_thread_start_tracking(GEyeEyelinkEt *self, GError** error);
void     eyelink_thread_stop_tracking(GEyeEyelinkEt  *self);

void     eyelink_thread_start_recording(GEyeEyelinkEt *self, GError **error);
void     eyelink_thread_stop_recording(GEyeEyelinkEt *self);

void     eyelink_thread_start_setup(GEyeEyelinkEt *self);
void     eyelink_thread_stop_setup(GEyeEyelinkEt *self);

void     eyelink_thread_calibrate(GEyeEyelinkEt* self, GError** error);
void     eyelink_thread_validate(GEyeEyelinkEt* self, GError** error);

G_END_DECLS 

#endif 
