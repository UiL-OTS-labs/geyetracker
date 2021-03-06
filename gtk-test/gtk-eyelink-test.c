
#include <glib.h>
#include <gtk/gtk.h>
#include <geye.h>
#include <math.h>

const char* supported_eyetrackers[] = {
    "none",
    "eyelink"
};

typedef struct CalData {
    gboolean    needs_dot;
    gdouble     x, y;
} CalData;

typedef struct ImageData {
    cairo_surface_t    *surf;
}ImageData;

typedef struct EyetrackerData {
    gchar          *chosen_et;
    GEyeEyetracker *et;
    gboolean        is_calibrating;
    gboolean        is_cam_setup;
    gboolean        is_tracking;

    GEyeSample     *leye_sample;
    GEyeSample     *reye_sample;

    GtkWidget      *cal_button, *val_button, *setup_button, *tracking_toggle;

    CalData         cal_data;       // Specifies the calibration state.
    ImageData       img_data;

    GtkWidget      *darea;          // The GtkDrawingArea for cal graphics.
    GtkWidget      *error_label;    // a label for error messages.
} EyetrackerData;

typedef struct ImagePars {
    GEyeEyetracker *et;
    cairo_surface_t* surf;
    gpointer data;
} ImagePars;

ImagePars*
image_pars_create(
        GEyeEyetracker* et, cairo_surface_t* surf, gpointer data
)
{
    ImagePars *pars = g_malloc(sizeof(ImagePars));
    if (!pars)
        return NULL;
    pars->et = et;
    pars->surf = surf;
    pars->data = data;

    return pars;
}

void
image_pars_destroy(ImagePars* pars)
{
    if (pars)
        cairo_surface_destroy(pars->surf);
    g_free(pars);
}

gboolean
window_quit(GtkWidget* window, GdkEvent* event, gpointer data)
{
    (void) window, (void)event;
    g_print("%s data = %p\n", __func__, data);
    EyetrackerData *testdata = data;
    if (testdata->et != NULL)
        g_object_unref(testdata->et);
    g_free(testdata->chosen_et);
    gtk_main_quit();
    return TRUE;
}

gboolean
on_key_press(GtkWidget* darea, GdkEvent* event, gpointer data)
{
    (void) darea;
    EyetrackerData *testdata = data;
    GEyeEyetracker *et = testdata->et;

    GdkEventType type = event->type;
    g_assert(type == GDK_KEY_PRESS);

    guint state = event->key.state & (
            GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK
            );
    guint16 key = event->key.keyval;

    geye_eyetracker_send_key_press(
            et,
            key,
            state
            );
    return TRUE;
}

void
on_calpoint_start(GEyeEyetracker* et, gdouble x, gdouble y, gpointer data)
{
    (void) et;
    EyetrackerData *testdata = data;

    testdata->is_calibrating = TRUE;
    testdata->cal_data.needs_dot = TRUE;
    testdata->cal_data.x = x;
    testdata->cal_data.y = y;

    gtk_widget_queue_draw(testdata->darea);
}

void
on_calpoint_stop(GEyeEyetracker* et, gpointer data)
{
    (void) et;
    EyetrackerData *testdata = data;
    testdata->is_calibrating = TRUE;
    testdata->cal_data.needs_dot = FALSE;

    gtk_widget_queue_draw(testdata->darea);
}

void
on_sample(GEyeEyetracker* et, const GEyeSample *sample, gpointer data)
{
    (void) et;
    EyetrackerData *testdata = data;
    testdata->is_tracking = TRUE;

    if (sample->parent.eye == GEYE_LEFT) {
        if (testdata->leye_sample)
            geye_sample_free(testdata->leye_sample);
        testdata->leye_sample = geye_sample_copy(sample);
    }
    else if (sample->parent.eye == GEYE_RIGHT) {
        if (testdata->reye_sample)
            geye_sample_free(testdata->reye_sample);
        testdata->reye_sample = geye_sample_copy(sample);
    }
    gtk_widget_queue_draw(testdata->darea);
}

gboolean
on_setup_image(gpointer data) {
    ImagePars *pars = data;
    EyetrackerData *testdata = pars->data;
    testdata->is_cam_setup = TRUE;

    // if there is a surface clear it.
    if(testdata->img_data.surf){
        cairo_surface_destroy(testdata->img_data.surf);
        testdata->img_data.surf = NULL;
    }

    testdata->img_data.surf = cairo_surface_reference(pars->surf);
    // Don't do the next as it is destroy by the g_source's destroy notify.
    // image_pars_destroy(pars);
    gtk_widget_queue_draw(testdata->darea);
    return G_SOURCE_REMOVE;
}

