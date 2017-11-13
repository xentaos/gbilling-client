/*
 *  gBilling is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
 *
 *  gBilling is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gBilling; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *	
 *  gui.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbilling.h"
#include "main.h"
#include "gui.h"
#include "callback.h"
#include "client.h"
#include "setting.h"
#include "version.h"

WindowInit *window_init = NULL;
WindowMain *window_main = NULL;
WindowLogin *window_login = NULL;
WindowTimer *window_timer = NULL;
WindowSetting *window_setting = NULL;
WindowChat *window_chat = NULL;
WindowItem *window_item = NULL;

guint label_istatsid = 0;
guint label_wlogtimesid;

static GtkBuilder *builder = NULL;

static void builder_init (const gchar*);
static void builder_destroy (void);
static GtkWidget* get_widget (const gchar*);

static gchar* get_pixmap_file (const gchar*);
static void set_window_icon (GtkWindow*);
static gboolean set_label_wlogtime (gpointer);
static gboolean animate_label_istat (gpointer);

static void
builder_init (const gchar *ui)
{
    g_return_if_fail (ui != NULL);
    gchar *file;
    guint retval;
    GError *error = NULL;

#ifdef PACKAGE_DATA_DIR
    file = g_build_filename (PACKAGE_DATA_DIR, "ui", ui, NULL);
#else
    file = g_build_filename ("share", "ui", ui, NULL);
#endif
    builder = gtk_builder_new ();
    retval = gtk_builder_add_from_file (builder, file, &error);
    g_free (file);
    if (!retval)
    {
        gbilling_debug ("%s: %s\n", __func__, error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }
}

static void
builder_destroy (void)
{
    g_object_unref (builder);
}

static GtkWidget*
get_widget (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    GObject *object;

    object = gtk_builder_get_object (builder, name);
    if (!object)
    {
        gbilling_debug ("%s: invalid widget '%s'\n", __func__, name);
        return NULL;
    }
    return GTK_WIDGET (object);
}

gchar*
get_data_file (const gchar *file)
{
    g_return_val_if_fail(file != NULL, NULL);

	gchar *tmp;

#ifdef PACKAGE_DATA_DIR
    tmp = g_build_filename (PACKAGE_DATA_DIR, file, NULL);
#else
    tmp = g_build_filename ("share", "data", file, NULL);
#endif

    return tmp;
}

static gchar*
get_pixmap_file (const gchar *file)
{
    g_return_val_if_fail (file != NULL, NULL);

    gchar *tmp;

#ifdef PACKAGE_PIXMAPS_DIR
    tmp = g_build_filename (PACKAGE_PIXMAPS_DIR, file, NULL);
#else
    tmp = g_build_filename ("share", "pixmaps", file, NULL);
#endif

    return tmp;
}

static void
set_window_icon (GtkWindow *window)
{
#ifndef _WIN32
    g_return_if_fail (window != NULL);
    static GdkPixbuf *pixbuf = NULL;
    GError *error = NULL;
    gchar *icon;
    
    if (!pixbuf)
    {
        icon = get_pixmap_file ("gbilling.png");
        pixbuf = gdk_pixbuf_new_from_file (icon, &error);
        g_free (icon);
        if (!pixbuf)
        {
            gbilling_debug ("%s: %s\n", __func__, error->message);
            g_error_free (error);
            return;
        }
    }
    gtk_window_set_icon (window, pixbuf);
#endif
}

static gboolean
set_label_wlogtime (gpointer data)
{
    time_t t = get_time ();
    struct tm *st = localtime (&t);
    static gchar buf[128];
    static gchar * const day[] =
    {
        "Minggu", 
        "Senin", 
        "Selasa", 
        "Rabu", 
        "Kamis", 
        "Jumat", 
        "Sabtu"
    };
    static gchar * const month[] =
    {
        "Januari", 
        "Februari", 
        "Maret", 
        "April", 
        "Mei", 
        "Juni", 
        "Juli",
        "Agustus", 
        "September", 
        "Oktober", 
        "November", 
        "Desember"
    };

    g_snprintf (buf, sizeof(buf), "%s, %i %s %i | %.2i:%.2i:%.2i", 
                day[st->tm_wday], st->tm_mday, month[st->tm_mon], 
                st->tm_year + 1900, st->tm_hour, st->tm_min, st->tm_sec);
#ifdef _WIN32
    set_markup ((GtkLabel *) data, GBILLING_FONT_MEDIUM, FALSE, buf);
#else
    set_markup ((GtkLabel *) data, GBILLING_FONT_SMALL, FALSE, buf);
#endif

    return TRUE;
}

static gboolean
animate_label_istat (gpointer data)
{
    gtk_progress_bar_pulse ((GtkProgressBar *) window_init->progressbar);
    return TRUE;
}

void
start_label_istat (void)
{
    label_istatsid = g_timeout_add (125, (GSourceFunc) animate_label_istat, NULL);
}

void
stop_label_istat (void)
{
    if (!g_source_remove (label_istatsid))
        gbilling_debug ("%s: Could not remove source\n", __func__);
}

void
create_window_init (void)
{
    GtkWidget *vbox;
    GdkColor bg;
    gchar *tmp;

    window_init = g_new (WindowInit, 1);

    window_init->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window_init->window, "Inisialisasi");
    gtk_window_set_position ((GtkWindow *) window_init->window, GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_type_hint ((GtkWindow *) window_init->window, 
                              GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_transient_for ((GtkWindow *) window_init->window, 
                                  (GtkWindow *) window_main->window);
    gtk_window_set_resizable ((GtkWindow *) window_init->window, FALSE);
    gtk_window_set_deletable ((GtkWindow *) window_init->window, FALSE);
    gdk_color_parse ("#0E71A6", &bg);
    gtk_widget_modify_bg (window_init->window, GTK_STATE_NORMAL, &bg);
    g_signal_connect ((GObject *) window_init->window, "delete-event",
                      (GCallback) gtk_true, NULL);
    
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) window_init->window, vbox);
    
    window_init->img = gtk_image_new ();
    tmp = get_data_file ("splash.png");
    gtk_image_set_from_file ((GtkImage *) window_init->img, tmp);
    g_free (tmp);
    gtk_container_add ((GtkContainer *) vbox, window_init->img);

    window_init->progressbar = gtk_progress_bar_new ();
    gtk_container_add ((GtkContainer *) vbox, window_init->progressbar);
    
    gtk_widget_show_all (vbox);
}

void
create_window_main (void)
{
    GdkColor color;
    gchar *tmp;
    gboolean wall;

    window_main = g_new (WindowMain, 1);

    window_main->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    tmp = get_pixmap_file ("gbilling.png");
    gtk_window_set_icon_from_file ((GtkWindow *) window_main->window, tmp, NULL);
    g_free (tmp);
    gtk_window_fullscreen ((GtkWindow *) window_main->window);
    gtk_window_set_title ((GtkWindow *) window_main->window, PROGRAM_NAME);
    tmp = g_key_file_get_string (key_file, DISPLAY_GROUP, DISPLAY_BGCOLOR_KEY, NULL);
    gdk_color_parse (tmp, &color);
    gtk_widget_modify_bg (window_main->window, GTK_STATE_NORMAL, &color);
    g_free (tmp);

    window_main->img = gtk_image_new_from_stock (GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG);
    gtk_container_add ((GtkContainer *) window_main->window, window_main->img);
    wall = g_key_file_get_boolean (key_file, DISPLAY_GROUP, DISPLAY_USEWALLPAPER_KEY, NULL);
    if (wall)
    {
        tmp = g_key_file_get_string (key_file, DISPLAY_GROUP, DISPLAY_WALLPAPER_KEY, NULL);
        if (g_file_test (tmp, G_FILE_TEST_EXISTS))
        {
            gtk_image_set_from_file ((GtkImage *) window_main->img, tmp);
            gtk_widget_show (window_main->img);
        }
        g_free (tmp);
    } else
        gtk_widget_hide (window_main->img);


    g_signal_connect ((GObject *) window_main->window, "delete-event", 
                      (GCallback) gtk_true, NULL);
    g_signal_connect ((GObject *) window_main->window, "show", 
                      (GCallback) on_window_main_show, NULL);
    /* g_signal_connect ((GObject *) window_main->window, "key-press-event", 
            (GCallback) on_window_login_key_press_event, NULL); */
}

