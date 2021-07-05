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

#include <geye.h>
#include <locale.h>

static void
eyelink_create(void)
{
    GEyeEyelinkEt* et;
    et = g_object_new(GEYE_TYPE_EYELINK_ET, NULL);
    g_assert_nonnull(et);

    g_object_unref(et);
}

static void
eyelink_connect(void)
{
    GEyeEyelinkEt  *et;
    GError*         error = NULL;
    gboolean        connected;
    et = g_object_new(GEYE_TYPE_EYELINK_ET, NULL);

    geye_eyetracker_connect(GEYE_EYETRACKER(et), &error);
    g_assert_no_error(error);

    g_object_get(et, "connected", &connected, NULL);
    g_assert_true(connected);

    g_object_unref(et);
}

static void
eyelink_connect_simulated(void)
{
    GEyeEyelinkEt  *et;
    GError*         error = NULL;
    gboolean        connected, simulated;
    et = g_object_new(GEYE_TYPE_EYELINK_ET, NULL);

    geye_eyelink_et_set_simulated(et, TRUE, &error);
    g_assert_no_error(error);

    geye_eyetracker_connect(GEYE_EYETRACKER(et), &error);
    g_assert_no_error(error);

    g_object_get(et, "connected", &connected,
                 "simulated", &simulated,
                 NULL);

    g_assert_true(connected);
    g_assert_true(simulated);

    g_object_unref(et);
}

static void
eyelink_disconnect(void)
{
    GEyeEyelinkEt  *et;
    GError*         error = NULL;
    gboolean        connected;
    et = g_object_new(GEYE_TYPE_EYELINK_ET, NULL);

    geye_eyetracker_connect(GEYE_EYETRACKER(et), &error);
    g_assert_no_error(error);

    geye_eyetracker_disconnect(GEYE_EYETRACKER(et));
    g_object_get(et, "connected", &connected, NULL);
    g_assert_false(connected);

    g_object_unref(et);
}

static void
eyelink_tracking(void)
{
    GEyeEyelinkEt  *eyelink;
    GEyeEyetracker *et;
    GError         *error = NULL;
    gboolean        tracking;

    eyelink  = geye_eyelink_et_new();
    et = GEYE_EYETRACKER(eyelink);
    g_assert_true(et);

    geye_eyetracker_connect(et, &error);
    g_assert_no_error(error);

    geye_eyetracker_start_tracking(et, &error);
    g_assert_no_error(error);

    g_object_get(et, "tracking", &tracking, NULL);
    g_assert_true(tracking);

    g_usleep(100000);

    geye_eyelink_et_destroy(et);
}

static void
eyelink_recording(void)
{
    GEyeEyelinkEt   *et;
    GError         **error = NULL;

    et = geye_eyelink_et_new();

    geye_eyelink_et_destroy(et);
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");
    g_test_init(&argc, &argv, NULL);

    g_log_set_always_fatal(G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION);

    g_test_add_func("/EyelinkEt/create",  eyelink_create);
    g_test_add_func("/EyelinkEt/connect", eyelink_connect);
    g_test_add_func(
            "/EyelinkEt/connect_simulated", eyelink_connect_simulated
            );
    g_test_add_func(
            "/EyelinkEt/disconnect", eyelink_disconnect
            );

    g_test_add_func(
            "/EyelinkEt/start_tracking", eyelink_tracking
    );
    g_test_add_func(
            "/EyelinkEt/start_recording", eyelink_recording
    );

    return g_test_run();
}
