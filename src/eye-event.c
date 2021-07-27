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

#include "eye-event.h"

/**
 * geye_sample_new:
 * @eye: one of GEyeEyeType
 * @time: the time at which this event occured
 * @x: the x coordinate
 * @y: the y coordinate
 *
 * Create a new sample event.
 *
 * Returns:(transfer full): A new `GEyeSample`
 */
GEyeSample*
geye_sample_new(GEyeEyeType eye, gdouble time, gdouble x, gdouble y)
{
    GEyeSample* sample = g_slice_new(GEyeSample);

    sample->parent.type = GEYE_EVENT_SAMPLE;
    sample->parent.eye  = eye;
    sample->parent.time = time;
    sample->x = x;
    sample->y = y;

    return sample;
}

void
geye_sample_free(GEyeSample* sample)
{
    g_slice_free(GEyeSample, sample);
}

GEyeSample*
geye_sample_copy(const GEyeSample* sample)
{
    g_return_val_if_fail(sample != NULL, NULL);

    return g_slice_dup(GEyeSample, sample);
}

G_DEFINE_BOXED_TYPE(GEyeSample, geye_sample,
                    geye_sample_copy, geye_sample_free)