void
setup_image(
        GEyeEyetracker* et,
        guint           width,
        guint           height,
        gsize           img_buf_sz,
        guint8*         img_data,
        gpointer        data
        )
{
    cairo_surface_t *surf = cairo_image_surface_create(
            CAIRO_FORMAT_RGB24, width, height);
    cairo_status_t status = cairo_surface_status(surf);
    if (status != CAIRO_STATUS_SUCCESS) {
        g_warning("Unable to create cairo suface %s",
                  cairo_status_to_string(status));
        cairo_surface_destroy(surf);
        return;
    }

    memcpy(cairo_image_surface_get_data(surf), img_data, img_buf_sz);
    ImagePars *pars = image_pars_create(
            et, surf, data
            );
    if (!pars) {
        cairo_surface_destroy(surf);
        return;
    }

    GSource* img_source = g_idle_source_new();
    g_source_set_callback (
            img_source,
            G_SOURCE_FUNC(on_setup_image),
            pars,
            (GDestroyNotify) image_pars_destroy
            );

    g_source_attach(img_source, g_main_context_default());
}

void
cal_button_clicked(GtkWidget* button, gpointer data)
{
    (void) button;
    g_print("%s data = %p\n", __func__, data);

    EyetrackerData* testdata = data;

    gint w = gtk_widget_get_allocated_width(testdata->darea);
    gint h = gtk_widget_get_allocated_height(testdata->darea);

    if (g_strcmp0(testdata->chosen_et, supported_eyetrackers[1]) == 0) {
        geye_eyelink_et_set_display_dimensions(
                GEYE_EYELINK_ET(testdata->et), w, h
                );
    }

    gtk_widget_grab_focus(testdata->darea);

    geye_eyetracker_calibrate(testdata->et, NULL);
}

void
val_button_clicked(GtkWidget* button, gpointer data)
{
    (void) button;
    g_print("%s data = %p\n", __func__, data);

    EyetrackerData* testdata = data;

    gint w = gtk_widget_get_allocated_width(testdata->darea);
    gint h = gtk_widget_get_allocated_height(testdata->darea);

    if (g_strcmp0(testdata->chosen_et, supported_eyetrackers[1]) == 0) {
        geye_eyelink_et_set_display_dimensions(
                GEYE_EYELINK_ET(testdata->et), w, h
        );
    }

    gtk_widget_grab_focus(testdata->darea);

    geye_eyetracker_validate(testdata->et, NULL);
}

void
setup_button_clicked(GtkWidget* button, gpointer data)
{
    (void) button;
    g_print("%s data = %p\n", __func__, data);

    EyetrackerData *testdata = data;

    gtk_widget_grab_focus(testdata->darea);
    geye_eyetracker_start_setup(testdata->et);
}

void
on_tracking_toggled(GtkWidget* tbutton, gpointer data)
{
    EyetrackerData *testdata = data;
    GtkToggleButton *toggle = GTK_TOGGLE_BUTTON(tbutton);
    gboolean active = gtk_toggle_button_get_active(toggle);

    if (active) {
        GError *error = NULL;
        char buffer[256];
        geye_eyetracker_start_tracking(testdata->et, &error);
        if (error) {
            g_snprintf(buffer, sizeof(buffer),
                       "Unable to start tracking: %s", error->message);
            gtk_label_set_label(GTK_LABEL(testdata->error_label), buffer);
            g_error_free(error);
            error = NULL;
        }
    }
    else {
        geye_eyetracker_stop_tracking(testdata->et);
    }
}

GEyeEyetracker *
setup_eyelink()
{
    GEyeEyelinkEt *et;

    et = geye_eyelink_et_new();
    return GEYE_EYETRACKER(et);
}

void
on_et_connected(GEyeEyetracker* et, gboolean connected, gpointer data)
{
    EyetrackerData *testdata = data;

    gtk_widget_set_sensitive(testdata->cal_button, connected);
    gtk_widget_set_sensitive(testdata->val_button, connected);
    gtk_widget_set_sensitive(testdata->setup_button, connected);
    gtk_widget_set_sensitive(testdata->tracking_toggle, connected);

    g_signal_connect(et,
                     "calpoint-start",
                     G_CALLBACK(on_calpoint_start),
                     data);
    g_signal_connect(et,
                     "calpoint-stop",
                     G_CALLBACK(on_calpoint_stop),
                     data);
    g_signal_connect(et,
                     "sample",
                     G_CALLBACK(on_sample),
                     data);

    if (connected == TRUE) {
        gchar buffer[1024];
        char* tracker_info = geye_eyetracker_get_tracker_info(et);
        g_snprintf(buffer, sizeof(buffer), "Connected to: %s", tracker_info);
        gtk_label_set_label(GTK_LABEL(testdata->error_label), buffer);
        g_free(tracker_info);
    }
}

