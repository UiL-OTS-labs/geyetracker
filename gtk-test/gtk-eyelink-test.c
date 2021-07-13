
#include <glib.h>
#include <gtk/gtk.h>
#include <geye.h>

const char* supported_eyetrackers[] = {
    "none",
    "eyelink"
};

typedef struct CalData {
    gboolean    needs_dot;
    gdouble     x, y;
} CalData;

typedef struct EyetrackerData {
    gchar          *chosen_et;
    GEyeEyetracker *et;
    gboolean        is_calibrating;
    CalData         cal_data;       // Specifies the calibration state.
    GtkWidget*      darea;          // The GtkDrawingArea for cal graphics.
} EyetrackerData;

gboolean
window_quit(GtkWidget* window, GdkEvent* event, gpointer data)
{
    (void) window, (void)event;
    g_print("%s data = %p\n", __func__, data);
    EyetrackerData *testdata = data;
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

    gboolean send = geye_eyetracker_send_key_press(
            et,
            key,
            state
            );
    return TRUE;
}

typedef struct CalpointPars {
    GEyeEyetracker *et;
    gboolean needs_dot;
    gdouble x;
    gdouble y;
    gpointer data;
} CalpointPars;

gboolean
on_calpoint_start(gpointer data)
{
    CalpointPars *pars = data;
    EyetrackerData *testdata = pars->data;
    testdata->is_calibrating = TRUE;
    testdata->cal_data.needs_dot = pars->needs_dot;
    testdata->cal_data.x = pars->x;
    testdata->cal_data.y = pars->y;

    g_free(data);

    gtk_widget_queue_draw(testdata->darea);

    return G_SOURCE_REMOVE;
}

gboolean
on_calpoint_stop(gpointer data)
{
    CalpointPars *pars = data;
    EyetrackerData *testdata = pars->data;
    testdata->is_calibrating = TRUE;
    testdata->cal_data.needs_dot = pars->needs_dot;
    testdata->cal_data.x = pars->x;
    testdata->cal_data.y = pars->y;

    g_free(data);

    gtk_widget_queue_draw(testdata->darea);

    return G_SOURCE_REMOVE;
}

void
calpoint_start(GEyeEyetracker* et, gdouble x, gdouble y, gpointer data)
{
    CalpointPars * pars = g_malloc0(sizeof(CalpointPars));
    pars->et = et;
    pars->x = x;
    pars->y = y;
    pars->needs_dot = TRUE;
    pars->data = data;
    GSource* calpoint_source = g_idle_source_new();
    g_source_set_callback(
            calpoint_source,
            G_SOURCE_FUNC(on_calpoint_start),
            pars,
            NULL
            );
    g_source_attach(calpoint_source, g_main_context_default());
}

void
calpoint_stop(GEyeEyetracker* et, gpointer data)
{
    CalpointPars * pars = g_malloc0(sizeof(CalpointPars));
    pars->et = et;
    pars->x = 0;
    pars->y = 0;
    pars->needs_dot = FALSE;
    pars->data = data;
    GSource* calpoint_source = g_idle_source_new();
    g_source_set_callback(
            calpoint_source,
            G_SOURCE_FUNC(on_calpoint_stop),
            pars,
            NULL
    );
    g_source_attach(calpoint_source, g_main_context_default());
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

GEyeEyetracker *
setup_eyelink()
{
    GEyeEyelinkEt *et;

    et = geye_eyelink_et_new();
    return GEYE_EYETRACKER(et);
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

            geye_eyetracker_connect(testdata->et, &error);
            if (error) {
                g_printerr(
                        "Unable to connect to eyetracker: %s",
                        error->message
                        );
                g_clear_error(&error);
            }
            else {
                geye_eyetracker_set_calpoint_start_cb(
                        testdata->et, calpoint_start, testdata
                        );
                geye_eyetracker_set_calpoint_stop_cb(
                        testdata->et, calpoint_stop, testdata
                        );
            }
        }
    }
}

gboolean
on_draw(GtkWidget* widget, cairo_t *cr, gpointer data) {
    guint width, height;
    EyetrackerData* testdata = data;

    double circle_rad;
    double s_circle_rad;

    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(context, cr, 0, 0, width, height);

    circle_rad = width/50;
    s_circle_rad = height/200;

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    if (testdata->is_calibrating && testdata->cal_data.needs_dot) {
        cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
        cairo_arc(
                cr,
                testdata->cal_data.x,
                testdata->cal_data.y,
                circle_rad,
                0,
                2 * 3.1415);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_arc(
                cr,
                testdata->cal_data.x,
                testdata->cal_data.y,
                s_circle_rad,
                0,
                2 * 3.1415);
        cairo_close_path(cr);
        cairo_fill(cr);

    }
    return FALSE;
}

int main(int argc, char** argv)
{
    GtkWidget* window, *darea, *cal_button, *val_button, *et_combo;
    GtkWidget* setup_button, *grid;
    gtk_init(&argc, &argv);

    EyetrackerData testdata = {
        .is_calibrating = FALSE,
    };

    g_print("testdata is at %p\n", (void*)&testdata);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    grid = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    cal_button = gtk_button_new_with_label("calibrate");
    val_button = gtk_button_new_with_label("validate");
    setup_button = gtk_button_new_with_label("setup");
    darea = gtk_drawing_area_new();
    testdata.darea = darea;

    GdkEventMask mask = gtk_widget_get_events(darea);
    gtk_widget_set_events(darea,
                          GDK_KEY_PRESS_MASK | mask
                          );

    gtk_widget_set_size_request(darea, 900, 900);
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

    for (size_t i = 0;
         i < sizeof(supported_eyetrackers)/sizeof(supported_eyetrackers[0]);
         ++i) {
        gtk_combo_box_text_append(
                GTK_COMBO_BOX_TEXT(et_combo),
                "Eyetracker",
                supported_eyetrackers[i]);
    }

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

    gtk_widget_set_can_focus(darea, TRUE);
    gtk_widget_grab_focus(darea);

    gtk_combo_box_set_active(GTK_COMBO_BOX(et_combo), 0);
    gtk_widget_show_all(window);

    gtk_main();

    return TRUE;
}
