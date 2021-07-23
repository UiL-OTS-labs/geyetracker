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

typedef struct {
    GEyeEyelinkEt  *et;
    GMainContext   *context;
    GMainLoop      *loop;
}EyelinkFixture;

static gint
quit_main_loop(gpointer data) {
    GMainLoop *loop = data;
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

static void
eyelink_fixture_setup(EyelinkFixture* fix, gconstpointer data)
{
    const gint* seconds = data;
    fix->context = g_main_context_new();
    g_main_context_push_thread_default(fix->context);
    GSource* timeout = g_timeout_source_new_seconds(*seconds);
    g_source_set_callback(timeout, quit_main_loop, fix->loop, NULL);
    g_source_attach(timeout, fix->context);

    fix->loop = g_main_loop_new(fix->context, FALSE);

    fix->et = geye_eyelink_et_new();
}

static void
eyelink_fixture_tear_down(EyelinkFixture* fix, gconstpointer data)
{
    (void) data;
    g_clear_object(&fix->et);
    g_main_loop_unref(fix->loop);

    g_main_context_pop_thread_default(fix->context);
    g_main_context_unref(fix->context);
}

static void
eyelink_create(void)
{
    GEyeEyelinkEt* et;
    et = g_object_new(GEYE_TYPE_EYELINK_ET, NULL);
    g_assert_nonnull(et);

    g_object_unref(et);
}

typedef struct ConnectData {
    EyelinkFixture *fix;
    gboolean connected;
} ConnectData;

static void
on_test_connect(GEyeEyetracker* et, gboolean connected, gpointer data)
{
    ConnectData *cdata = data;
    cdata->connected = connected;
    g_assert(et == GEYE_EYETRACKER(cdata->fix->et));
    g_main_loop_quit(cdata->fix->loop);
}

static void
eyelink_connect(EyelinkFixture* fix, gconstpointer data)
{
    (void) data;
    ConnectData connect_info = {fix, FALSE};
    gboolean connected;

    geye_eyetracker_connect(GEYE_EYETRACKER(fix->et), NULL);
    g_signal_connect(
            fix->et,
            "connected",
            G_CALLBACK(on_test_connect),
            &connect_info);

    g_main_loop_run(fix->loop);

    g_object_get(fix->et, "connected", &connected, NULL);
    g_assert_true(connected);
    g_assert_true(connect_info.connected);
}

static void
eyelink_connect_simulated(EyelinkFixture* fix, gconstpointer data)
{
    (void) data;
    gboolean connected, simulated;
    ConnectData connect_info = {
            .fix = fix,
            .connected = FALSE
    };
    GError  *error = NULL;

    geye_eyelink_et_set_simulated(fix->et, TRUE, &error);
    g_assert_no_error(error);

    g_signal_connect(fix->et,
                     "connected",
                     G_CALLBACK(on_test_connect),
                     &connect_info);

    geye_eyetracker_connect(GEYE_EYETRACKER(fix->et), NULL);
    g_assert_no_error(error);

    g_main_loop_run(fix->loop);

    g_object_get(fix->et, "connected", &connected,
                 "simulated", &simulated,
                 NULL);

    g_assert_true(connected);
    g_assert_true(simulated);
    g_assert_true(connect_info.connected);
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
    gint tenth_of_sec = 100000;

    eyelink  = geye_eyelink_et_new();
    et = GEYE_EYETRACKER(eyelink);
    g_assert_true(et);

    geye_eyetracker_connect(et, &error);
    g_assert_no_error(error);

    geye_eyetracker_start_tracking(et, &error);
    g_assert_no_error(error);

    g_usleep(tenth_of_sec);
    g_object_get(et, "tracking", &tracking, NULL);
    g_assert_true(tracking);

    geye_eyetracker_stop_tracking(et);

    g_usleep(tenth_of_sec);
    g_object_get(et, "tracking", &tracking, NULL);
    g_assert_false(tracking);

    geye_eyelink_et_destroy(eyelink);
}

/*
static void
eyelink_recording(void)
{
    GEyeEyelinkEt   *et;
    GError          *error = NULL;
    gboolean         recording;

    et = geye_eyelink_et_new();

    geye_eyetracker_connect(GEYE_EYETRACKER(et), &error);
    g_assert_no_error(error);

    geye_eyetracker_start_recording(GEYE_EYETRACKER(et), &error);
    g_assert_no_error(error);

    g_object_get(et, "recording", &recording, NULL);
    g_assert_true(recording);

    geye_eyetracker_stop_recording(GEYE_EYETRACKER(et));

    g_object_get(et, "recording", &recording, NULL);
    g_assert_false(recording);

    geye_eyelink_et_destroy(et);
}
*/

static void
eyelink_run_setup(void)
{
    GEyeEyelinkEt   *el;
    GEyeEyetracker  *et;
    GError          *error = NULL;

    guint sleep = 100000;

    el = geye_eyelink_et_new();
    et = GEYE_EYETRACKER(el);

    geye_eyetracker_connect(et, &error);
    g_assert_no_error(error);
    g_usleep(sleep);

    geye_eyetracker_start_setup(et);

    g_usleep(2e6);

    geye_eyetracker_stop_setup(et);

    geye_eyelink_et_destroy(el);
}

//static void
//eyelink_run_setup_exit_escape(void)
//{
//    GEyeEyelinkEt   *el;
//    GEyeEyetracker  *et;
//    GError          *error = NULL;
//
//    el = geye_eyelink_et_new();
//    et = GEYE_EYETRACKER(el);
//
//    geye_eyetracker_connect(et, &error);
//    g_assert_no_error(error);
//
//    geye_eyetracker_start_setup(et);
//
//    g_print("Press escape on Eyelink Host to quit this test.\n");
//
//    geye_eyelink_et_destroy(el);
//}

typedef struct CallbackData {
    GEyeEyetracker *et;
    gboolean        called;
    gint            n_calls;
    gboolean        et_the_same;
} CallbackData;

static void
cal_start_stop_cb(GEyeEyetracker* et, gpointer data)
{
    CallbackData *td_ptr = data;
    td_ptr->called      = TRUE;
    td_ptr->n_calls++;
    td_ptr->et_the_same = td_ptr->et == et;
}

static void
cal_point_start_cb(GEyeEyetracker* et, gdouble x, gdouble y, gpointer data)
{
    (void) x, (void) y;
    CallbackData *td_ptr = data;
    td_ptr->called = TRUE;
    td_ptr->n_calls++;
    td_ptr->et_the_same = td_ptr->et == et;
}

static void
eyelink_calibrate(void)
{
    GEyeEyelinkEt   *el;
    GEyeEyetracker  *et;
    GError          *error = NULL;

    el = geye_eyelink_et_new();
    et = GEYE_EYETRACKER(el);

    CallbackData data_start         = {.et = et};
    CallbackData data_stop          = {.et = et};
    CallbackData data_point_start   = {.et = et};
    CallbackData data_point_stop    = {.et = et};

    geye_eyetracker_set_calibration_start_cb(et, cal_start_stop_cb, &data_start);
    geye_eyetracker_set_calibration_stop_cb(et, cal_start_stop_cb, &data_stop);
    geye_eyetracker_set_calpoint_start_cb(et, cal_point_start_cb, &data_point_start);
    geye_eyetracker_set_calpoint_stop_cb(et, cal_start_stop_cb, &data_point_stop);

    geye_eyetracker_connect(et, &error);
    g_assert_no_error(error);
    g_usleep(100000);

    geye_eyetracker_calibrate(et, &error);
    g_assert_no_error(error);

    for (guint i = 0; i < geye_eyetracker_get_num_calpoints(et); ++i) {
        g_usleep((guint64)2.5e5);
        // geye_eyetracker_trigger_calpoint(et, &error);
        g_assert_no_error(error);
    }
    g_usleep((guint)2.5e5);

    g_assert_true(data_start.et_the_same);
    // TODO know error, with the eyelink it is difficult to determine when
    // the last calibration dot is presented.
    // g_assert_true(data_stop.et_the_same);
    g_assert_true(data_point_start.et_the_same);
    g_assert_true(data_point_stop.et_the_same);

    geye_eyelink_et_destroy(el);
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");
    g_test_init(&argc, &argv, NULL);

    int one = 1, two = 2, four=4, eight = 8;

    g_log_set_always_fatal(G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION);

    g_test_add_func("/EyelinkEt/create",  eyelink_create);

    g_test_add("/EyelinkEt/connect",
               EyelinkFixture,
               &four,
               eyelink_fixture_setup,
               eyelink_connect,
               eyelink_fixture_tear_down);

    g_test_add_func(
            "/EyelinkEt/disconnect", eyelink_disconnect
    );
    g_test_add(
            "/EyelinkEt/connect_simulated",
            EyelinkFixture,
            &one,
            eyelink_fixture_setup,
            eyelink_connect_simulated,
            eyelink_fixture_tear_down
            );
    g_test_add_func(
            "/EyelinkEt/start_tracking", eyelink_tracking
    );


    //g_test_add_func(
    //        "/EyelinkEt/start_recording", eyelink_recording
    //);

    g_test_add_func(
            "/EyelinkEt/run_setup", eyelink_run_setup
            );

// This test doesn't work...
//    g_test_add_func(
//            "/EyelinkEt/run_setup_exit_escape", eyelink_run_setup_exit_escape
//    );

    g_test_add_func("/EyelinkEt/calibrate", eyelink_calibrate);

    return g_test_run();
}