gboolean
set_window_login (gpointer data)
{
    GList *ptr;
    GbillingPrepaid *prepaid;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint a;
    gchar *tmp;

    gtk_window_set_title ((GtkWindow *) window_login->window, "Login");
    tmp = g_ascii_strup (client->cname, -1);
    set_markup ((GtkLabel *) window_login->label_client, GBILLING_FONT_LARGE, TRUE, tmp);
    g_free (tmp);
    set_markup ((GtkLabel *) window_login->label_cafe, 
                GBILLING_FONT_X_LARGE, TRUE, server_info.name);
    set_markup ((GtkLabel *) window_login->label_desc, 
                GBILLING_FONT_LARGE, TRUE, server_info.desc);
#ifndef _WIN32
    set_markup ((GtkLabel *) window_login->label_address, 
                GBILLING_FONT_SMALL, FALSE, server_info.addr);
#else
    set_markup ((GtkLabel *) window_login->label_address,
                GBILLING_FONT_MEDIUM, FALSE, server_info.addr);
#endif

    tmp = g_strdup_printf ("%s\n%s\n%s\n", server_info.name, 
                           server_info.desc, server_info.addr);
    gtk_widget_set_tooltip_text (window_login->vbox_info, tmp);
    g_free (tmp);

    model = gtk_combo_box_get_model ((GtkComboBox *)  window_login->combob_prepaid);
    a = gtk_combo_box_get_active ((GtkComboBox *) window_login->combob_prepaid);
    gtk_list_store_clear ((GtkListStore *) model);

    for (ptr = prepaid_list; ptr; ptr = ptr->next)
    {
        prepaid = ptr->data;
        gtk_list_store_append ((GtkListStore *) model, &iter);
        gtk_list_store_set ((GtkListStore *) model, &iter, 0, prepaid->name, -1);
    }
    gtk_combo_box_set_active ((GtkComboBox *) window_login->combob_prepaid, 
                              a == -1 ? 0 : a);

    return FALSE;
}