void on_error(GEyeEyetracker* et, const gchar* message, gpointer data)
{
    (void) et;
    EyetrackerData *testdata = data;
    gtk_label_set_label(GTK_LABEL(testdata->error_label), message);
}

void
select_eyetracker(GtkComboBox* widget, gpointer data)
{
    g_print("%s data = %p\n", __func__, data);
    GError* error = NULL;
    EyetrackerData* testdata = data;
    char* et_name = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(widget)
            );
    if (g_strcmp0(et_name, testdata->chosen_et) != 0) {

        if (testdata->chosen_et)
            g_free(testdata->chosen_et);
        testdata->chosen_et = et_name;
        g_clear_object(&testdata->et);
        GEyeEyetracker* et = NULL;
        if (g_strcmp0(et_name, supported_eyetrackers[1]) == 0) {
            et = setup_eyelink();
        }
        else
            et = NULL;

        testdata->et = et;

        if (testdata->et) {

            g_signal_connect(
                    testdata->et,
                    "connected",
                    G_CALLBACK(on_et_connected),
                    testdata);
            g_signal_connect(
                    testdata->et,
                    "error",
                    G_CALLBACK(on_error),
                    testdata
                    );

            geye_eyetracker_connect(testdata->et, &error);
            if (error) {
                g_printerr(
                        "Unable to connect to eyetracker: %s",
                        error->message
                        );
                g_clear_error(&error);
            }
            else {
                geye_eyetracker_set_image_data_cb(
                        testdata->et, setup_image, testdata
                        );
            }
        }
    }
}

void
fill_circle(cairo_t* cr, gdouble x, gdouble y, gdouble radius, gdouble* color)
{
    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_move_to(cr, radius, 0.0);
    cairo_arc(cr, 0.0, 0.0, radius, 0.0, 2 * M_PI);
    cairo_set_source_rgb(cr, color[0], color[1], color[2]);
    cairo_fill(cr);
    cairo_restore(cr);
}

void
stroke_circle(cairo_t* cr, gdouble x, gdouble y, gdouble radius, gdouble* color)
{
    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_move_to(cr, radius, 0.0);
    cairo_arc(cr, 0.0, 0.0, radius, 0.0, 2 * M_PI);
    cairo_set_source_rgb(cr, color[0], color[1], color[2]);
    cairo_stroke(cr);
    cairo_restore(cr);
}

gboolean
on_draw(GtkWidget* widget, cairo_t *cr, gpointer data) {
    guint width, height;
    EyetrackerData* testdata = data;
    gdouble red[]    = {1.0, 0.0, 0.0};
    gdouble blue[]   = {0.0, 0.0, 1.0};
    gdouble yellow[] = {1.0, 1.0, 0.0};
    gdouble green[]  = {0.0, 1.0, 0.0};
    gdouble black[]  = {0.0, 0.0, 0.0};

    double circle_rad;
    double s_circle_rad;

    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(context, cr, 0, 0, width, height);

    circle_rad   = height/50.0;
    s_circle_rad = height/200.0;

    // clear background
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    if (testdata->is_calibrating && testdata->cal_data.needs_dot) {
        fill_circle(cr,
                    testdata->cal_data.x,
                    testdata->cal_data.y,
                    circle_rad,
                    blue);
        fill_circle(cr,
                    testdata->cal_data.x,
                    testdata->cal_data.y,
                    s_circle_rad,
                    yellow);
    }

    if (testdata->is_tracking) {
        gdouble radius = 20;
        if (testdata->leye_sample) {
            gdouble x = testdata->leye_sample->x;
            gdouble y = testdata->leye_sample->y;
            fill_circle(cr, x, y, radius, red);
            stroke_circle(cr, x, y, radius, black);
        }
        if (testdata->reye_sample) {
            gdouble x = testdata->reye_sample->x;
            gdouble y = testdata->reye_sample->y;
            fill_circle(cr, x, y, radius, green);
            stroke_circle(cr, x, y, radius, black);
        }
    }

    if (testdata->is_cam_setup){
        cairo_surface_t *tsurf = testdata->img_data.surf;
        double img_origin_x, img_origin_y;
        img_origin_x = width / 2.0 - cairo_image_surface_get_width(tsurf) / 2.0;
        img_origin_y = height / 2.0 - cairo_image_surface_get_height(tsurf) / 2.0;
        cairo_set_source_surface(
                cr,
                tsurf,
                img_origin_x,
                img_origin_y);
        cairo_rectangle(
                cr, img_origin_x,
                img_origin_y,
                cairo_image_surface_get_width(tsurf),
                cairo_image_surface_get_height(tsurf)
                );
        cairo_fill(cr);
        testdata-> is_cam_setup = FALSE;
    }

    return FALSE;
}

