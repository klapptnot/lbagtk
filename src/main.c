#define _DEFAULT_SOURCE
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

#include "gio/gio.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtkshortcut.h"
#include "pango/pango-attributes.h"

#define UNUSED __attribute__ ((unused))
#define GTK_APP_CLASS_OR_ID "xyz.gall.lgagtk"
#define DEFAULT_STYLES_PATH "~/.config/gall/lbagtk.css"
#define DEFAULT_BUTTON_TEXT "Hibernate"
#define DEFAULT_BUTTON_CMDL "systemctl hibernate"

typedef struct {
  GString *bp_path;
  GString *bs_path;
  int percentage;
  bool is_charging;
  bool hide4now;
  bool hide4all;
} AppData;

typedef struct {
  GtkWindow *window;
  GtkLabel *lbl_close;
  GtkWidget *btn_close;
  GtkWidget *btn_2ndry;
} AppTree;

typedef struct {
  char *css_file;
  char *btn_str;
  char *btn_cmd;
  int low_lvl;
  int risk_lvl;
} AppOpts;

typedef struct {
  AppData *data;
  AppTree *tree;
  AppOpts *opts;
  GtkApplication *gapp;
} AppCtx;

static const int WINDOW_WIDTH = 480;
static const int WINDOW_HEIGHT = 180;

GString *find_battery_path (const char *file_type) {
  GString *path = g_string_sized_new (64);

  g_string_append (path, "/sys/class/power_supply/BAT");
  gint cur_len = path->len;
  for (int bat_num = 0; bat_num < 10; bat_num++) {
    // /sys/class/power_supply/BAT{num}/{file_type}
    g_string_append_c (path, '0' + bat_num);
    g_string_append_c (path, '/');
    g_string_append (path, file_type);

    FILE *fp = fopen (path->str, "r");
    if (fp != NULL) {
      fclose (fp);
      return path;
    }

    g_string_truncate (path, cur_len);
  }

  g_string_truncate (path, 0);
  return path;
}

int get_battery_percentage (char *path) {
  FILE *capacity_file = fopen (path, "r");
  if (!capacity_file) {
    perror ("Failed to open battery capacity file");
    return -1;
  }

  int percentage;
  if (fscanf (capacity_file, "%d", &percentage) != 1) {
    fclose (capacity_file);
    return -1;
  }

  fclose (capacity_file);
  return percentage;
}

bool is_battery_charging (char *path) {
  FILE *status_file = fopen (path, "r");
  if (!status_file) {
    return NULL;
  }

  static char status[32];
  if (fgets (status, sizeof (status), status_file)) {
    status[strcspn (status, "\n")] = 0;
  }

  fclose (status_file);
  return (strncmp (status, "Charging", 8) == 0);
}

char *path_userexpand (const char *path) {
  if (path[0] == '~') {
    const char *home = g_get_home_dir ();
    if (home) {
      char *expanded = g_build_filename (home, path + 1, NULL);
      return expanded;
    }
  }

  return g_strdup (path);
}

