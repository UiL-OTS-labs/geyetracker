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

#include "eyelink-et.h"
#include "eyelink-et-private.h"
#include "eyetracker.h"

static void
geye_eyetracker_interface_init(GEyeEyetrackerInterface* iface);

G_DEFINE_TYPE_WITH_CODE(GEyeEyelinkEt,
                        geye_eyelink_et,
                        G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GEYE_TYPE_EYETRACKER,
                                              geye_eyetracker_interface_init)
                        )
static void
geye_eyelink_et_init(GEyeEyelinkEt* self) {
    self->stop_thread       = FALSE;
    self->eyelink_thread    = eyelink_thread_start(self);
}

static void
eyelink_et_connect(GEyeEyetracker* self, GError** error)
{
    eyelink_thread_connect(GEYE_EYELINK_ET(self), error);
}

static void
geye_eyetracker_interface_init(GEyeEyetrackerInterface* iface)
{
    iface->connect = eyelink_et_connect;
}

static void
eyelink_et_dispose(GObject* gobject)
{
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(gobject);

    g_free(self->ip_address);
    self->ip_address =NULL;
    
    // stop eyelink thread
    if (self->eyelink_thread)
        eyelink_thread_stop(self);
    g_thread_unref(self->eyelink_thread);
    self->eyelink_thread = NULL;

    G_OBJECT_CLASS(geye_eyelink_et_parent_class)->dispose(gobject);
}

static void
eyelink_et_finalize(GObject* gobject)
{
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(gobject);
    g_free(self->ip_address);
    G_OBJECT_CLASS(geye_eyelink_et_parent_class)->finalize(gobject);
}


static void
geye_eyelink_et_class_init(GEyeEyelinkEtClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = eyelink_et_dispose;
    object_class->finalize = eyelink_et_finalize;

}