void
window_login_grab_keyboard (void)
{
    if (gdk_keyboard_grab (window_login->window->window, TRUE, 
        GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
        g_warning ("grab failed\n");
}

void
create_window_login (void)
{
    GbillingAuth auth;
    gchar *tmp;
    
    builder_init ("login.ui");
    window_login = g_new (WindowLogin, 1);

    window_login->window = get_widget ("window");
    tmp = get_pixmap_file ("gbilling.png");
    gtk_window_set_icon_from_file ((GtkWindow *) window_login->window, tmp, NULL);
    g_free (tmp);
    gtk_window_set_title ((GtkWindow *) window_login->window, "Login");
    gtk_window_set_transient_for ((GtkWindow *) window_login->window, 
                                  (GtkWindow *) window_main->window);
    g_signal_connect ((GObject *) window_login->window, "delete-event",
                      (GCallback) gtk_true, NULL);

    window_login->vbox_info = get_widget ("vbox2");
    window_login->label_cafe = get_widget ("label_cafe");
    window_login->label_desc = get_widget ("label_desc");
    window_login->label_address = get_widget ("label_address");
    window_login->label_client = get_widget ("label_client");
    
    window_login->radiob_regular = get_widget ("radiob_regular");
    g_signal_connect ((GObject *) window_login->radiob_regular, "toggled", 
            (GCallback) on_radiob_wlogregular_toggled, NULL);
    
    window_login->radiob_prepaid = get_widget ("radiob_prepaid");
    g_signal_connect ((GObject *) window_login->radiob_prepaid, "toggled", 
            (GCallback) on_radiob_wlogpaket_toggled, NULL);
    
    window_login->radiob_admin = get_widget ("radiob_admin");
    g_signal_connect ((GObject *) window_login->radiob_admin, "toggled", 
            (GCallback) on_radiob_wlogadmin_toggled, NULL);

    window_login->entry_username = get_widget ("entry_username");
    gtk_entry_set_max_length ((GtkEntry *) window_login->entry_username, 
                              sizeof(auth.username) - 1);
    
    window_login->label_passwd = get_widget ("label_passwd");
    window_login->entry_passwd = get_widget ("entry_passwd");
    gtk_entry_set_max_length ((GtkEntry *) window_login->entry_passwd,
                              sizeof(auth.passwd) - 1);
                              
    window_login->label_prepaid = get_widget ("label_prepaid");
    window_login->combob_prepaid = get_widget ("combob_prepaid");
    
    window_login->button_start = get_widget ("button_start");
    g_signal_connect ((GObject *) window_login->button_start, "clicked",
                      (GCallback) on_button_wlogstart_clicked, NULL);
    
    window_login->button_restart = get_widget ("button_restart");
    g_signal_connect ((GObject *) window_login->button_restart, "clicked",
                      (GCallback) on_button_wlogrestart_clicked, NULL);
                      
    window_login->button_shutdown = get_widget ("button_shutdown");
    g_signal_connect ((GObject *) window_login->button_shutdown, "clicked",
                      (GCallback) on_button_wlogshutdown_clicked, NULL);
    
    window_login->label_time = get_widget ("label_time");
    set_label_wlogtime (window_login->label_time);
    label_wlogtimesid = g_timeout_add (1000, (GSourceFunc) set_label_wlogtime, 
                                       window_login->label_time);
    
    window_login->label_gbilling = get_widget ("label_gbilling");
    tmp = g_strdup_printf ("%s %s", PROGRAM_NAME, PROGRAM_VERSION);
    set_markup ((GtkLabel *) window_login->label_gbilling,
                GBILLING_FONT_SMALL, FALSE, tmp);
    g_free (tmp);

    on_radiob_wlogregular_toggled (NULL, NULL);
    
    builder_destroy ();
}

/*
 * Kirim perintah balasan sebagai tanda sukses enable/disable akses
 */
gboolean
set_button_wlogstart (gpointer data)
{
    gboolean access = GPOINTER_TO_INT (data);
    gint8 cmd;

    cmd = access ? GBILLING_COMMAND_ENABLE : GBILLING_COMMAND_DISABLE;
    send_command (cmd);
    gtk_widget_set_sensitive (window_login->button_start, access);

    return FALSE;
}

void
create_window_timer (void)
{
    gchar *tmp;

    builder_init ("timer.ui");
    window_timer = g_new (WindowTimer, 1);
    
    window_timer->window = get_widget ("window");
    tmp = get_pixmap_file ("gbilling.png");
    gtk_window_set_icon_from_file ((GtkWindow *) window_timer->window, tmp, NULL);
    g_signal_connect ((GObject *) window_timer->window, "delete-event", 
                      (GCallback) on_window_timer_delete_event, NULL);

    window_timer->label_client = get_widget ("label_client");
    tmp = g_ascii_strup (client->cname, -1);
    set_markup ((GtkLabel *) window_timer->label_client, 
                GBILLING_FONT_X_LARGE, TRUE, tmp);
    g_free (tmp);

    window_timer->entry_username = get_widget ("entry_username");
    window_timer->entry_type = get_widget ("entry_type");
    window_timer->entry_start = get_widget ("entry_start");
    window_timer->entry_duration = get_widget ("entry_duration");
    window_timer->entry_cost = get_widget ("entry_cost");
    
    window_timer->button_menu = get_widget ("button_menu");
    g_signal_connect ((GObject *) window_timer->button_menu, "clicked", 
                      (GCallback) on_button_menu_clicked, NULL);
            
    window_timer->button_chat = get_widget ("button_chat");
    g_signal_connect ((GObject *) window_timer->button_chat, "clicked", 
                      (GCallback) on_button_chat_clicked, NULL);
    
    window_timer->button_stop = get_widget ("button_stop");
    g_signal_connect ((GObject *) window_timer->button_stop, "clicked", 
                      (GCallback) on_button_stop_clicked, NULL);
    
    window_timer->button_about = get_widget ("button_about");
    g_signal_connect ((GObject *) window_timer->button_about, "clicked", 
                      (GCallback) on_button_about_clicked, NULL);
    
    builder_destroy ();
}

gboolean
hide_window_timer (gpointer data)
{
    if (window_timer)
        gtk_window_iconify ((GtkWindow *) window_timer->window);
    return FALSE;
}

void
create_window_setting (void)
{
    gchar *tmp;
    
    builder_init ("setting.ui");
    window_setting = g_new (WindowSetting, 1);

    window_setting->window = get_widget ("window");
    tmp = get_pixmap_file ("gbilling.png");
    gtk_window_set_icon_from_file ((GtkWindow *) window_setting->window, tmp, NULL);
    g_free (tmp);
    gtk_window_set_title ((GtkWindow *) window_setting->window, "Pengaturan Client");
    gtk_window_set_transient_for ((GtkWindow *) window_setting->window,
                                  (GtkWindow *) window_main->window);
    g_signal_connect ((GObject *) window_setting->window, "delete-event", 
                      (GCallback) on_window_admin_delete_event, NULL);

    window_setting->entry_ip = get_widget ("entry_ip");
    g_signal_connect ((GObject *) window_setting->entry_ip, "insert-text",
                      (GCallback) on_window_setting_entry_ip_insert_text, NULL);

    window_setting->spinb_port = get_widget ("spinb_port");
    
    window_setting->checkb_wallpaper = get_widget ("checkb_wallpaper");
    g_signal_connect ((GObject *) window_setting->checkb_wallpaper, "toggled",
                      (GCallback) on_checkb_adwall_toggled, NULL);
    
    window_setting->label_wallpaper = get_widget ("label_wallpaper");                  
    window_setting->entry_wallpaper = get_widget ("entry_wallpaper");
    
    window_setting->button_search = get_widget ("button_search");
    g_signal_connect ((GObject *) window_setting->button_search, "clicked",
                      (GCallback) on_button_adwall_clicked, NULL);
    
    window_setting->colorb_background = get_widget ("colorb_background");
    gtk_color_button_set_title ((GtkColorButton *) 
                                window_setting->colorb_background, "Pilih Warna");

    window_setting->button_exit = get_widget ("button_exit");
    g_signal_connect ((GObject *) window_setting->button_exit, "clicked",
                      (GCallback) on_button_adexit_clicked, NULL);
                      
    window_setting->button_restart = get_widget ("button_restart");
    g_signal_connect ((GObject *) window_setting->button_restart, "clicked",
                      (GCallback) on_button_adrestart_clicked, NULL);
                      
    window_setting->button_shutdown = get_widget ("button_shutdown");
    g_signal_connect ((GObject *) window_setting->button_shutdown, "clicked",
                      (GCallback) on_button_adshutdown_clicked, NULL);
                      
    window_setting->button_info = get_widget ("button_info");
    g_signal_connect ((GObject *) window_setting->button_info, "clicked",
                      (GCallback) on_button_adinfo_clicked, NULL);

    window_setting->entry_username = get_widget ("entry_username");
    g_signal_connect ((GObject *) window_setting->entry_username, "changed",
                      (GCallback) on_window_setting_entry_username_changed, NULL);

    window_setting->entry_passwd = get_widget ("entry_passwd");
    gtk_widget_set_sensitive (window_setting->entry_passwd, FALSE);
    g_signal_connect ((GObject *) window_setting->entry_passwd, "changed",
                      (GCallback) on_window_setting_entry_passwd_changed, NULL);

    window_setting->entry_passwdc = get_widget ("entry_passwdc");
    gtk_widget_set_sensitive (window_setting->entry_passwdc, FALSE);
    g_signal_connect ((GObject *) window_setting->entry_passwdc, "changed",
                      (GCallback) on_window_setting_entry_passwdc_changed, NULL);    

    window_setting->button_save = get_widget ("button_save");
    gtk_widget_set_sensitive (window_setting->button_save, FALSE);
    g_signal_connect ((GObject *) window_setting->button_save, "clicked",
                      (GCallback) on_window_setting_button_save_clicked, NULL);

    window_setting->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_setting->button_ok, "clicked",
                      (GCallback) on_button_adok_clicked, NULL);

    builder_destroy ();
}

void
set_window_setting (void)
{
    GdkColor bg;
    gchar *tmp;
    gint value;
    
    tmp = g_key_file_get_string (key_file, SERVER_GROUP, SERVER_IP_KEY, NULL);
    gtk_entry_set_text ((GtkEntry *) window_setting->entry_ip, tmp);
    g_free (tmp);
    
    value = g_key_file_get_integer (key_file, SERVER_GROUP, SERVER_PORT_KEY, NULL);
    gtk_spin_button_set_value ((GtkSpinButton *) window_setting->spinb_port, value);
 
    value = g_key_file_get_boolean (key_file, DISPLAY_GROUP, 
                                    DISPLAY_USEWALLPAPER_KEY, NULL);
    gtk_toggle_button_set_active ((GtkToggleButton *) 
                                  window_setting->checkb_wallpaper, value);
    /* set 'sensitive' widget yang relevan */
    on_checkb_adwall_toggled ((GtkCheckButton *) window_setting->checkb_wallpaper, NULL);

    tmp = g_key_file_get_string (key_file, DISPLAY_GROUP, DISPLAY_WALLPAPER_KEY, NULL);
    gtk_entry_set_text ((GtkEntry *) window_setting->entry_wallpaper, tmp);
    g_free (tmp);

    tmp = g_key_file_get_string (key_file, DISPLAY_GROUP, DISPLAY_BGCOLOR_KEY, NULL);
    gdk_color_parse (tmp, &bg);
    gtk_color_button_set_color ((GtkColorButton *) window_setting->colorb_background, &bg);
    g_free (tmp);

    tmp = g_key_file_get_string (key_file, SECURITY_GROUP, SECURITY_USERNAME, NULL);
    gtk_entry_set_text ((GtkEntry *) window_setting->entry_username, tmp);
    g_free (tmp);

    tmp = g_key_file_get_string (key_file, SECURITY_GROUP, SECURITY_PASSWORD, NULL);
    gtk_entry_set_text ((GtkEntry *) window_setting->entry_passwd, tmp);
    gtk_entry_set_text ((GtkEntry *) window_setting->entry_passwdc, tmp);
    g_free (tmp);

    gtk_widget_set_sensitive (window_setting->entry_passwd, FALSE);
    gtk_widget_set_sensitive (window_setting->entry_passwdc, FALSE);
    gtk_widget_set_sensitive (window_setting->button_save, FALSE);
}

void
create_window_item (void)
{
    GtkListStore *store;
    GtkCellRenderer *render_name;
    GtkCellRenderer *render_cost;
    GtkTreeViewColumn *column_name;
    GtkTreeViewColumn *column_cost;
    
    builder_init ("item.ui");
    window_item = g_new (WindowItem, 1);
    
    window_item->window = get_widget ("window");
    gtk_window_set_transient_for ((GtkWindow *) window_item->window,
                                  (GtkWindow *) window_timer->window);
    gtk_window_set_title ((GtkWindow *) window_item->window, "Daftar Menu");
    g_signal_connect ((GObject *) window_item->window, "destroy",
                      (GCallback) on_window_item_destroy, NULL);
    
    render_name = gtk_cell_renderer_text_new ();
    column_name = gtk_tree_view_column_new_with_attributes ("Nama", render_name,
                                                            "text", ITEM_NAME, NULL);
    gtk_tree_view_column_set_min_width (column_name, 160);
    gtk_tree_view_column_set_expand (column_name, TRUE);

    render_cost = gtk_cell_renderer_text_new ();
    column_cost = gtk_tree_view_column_new_with_attributes ("Harga", render_cost,
                                                            "text", ITEM_COST, NULL);
    gtk_tree_view_column_set_min_width (column_cost, 80);
    gtk_tree_view_column_set_expand (column_cost, TRUE);
    
    store = gtk_list_store_new (ITEM_COLUMN, G_TYPE_STRING, G_TYPE_ULONG);
    window_item->treeview = get_widget ("treeview");
    gtk_tree_view_set_rules_hint ((GtkTreeView *) window_item->treeview, TRUE);
    gtk_tree_view_set_model ((GtkTreeView *) window_item->treeview, 
                             (GtkTreeModel *) store);
    gtk_tree_view_append_column ((GtkTreeView *) window_item->treeview, column_name);
    gtk_tree_view_append_column ((GtkTreeView *) window_item->treeview, column_cost);
    
    window_item->button = get_widget ("button");
    g_signal_connect ((GObject *) window_item->button, "clicked",
                      (GCallback) on_window_item_destroy, NULL);
    builder_destroy ();
}

void
create_window_chat (void)
{
    gchar *tmp;

    builder_init ("chat.ui");
    window_chat = g_new (WindowChat, 1);

    window_chat->window = get_widget ("window");
    gtk_window_set_title ((GtkWindow *) window_chat->window, "Chat");
    tmp = get_pixmap_file ("gbilling.png");
    gtk_window_set_icon_from_file ((GtkWindow *) window_chat->window, tmp, NULL);
    g_free (tmp);
    g_signal_connect ((GObject *) window_chat->window, "destroy",
            (GCallback) on_window_chat_destroy, NULL);
    g_signal_connect ((GObject *) window_chat->window, "delete-event", 
            (GCallback) gtk_widget_hide_on_delete, NULL);
    g_signal_connect ((GObject *) window_chat->window, "key-press-event", 
            (GCallback) on_window_chat_key_press_event, NULL);

    window_chat->textv_log = get_widget ("textv_log");
    window_chat->textv_msg = get_widget ("textv_msg");

    window_chat->button_chat = get_widget ("button_chat");
    g_signal_connect ((GObject *) window_chat->button_chat, "clicked", 
            (GCallback) on_button_chatsend_clicked, NULL);

    builder_destroy ();
}

gint
create_dialog_auth (GtkWindow    *parent,
                    const gchar  *msg,
                    GbillingAuth *auth)
{
    g_return_val_if_fail (msg != NULL, GTK_RESPONSE_CANCEL);

    GtkWidget *window;
    GtkWidget *label_msg;
    GtkWidget *entry_username;
    GtkWidget *entry_passwd;
    const gchar *tmp;
    gchar *sum, *markup;
    gint ret;

    builder_init ("auth.ui");
    
    window = get_widget ("dialog");
    gtk_window_set_title ((GtkWindow *) window, "Login");
    if (parent)
        gtk_window_set_transient_for ((GtkWindow *) window, parent);
    else
        set_window_icon ((GtkWindow *) window);

    label_msg = get_widget ("label_msg");
    markup = g_strdup_printf ("<b>%s</b>", msg);
    gtk_label_set_markup ((GtkLabel *) label_msg, markup);
    g_free (markup);
    
    entry_username = get_widget ("entry_username");
    gtk_entry_set_max_length ((GtkEntry *) entry_username, 
                              sizeof(auth->username) - 1);
    entry_passwd = get_widget ("entry_passwd");
    gtk_entry_set_max_length ((GtkEntry *) entry_passwd, 
                              sizeof(auth->username) - 1);

    ret = gtk_dialog_run ((GtkDialog *) window);
    while (ret == GTK_RESPONSE_OK)
    {
        ret = GTK_RESPONSE_NO;
        tmp = gtk_entry_get_text ((GtkEntry *) entry_username);
        if (!strlen (tmp))
            break;
        g_snprintf (auth->username, sizeof(auth->username), tmp);
        tmp = gtk_entry_get_text ((GtkEntry *) entry_passwd);
        if (!strlen (tmp))
            break;
        sum = gbilling_str_checksum (tmp);
        g_snprintf (auth->passwd, sizeof(auth->passwd), sum);
        g_free (sum);

        ret = GTK_RESPONSE_OK;
        break;
    }

    gtk_widget_destroy (window);
    builder_destroy ();

    return ret;
}