static void load_css_styles (char *path) {
  if (!path || !g_file_test (path, G_FILE_TEST_EXISTS)) {
    g_print ("CSS file not found or path not set: %s\n", path ? path : "NULL");
    return;
  }

  GtkCssProvider *css_provider = gtk_css_provider_new ();

  gtk_css_provider_load_from_path (css_provider, path);

  gtk_style_context_add_provider_for_display (gdk_display_get_default (), GTK_STYLE_PROVIDER (css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_object_unref (css_provider);
}

static gboolean on_timeout_updates (gpointer user_data) {
  AppCtx *app_ctx = (AppCtx *)user_data;
  AppData *app_data = app_ctx->data;

  app_data->percentage = get_battery_percentage (app_data->bp_path->str);
  app_data->is_charging = is_battery_charging (app_data->bs_path->str);

  return TRUE;
}

static gboolean on_timeout_toggle (gpointer user_data) {
  AppCtx *app_ctx = (AppCtx *)user_data;
  AppData *app_data = app_ctx->data;
  AppTree *app_tree = app_ctx->tree;
  AppOpts *app_opts = app_ctx->opts;

  gboolean is_visible = gtk_widget_get_visible (GTK_WIDGET (app_tree->window));

  if (app_data->percentage > app_opts->low_lvl || app_data->is_charging) {
    gtk_widget_set_visible (GTK_WIDGET (app_tree->window), FALSE);
    app_data->hide4now = false;
    app_data->hide4all = false;
    return G_SOURCE_CONTINUE;
  }
  if ((app_data->hide4now && app_data->percentage > app_opts->risk_lvl) || app_data->hide4all) return G_SOURCE_CONTINUE;

  if (app_data->percentage > app_opts->risk_lvl)
    gtk_widget_set_sensitive (GTK_WIDGET (app_tree->btn_2ndry), FALSE);
  else
    gtk_widget_set_sensitive (GTK_WIDGET (app_tree->btn_2ndry), TRUE);

  if (!is_visible) {
    gtk_widget_set_visible (GTK_WIDGET (app_tree->window), TRUE);
    gtk_window_present (GTK_WINDOW (app_tree->window));
    gtk_widget_grab_focus (app_tree->btn_close);
    load_css_styles (app_ctx->opts->css_file);
  }

  gtk_label_set_text (GTK_LABEL (app_tree->lbl_close),
                      g_strdup_printf ("You may want to charge it soon. Current battery level: %d%%", app_data->percentage));

  return G_SOURCE_CONTINUE;
}

static void cb_secondary_btn (gpointer user_data) {
  AppCtx *app_ctx = (AppCtx *)user_data;

  app_ctx->data->hide4all = true;
  int exitc = system (app_ctx->opts->btn_cmd);

  if (exitc == 0) {
    gtk_widget_set_visible (GTK_WIDGET (app_ctx->tree->window), FALSE);
    app_ctx->data->hide4all = true;
  }
}

static void action_hide_contextual (gpointer user_data) {
  AppCtx *app_ctx = (AppCtx *)user_data;

  if (app_ctx->data->hide4now || app_ctx->data->percentage <= app_ctx->opts->risk_lvl) app_ctx->data->hide4all = true;

  app_ctx->data->hide4now = true;
  gtk_widget_set_visible (GTK_WIDGET (app_ctx->tree->window), FALSE);
}

gboolean on_key_pressed (UNUSED GtkEventControllerKey *controller, guint keyval, UNUSED guint keycode,
                         UNUSED GdkModifierType state, gpointer user_data) {
  if (keyval == GDK_KEY_Escape) {
    action_hide_contextual (user_data);
    return TRUE;
  }
  return FALSE;
}

static void on_activate (GtkApplication *app, gpointer user_data) {
  AppCtx *app_ctx = g_new (AppCtx, 1);
  app_ctx->data = g_new (AppData, 1);
  app_ctx->tree = g_new (AppTree, 1);
  AppData *app_data = app_ctx->data;
  AppTree *app_tree = app_ctx->tree;

  app_ctx->gapp = app;
  app_ctx->opts = (AppOpts *)user_data;

  if (app_ctx->opts->css_file) {
    char *expanded_styles = path_userexpand (app_ctx->opts->css_file);
    g_free (app_ctx->opts->css_file);
    app_ctx->opts->css_file = expanded_styles;
  }

  app_data->bs_path = find_battery_path ("status");
  app_data->bp_path = find_battery_path ("capacity");
  app_data->percentage = 100;
  app_data->is_charging = false;
  app_data->hide4now = false;
  app_data->hide4all = false;

  if (!app_data->bs_path->len || !app_data->bp_path->len) {
    g_print ("No battery file found, exiting...\n");
    g_string_free (app_data->bs_path, TRUE);
    g_string_free (app_data->bp_path, TRUE);
    g_free (app_ctx->opts->css_file);
    g_free (app_ctx->opts);
    g_free (app_ctx->tree);
    g_free (app_ctx->data);
    g_free (app_ctx);

    g_application_quit (G_APPLICATION (app));
    return;
  }

  load_css_styles (app_ctx->opts->css_file);

  PangoAttrList *h1_attr = pango_attr_list_new ();
  pango_attr_list_insert (h1_attr, pango_attr_size_new (16 * PANGO_SCALE));

  app_tree->window = GTK_WINDOW (gtk_application_window_new (app));
  GtkWindow *window = app_tree->window;

  gtk_window_set_title (GTK_WINDOW (window), "Low Battery Alert");

  gtk_window_set_default_size (GTK_WINDOW (window), WINDOW_WIDTH, WINDOW_HEIGHT);
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window), NULL);

  gtk_widget_add_css_class (GTK_WIDGET (window), "battery-alert-window");

  GtkWidget *notice_label = gtk_label_new ("Battery is running low");
  gtk_label_set_attributes (GTK_LABEL (notice_label), (PangoAttrList *)h1_attr);
  gtk_label_set_xalign (GTK_LABEL (notice_label), 0.0);
  gtk_widget_add_css_class (notice_label, "battery-alert-header");

  app_tree->lbl_close = GTK_LABEL (gtk_label_new ("None"));
  GtkWidget *label_info = GTK_WIDGET (app_tree->lbl_close);
  gtk_label_set_xalign (GTK_LABEL (label_info), 0.0);
  gtk_widget_add_css_class (label_info, "battery-alert-info");

  app_tree->btn_2ndry = gtk_button_new_with_label (app_ctx->opts->btn_str);
  GtkWidget *sleep_btn = app_tree->btn_2ndry;
  gtk_widget_add_css_class (sleep_btn, "battery-alert-hibernate-btn");

  app_tree->btn_close = gtk_button_new_with_label ("Got it!");
  GtkWidget *close_btn = app_tree->btn_close;
  gtk_widget_add_css_class (close_btn, "battery-alert-close-btn");

  GtkWidget *box_main = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_set_homogeneous (GTK_BOX (box_main), FALSE);
  gtk_widget_add_css_class (box_main, "battery-alert-main-container");

  GtkWidget *box_label = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_set_homogeneous (GTK_BOX (box_label), FALSE);
  gtk_widget_add_css_class (box_label, "battery-alert-label-container");

  GtkWidget *box_btns = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_set_homogeneous (GTK_BOX (box_btns), TRUE);
  gtk_widget_add_css_class (box_btns, "battery-alert-button-container");

  gtk_box_append (GTK_BOX (box_label), notice_label);
  gtk_box_append (GTK_BOX (box_label), label_info);

  gtk_box_append (GTK_BOX (box_btns), sleep_btn);
  gtk_box_append (GTK_BOX (box_btns), close_btn);

  gtk_box_append (GTK_BOX (box_main), box_label);
  gtk_box_append (GTK_BOX (box_main), box_btns);

  gtk_window_set_child (GTK_WINDOW (window), box_main);

  g_signal_connect_swapped (sleep_btn, "clicked", G_CALLBACK (cb_secondary_btn), app_ctx);
  g_signal_connect_swapped (close_btn, "clicked", G_CALLBACK (action_hide_contextual), app_ctx);

  g_timeout_add (1000, (GSourceFunc)on_timeout_updates, app_ctx);
  g_timeout_add (1000, (GSourceFunc)on_timeout_toggle, app_ctx);

  GtkEventController *key_controller = gtk_event_controller_key_new ();
  gtk_widget_add_controller (GTK_WIDGET (window), key_controller);
  g_signal_connect (key_controller, "key-pressed", G_CALLBACK (on_key_pressed), app_ctx);

  g_signal_connect_swapped (window, "close-request", G_CALLBACK (action_hide_contextual), app_ctx);

  gtk_window_present (GTK_WINDOW (window));
  gtk_widget_set_visible (GTK_WIDGET (window), FALSE);
}

