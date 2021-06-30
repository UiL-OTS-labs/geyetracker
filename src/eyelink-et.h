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

G_MODULE_EXPORT GEyeEyelinkEt*
g_eyelink_et_new(void);


G_END_DECLS 

#endif 
