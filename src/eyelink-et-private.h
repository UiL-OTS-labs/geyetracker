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

gboolean eyelink_thread_send_key_press(
                GEyeEyelinkEt* self, guint16 key, guint modifiers
                );

G_END_DECLS 

#endif 