static AppOpts *parse_arguments (int *argc, char ***argv, gboolean *will_daemonize) {
  AppOpts *opts = g_new0 (AppOpts, 1);
  GError *error = NULL;
  GOptionContext *context;

  opts->css_file = g_strdup (DEFAULT_STYLES_PATH);
  opts->btn_str = g_strdup (DEFAULT_BUTTON_TEXT);
  opts->btn_cmd = g_strdup (DEFAULT_BUTTON_CMDL);
  opts->low_lvl = 20;
  opts->risk_lvl = 10;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  GOptionEntry entries[] = {
      {"daemon", 'D', 0, G_OPTION_ARG_NONE, will_daemonize, "Run as a background, dettached process", NULL},
      {"styles", 's', 0, G_OPTION_ARG_FILENAME, &opts->css_file, "Path to styles file", "FILE"},
      {"low", 'l', 0, G_OPTION_ARG_INT, &opts->low_lvl, "Low battery level (def: 20)", "LEVEL"},
      {"risky", 'r', 0, G_OPTION_ARG_INT, &opts->risk_lvl, "Risky battery level, activating secondary button (def: 10)", "LEVEL"},
      {"btn", 'b', 0, G_OPTION_ARG_STRING, &opts->btn_str, "Secondary button text (def: 'Hibernate')", "STR"},
      {"btn-cmd", 'B', 0, G_OPTION_ARG_STRING, &opts->btn_cmd, "Secondary button command (def: 'systemctl hibernate')", "CMD"},
      {NULL}};
#pragma GCC diagnostic pop

  context = g_option_context_new ("- Low battery notifier");
  g_option_context_add_main_entries (context, entries, NULL);

  if (!g_option_context_parse (context, argc, argv, &error)) {
    g_print ("Option parsing failed: %s\n", error->message);
    g_error_free (error);
    g_option_context_free (context);
    g_free (opts->css_file);
    g_free (opts);
    return NULL;
  }

  g_option_context_free (context);
  return opts;
}

int main (int argc, char *argv[]) {
  gboolean will_daemonize = FALSE;
  AppOpts *opts = parse_arguments (&argc, &argv, &will_daemonize);
  if (!opts) return 1;

  g_print ("Styles   : %s\n", opts->css_file);
  g_print ("Low      : %d\n", opts->low_lvl);
  g_print ("Critical : %d\n", opts->risk_lvl);
  g_print ("Label    : %s\n", opts->btn_str);
  g_print ("Command  : %s\n", opts->btn_cmd);

  GtkApplication *app = gtk_application_new (GTK_APP_CLASS_OR_ID, G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect (app, "activate", G_CALLBACK (on_activate), opts);

  if (will_daemonize)
    if (daemon (0, 0) == -1) {
      perror ("daemon()");
      exit (EXIT_FAILURE);
    }

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}