int main(int argc, char** argv)
{
    GtkWidget* window, *darea, *cal_button, *val_button, *et_combo;
    GtkWidget* setup_button, *grid, *error_label, *tracking_toggle;
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    grid = gtk_grid_new();

    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    cal_button = gtk_button_new_with_label("calibrate");
    val_button = gtk_button_new_with_label("validate");
    setup_button = gtk_button_new_with_label("setup");
    darea = gtk_drawing_area_new();
    error_label = gtk_label_new("Welcom to the gtk geyetracker test");
    tracking_toggle = gtk_toggle_button_new_with_label("tracking");
    gtk_widget_set_sensitive(cal_button, FALSE);
    gtk_widget_set_sensitive(val_button, FALSE);
    gtk_widget_set_sensitive(setup_button, FALSE);
    gtk_widget_set_sensitive(tracking_toggle, FALSE);
    gtk_widget_set_halign(error_label, GTK_ALIGN_START);

    GdkEventMask mask = gtk_widget_get_events(darea);
    gtk_widget_set_events(darea,
                          GDK_KEY_PRESS_MASK | mask
                          );

    gtk_widget_set_size_request(darea, 500, 500);
    gtk_widget_set_hexpand(darea, TRUE);
    gtk_widget_set_vexpand(darea, TRUE);
    gtk_widget_set_halign(darea, GTK_ALIGN_FILL);
    gtk_widget_set_valign(darea, GTK_ALIGN_FILL);

    et_combo = gtk_combo_box_text_new();

    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_attach(GTK_GRID(grid), darea, 0, 0, 5, 1);
    gtk_grid_attach(GTK_GRID(grid), et_combo, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), setup_button, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cal_button, 3, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), val_button, 4, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), error_label, 0, 2, 4, 1);
    gtk_grid_attach(GTK_GRID(grid), tracking_toggle, 4, 2, 1, 1);

    for (size_t i = 0;
         i < sizeof(supported_eyetrackers)/sizeof(supported_eyetrackers[0]);
         ++i) {
        gtk_combo_box_text_append(
                GTK_COMBO_BOX_TEXT(et_combo),
                "Eyetracker",
                supported_eyetrackers[i]);
    }

    EyetrackerData testdata = {
        .is_calibrating = FALSE,
        .cal_button = cal_button,
        .val_button = val_button,
        .setup_button = setup_button,
        .tracking_toggle = tracking_toggle,
        .darea = darea,
        .error_label = error_label
    };

    g_signal_connect(
            window,
            "delete-event",
            G_CALLBACK(window_quit),
            &testdata
            );
    g_signal_connect(
            cal_button,
            "clicked",
            G_CALLBACK(cal_button_clicked),
            &testdata
            );
    g_signal_connect(
            val_button,
            "clicked",
            G_CALLBACK(val_button_clicked),
            &testdata
            );
    g_signal_connect(
            setup_button,
            "clicked",
            G_CALLBACK(setup_button_clicked),
            &testdata
            );
    g_signal_connect(
            et_combo,
            "changed",
            G_CALLBACK(select_eyetracker),
            &testdata
            );
    g_signal_connect(
            darea,
            "draw",
            G_CALLBACK(on_draw),
            &testdata
            );
    g_signal_connect(
            darea,
            "key-press-event",
            G_CALLBACK(on_key_press),
            &testdata
            );
    g_signal_connect(
            tracking_toggle,
            "toggled",
            G_CALLBACK(on_tracking_toggled),
            &testdata
            );

    gtk_widget_set_can_focus(darea, TRUE);
    gtk_widget_grab_focus(darea);

    gtk_combo_box_set_active(GTK_COMBO_BOX(et_combo), 0);
    gtk_widget_show_all(window);

    gtk_main();

    return TRUE;
}
