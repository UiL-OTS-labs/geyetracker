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

#include "eyelink-et-private.h"
#include "eyetracker-error.h"
#include <EyeLink/core_expt.h>

const char* EYELINK_THREAD_NAME = "Eyelink-thread";

typedef enum {
    ET_STOP,
    ET_CONNECT,
    ET_CONNECTED,
    ET_CONNECTED_ERROR,
} ThreadMsgType;


typedef struct {
    ThreadMsgType type;
    GVariant* content;
} EyelinkThreadMsg;

static void
et_send_message(GEyeEyelinkEt* self, EyelinkThreadMsg* msg) {
    g_async_queue_push(self->instance_to_thread, msg);
}

static EyelinkThreadMsg*
et_receive_reply(GEyeEyelinkEt* self)
{
    EyelinkThreadMsg* msg = g_async_queue_pop(self->thread_to_instance);
    return msg;
}

static void
et_reply(GEyeEyelinkEt* self, EyelinkThreadMsg* msg) {
    g_async_queue_push(self->thread_to_instance, msg);
}

static void
et_connect(GEyeEyelinkEt* self) {

    int ret;

    if (self->ip_address) {
        // this initializes the library.
        ret = open_eyelink_connection(-1);
        if (ret)
            g_warning("Unable to initialize the library");


        ret = set_eyelink_address(self->ip_address);
        if (ret)
            g_warning("Eyelink-Thread:Invalid ip address: \"%s\".",
                      self->ip_address);
    }

    
    if (open_eyelink_connection(0)) {
        g_warning("Unable to connect to the eyelink eyetracker");
        EyelinkThreadMsg* msg = g_malloc0(sizeof(EyelinkThreadMsg));
        msg->type = ET_CONNECTED_ERROR;
        et_reply(self, msg);
    }
    else {
        EyelinkThreadMsg* msg = g_malloc0(sizeof(EyelinkThreadMsg));
        msg->type = ET_CONNECTED;
        et_reply(self, msg);
    }
}

static void
et_disconnect(GEyeEyelinkEt* self)
{
    close_eyelink_connection();
    self->connected = FALSE;
}

static void
handle_msg(GEyeEyelinkEt* self, EyelinkThreadMsg* msg)
{
    switch(msg->type) {
        case ET_STOP:
            self->stop_thread = TRUE;
            if (self->connected)
                et_disconnect(self);
            break;
        case ET_CONNECT:
            et_connect(self);
            break;
        case ET_CONNECTED:
            g_warning("EyelinkThread Unexpected message");
            break;
        default:
            g_warn_if_reached();
    }
}

static gboolean
monitor_main_thread(GEyeEyelinkEt* self, gboolean sleep)
{
    EyelinkThreadMsg* msg;
    if (sleep)
       msg  = g_async_queue_try_pop(self->instance_to_thread);
    else
       msg  = g_async_queue_timeout_pop(self->instance_to_thread, 1000);

    if (msg) {
        handle_msg(self, msg);
        if (msg->content)
            g_variant_unref(msg->content);
        g_free(msg);
        return TRUE;
    }
    return FALSE;
}

gpointer eyelink_thread(gpointer data) {
    GEyeEyelinkEt* self = GEYE_EYELINK_ET(data);

    while (self->stop_thread) {
        gboolean didsomething = FALSE;
        
        if (didsomething)
            monitor_main_thread(self, FALSE);
        else
            monitor_main_thread(self, TRUE);
            
    }

    return NULL;
}

GThread*
eyelink_thread_start(GEyeEyelinkEt* self)
{
    GThread* thread = NULL;
    thread = g_thread_new(EYELINK_THREAD_NAME, eyelink_thread, self);
    return thread;
}

void
eyelink_thread_stop(GEyeEyelinkEt* self)
{
    EyelinkThreadMsg* msg = g_malloc0(sizeof(EyelinkThreadMsg));
    msg->type = ET_STOP;
    et_send_message(self, msg);
    g_thread_join(self->eyelink_thread);
}

void
eyelink_thread_connect(GEyeEyelinkEt* self, GError** error)
{
    EyelinkThreadMsg* msg = g_malloc0(sizeof(EyelinkThreadMsg));
    msg->type = ET_CONNECT;
    et_send_message(self, msg);
    msg = et_receive_reply(self);
    if (msg->type != ET_CONNECTED) {
        g_set_error(
                error,
                GEYE_EYETRACKER_ERROR,
                GEYE_EYETRACKER_ERROR_UNABLE_TO_CONNECT,
                "The eyelink thread was not able to connect to the eyetracker."
                );
    }
}


