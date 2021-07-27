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


#ifndef GEYE_EVENT_H
#define GEYE_EVENT_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS 

typedef enum _GEyeEventType {
    GEYE_EVENT_SAMPLE,
    GEYE_EVENT_FIX_START,
    GEYE_EVENT_FIX_END,
    GEYE_EVENT_FIX,
    GEYE_EVENT_SAC_START,
    GEYE_EVENT_SAC_END,
    GEYE_EVENT_SAC
} GEyeEventType;

typedef enum _GEyeEyeType{
    GEYE_LEFT,
    GEYE_RIGHT,
    GEYE_AVG,
} GEyeEyeType;

/**
 * GEyeEvent:
 * @type: signals what event this is
 * @eye: tell which eye(s) this sample represents
 * @time: the time since some specific time in the past.
 *
 * This is something all event have together, they are of some type,
 * They relate to one eye or eg the average of both eyes, and they
 * have a time at which they occured.
 */
typedef struct _GEyeEvent {
    GEyeEventType   type;
    GEyeEyeType     eye;
    gdouble         time;
} GEyeEvent;

/**
 * GEyeSample:
 * @parent: this is one kind of an eyevent
 * @x: the x position of the coordinate
 * @y: the y position of the coordinate
 *
 * This struct specifies which eye was measured on which location at
 * which time.
 */
typedef struct _GEyeSample {
    GEyeEvent       parent;
    gdouble         x;
    gdouble         y;
} GEyeSample;

#define GEYE_TYPE_SAMPLE geye_sample_get_type()

G_MODULE_EXPORT GEyeEyeType
geye_event_get_event_type(GEyeEvent* eventj);

G_MODULE_EXPORT GEyeEyeType
geye_event_get_time(GEyeEvent* event);

G_MODULE_EXPORT GEyeSample*
geye_sample_new(GEyeEyeType eye, gdouble time, gdouble x, gdouble y);

G_END_DECLS 

#endif 
