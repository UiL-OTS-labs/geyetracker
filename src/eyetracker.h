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
G_MODULE_EXPORT
G_DECLARE_INTERFACE(GEyeEyetracker, geye_eyetracker, GEYE, EYETRACKER, GObject)

typedef void (*geye_start_cal_func)(GEyeEyetracker *self, gpointer data);
typedef void (*geye_stop_cal_func)(GEyeEyetracker *self, gpointer data);
typedef void (*geye_calpoint_start_func) (GEyeEyetracker *self,
                                          gdouble x,
                                          gdouble y,
                                          gpointer data
                                          );
typedef void (*geye_calpoint_stop_func)(GEyeEyetracker*self, gpointer data);

typedef void (*geye_image_data_func)(GEyeEyetracker *self,
                                     guint           width,
                                     guint           height,
                                     gsize           img_num_bytes,
                                     guint8*         img_data,
                                     gpointer        data);


struct _GEyeEyetrackerInterface {
    GTypeInterface parent_iface;

    void (*connect)         (GEyeEyetracker    *et,
                             GError           **error);

    void (*disconnect)      (GEyeEyetracker    *et);

    gchar* (*tracker_info)  (GEyeEyetracker *et);


    void (*start_tracking)  (GEyeEyetracker    *et,
                             GError           **error);

    void (*stop_tracking)   (GEyeEyetracker    *et);

    void (*start_recording) (GEyeEyetracker    *et,
                             GError           **error);

    void (*stop_recording)  (GEyeEyetracker    *et);

    void (*log_message)     (GEyeEyetracker    *et,
                             const gchar       *msg,
                             GError           **error);

    void (*start_setup)     (GEyeEyetracker    *et);

    void (*stop_setup)      (GEyeEyetracker    *et);

    void (*calibrate)       (GEyeEyetracker    *et,
                             GError           **error);

    void (*validate)        (GEyeEyetracker    *et,
                             GError           **error);

    void (*set_num_calpoints)(GEyeEyetracker   *et, guint n);
    guint (*get_num_calpoints)(GEyeEyetracker  *et);

    void (*trigger_calpoint)(GEyeEyetracker    *et,
                             GError           **error);

    void (*set_calibration_start_cb)(GEyeEyetracker        *et,
                                     geye_start_cal_func    cb,
                                     gpointer               data);

    void (*set_calibration_stop_cb) (GEyeEyetracker        *et,
                                     geye_stop_cal_func     cb,
                                     gpointer               data);

    void (*set_calpoint_start_cb)   (GEyeEyetracker            *et,
                                     geye_calpoint_start_func   cb,
                                     gpointer                   data);

    void (*set_calpoint_stop_cb)    (GEyeEyetracker            *et,
                                     geye_calpoint_stop_func    cb,
                                     gpointer                   data);

    void (*set_image_data_cb)       (GEyeEyetracker            *et,
                                     geye_image_data_func       cb,
                                     gpointer                   data);

    gboolean (*send_key_press)      (GEyeEyetracker            *et,
                                     guint16                    key_code,
                                     guint                      modifiers);
};

G_MODULE_EXPORT void
geye_eyetracker_connect(GEyeEyetracker* et, GError** error);

G_MODULE_EXPORT void
geye_eyetracker_disconnect(GEyeEyetracker* et);

G_MODULE_EXPORT gchar*
geye_eyetracker_get_tracker_info(GEyeEyetracker* et);

G_MODULE_EXPORT void
geye_eyetracker_start_tracking(GEyeEyetracker* et, GError** error);

G_MODULE_EXPORT void
geye_eyetracker_stop_tracking(GEyeEyetracker* et);

G_MODULE_EXPORT void
geye_eyetracker_start_recording(GEyeEyetracker* et, GError** error);

G_MODULE_EXPORT void
geye_eyetracker_stop_recording(GEyeEyetracker* et);

G_MODULE_EXPORT void
geye_eyetracker_log_message(
        GEyeEyetracker* et,
        const char*     msg,
        GError**        error
        );

G_MODULE_EXPORT void
geye_eyetracker_start_setup(GEyeEyetracker* self);

G_MODULE_EXPORT void
geye_eyetracker_stop_setup(GEyeEyetracker* self);

G_MODULE_EXPORT void
geye_eyetracker_trigger_calpoint(GEyeEyetracker* self, GError **error);

G_MODULE_EXPORT void
geye_eyetracker_calibrate(GEyeEyetracker* et, GError** error);

G_MODULE_EXPORT void
geye_eyetracker_validate(GEyeEyetracker* et, GError** error);

G_MODULE_EXPORT void
geye_eyetracker_set_num_calpoints(GEyeEyetracker* et, guint n);

G_MODULE_EXPORT guint
geye_eyetracker_get_num_calpoints(GEyeEyetracker* et);

G_MODULE_EXPORT void
geye_eyetracker_set_calibration_start_cb(GEyeEyetracker        *et,
                                         geye_start_cal_func    cb,
                                         gpointer               data);

G_MODULE_EXPORT void
geye_eyetracker_set_calibration_stop_cb (GEyeEyetracker        *et,
                                         geye_stop_cal_func     cb,
                                         gpointer               data);

G_MODULE_EXPORT void
geye_eyetracker_set_calpoint_start_cb  (GEyeEyetracker              *et,
                                        geye_calpoint_start_func    cb,
                                        gpointer                    data);
G_MODULE_EXPORT void
geye_eyetracker_set_calpoint_stop_cb   (GEyeEyetracker              *et,
                                        geye_calpoint_stop_func     cb,
                                        gpointer                    data);

G_MODULE_EXPORT void
geye_eyetracker_set_image_data_cb(GEyeEyetracker            *et,
                                  geye_image_data_func       cb,
                                  gpointer                   data);

G_MODULE_EXPORT gboolean
geye_eyetracker_send_key_press(GEyeEyetracker  *et,
                               guint16          key_code,
                               guint            modifiers);


G_END_DECLS 

#endif 
