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
 *  callback.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _WIN32
# include <winsock2.h>
#else
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "main.h"
#include "callback.h"
#include "gui.h"
#include "setting.h"
#include "client.h"
#include "sockc.h"
#include "gbilling.h"

static gboolean window_main_keep_above (gpointer);

/**
 * destroy widget
 * params: @widget = widget yang akan di "destroy"
 */
void
widget_destroy (GtkWidget *widget)
{
    gtk_widget_destroy (widget);
}

/**
 * TODO: blok signal "key-press-event" atau "key-release-event"
 * sementara dgn gtk_window_set_keep_above()
 * lihat force_to_window_main()
 */
gboolean
on_window_main_key_press_event (GtkWidget   *widget, 
                                GdkEventKey *event, 
                                gpointer     data)
{
    return TRUE;
}

gboolean
on_window_login_key_press_event (GtkWidget   *widget,
                                 GdkEventKey *event,
                                 gpointer     data)
{
    /* block key modifier tertentu, hope this doesn't sucks the user */
    if (event->keyval == GDK_Alt_L
        || event->keyval == GDK_Alt_R
        || event->keyval == GDK_Tab
        || event->keyval == GDK_F1
        || event->keyval == GDK_F2
        || event->keyval == GDK_F3
        || event->keyval == GDK_F4
        || event->keyval == GDK_F5
        || event->keyval == GDK_F6
        || event->keyval == GDK_F7
        || event->keyval == GDK_F8
        || event->keyval == GDK_F9
        || event->keyval == GDK_F10
        || event->keyval == GDK_F11
        || event->keyval == GDK_F12
        || event->keyval == GDK_Control_L
        || event->keyval == GDK_Control_R
        || event->keyval == GDK_Escape)
        window_login_grab_keyboard ();
    else
        gdk_keyboard_ungrab (GDK_CURRENT_TIME);
    return FALSE;
}

void
on_button_wlogstart_clicked (GtkButton *button, 
                             gpointer   data)
{
    GbillingClientZet cset;
    const gchar *tuser, *tpass;
    gchar *user, *pass = NULL;
    gint ret;

    tuser = gtk_entry_get_text ((GtkEntry *) window_login->entry_username);
    user = g_strstrip (g_strdup (tuser));
    if (!strlen (user))
    {
        g_free (user);
        return;
    }
    client->start = get_time ();
    if (client->username)
        g_free (client->username);
    client->username = g_strdup (user);
    g_free (user);

    /*
     * Tipe login.
     */
    if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                      window_login->radiob_regular))
    {
        if (!strlen (client->username))
            return;
        client->type = GBILLING_LOGIN_TYPE_REGULAR;
    }

    else if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                           window_login->radiob_prepaid))
    {
        ret = gtk_combo_box_get_active ((GtkComboBox *) window_login->combob_prepaid);
        if (ret < 0)
            return;
        client->type = GBILLING_LOGIN_TYPE_PREPAID;
        client->idtype = ret;
    }
    else
    {
        client->type = GBILLING_LOGIN_TYPE_ADMIN;
        tpass = gtk_entry_get_text ((GtkEntry *) window_login->entry_passwd);
        if (!strlen (tpass))
        {
            gtk_entry_set_text ((GtkEntry *) window_login->entry_passwd, "");
            return;
        }
        pass = gbilling_str_checksum (tpass);
        g_snprintf (cset.auth.passwd, sizeof(cset.auth.passwd), pass);
        g_free (pass);
        gtk_entry_set_text ((GtkEntry *) window_login->entry_passwd, "");
    }

    cset.type = client->type;
    cset.id = client->idtype;
    cset.start = client->start;
    g_snprintf (cset.auth.username, sizeof(cset.auth.username), 
                client->username);

    gtk_toggle_button_set_active ((GtkToggleButton *) window_login->radiob_regular, TRUE);
    gtk_entry_set_text ((GtkEntry *) window_login->entry_username, "");
    gtk_entry_set_text ((GtkEntry *) window_login->entry_passwd, "");

    /*
     * Kita tidak perlu periksa status koneksi jika untuk administrasi. 
     */
    ret = send_login (&cset);
    if (ret == GBILLING_CONN_STATUS_ERROR 
        && client->type != GBILLING_LOGIN_TYPE_ADMIN)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_login->window, "Login",
                         "Server tidak dapat dihubungi, hubungi operator anda.");
        gtk_entry_set_text ((GtkEntry *) window_login->entry_username, "");
        return;
    }
    else if (ret == GBILLING_CONN_STATUS_REJECT)
        return;

    gtk_widget_hide (window_login->window);
    client->islogin = TRUE;

    if (client->type == GBILLING_LOGIN_TYPE_ADMIN)
    {
        create_window_setting ();
        set_window_setting ();
        gtk_widget_show (window_setting->window);
    }
    else
    {
        gtk_widget_hide (window_main->window);
        create_window_timer ();
        gtk_widget_show (window_timer->window);
        g_timeout_add (3000, (GSourceFunc) hide_window_timer, NULL);
        g_thread_create ((GThreadFunc) update_data, NULL, FALSE, NULL);
        gtk_window_set_focus ((GtkWindow *) window_login->window, 
                              window_login->entry_username);
    }
}

void
on_button_wlogrestart_clicked (GtkButton *button, 
                               gpointer   data)
{
    gint d = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                     (GtkWindow *) window_login->window,
                                     "Restart", "Konfirmasi untuk restart client.");
    if (d == GTK_RESPONSE_OK)
        gbilling_control_client (GBILLING_CONTROL_RESTART);
}

void
on_button_wlogshutdown_clicked (GtkButton *button, 
                                gpointer   data)
{
    gint d = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                     (GtkWindow *) window_login->window,
                                     "Shutdown", "Konfirmasi untuk shutdown client.");
    if (d == GTK_RESPONSE_OK)
        gbilling_control_client (GBILLING_CONTROL_SHUTDOWN);
}

gboolean
on_window_timer_delete_event (GtkWidget *widget, 
                              GdkEvent  *event, 
                              gpointer   data)
{
    /* callback ke on_button_stop_clicked() */
    on_button_stop_clicked (NULL, NULL);

    return TRUE;
}

gboolean
on_window_admin_delete_event (GtkWidget *widget, 
                              GdkEvent  *event, 
                              gpointer   data)
{
    do_logout (TRUE);

    return TRUE;
}

void
on_window_setting_entry_ip_insert_text (GtkEditable *editable,
                                        gchar       *text,
                                        gint         len,
                                        gint        *pos,
                                        gpointer     data)
{
    char c;

    if (*text == '\0' || len == 0)
        return;

    c = text[len - 1];
    if (c == '.')
        return;
    if (!g_ascii_isdigit (c))
        g_signal_stop_emission_by_name (editable, "insert-text");
}

void
on_checkb_adwall_toggled (GtkCheckButton *checkb,
                          gpointer        data)
{
    gboolean active = gtk_toggle_button_get_active ((GtkToggleButton *) checkb);
    gtk_widget_set_sensitive (window_setting->label_wallpaper, active);
    gtk_widget_set_sensitive (window_setting->entry_wallpaper, active);
    gtk_widget_set_sensitive (window_setting->button_search, active);
}

void
on_button_adexit_clicked (GtkWidget *widget, 
                          gpointer   data)
{
    gint d = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                     (GtkWindow *) window_setting->window, 
                                     "Keluar", "Konfirmasi untuk keluar dari client.");
    if (d == GTK_RESPONSE_OK)
    {
        /* notify server */
        do_logout (TRUE);
        gtk_main_quit ();
    }
}

void
on_button_adrestart_clicked (GtkWidget *widget, 
                             gpointer   data)
{
    gint d = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                     (GtkWindow *) window_setting->window,
                                     "Restart", "Konfirmasi untuk restart client.");
    if (d == GTK_RESPONSE_OK)
        gbilling_control_client (GBILLING_CONTROL_RESTART);
}

void
on_button_adshutdown_clicked (GtkWidget *widget, 
                              gpointer   data)
{
    gint d = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                     (GtkWindow *) window_setting->window,
                                     "Shutdown", "Konfirmasi untuk shutdown client.");
    if (d == GTK_RESPONSE_OK)
        gbilling_control_client (GBILLING_CONTROL_SHUTDOWN);
}

void
on_button_adinfo_clicked (GtkWidget *widget, 
                          gpointer   data)
{

}

void
on_window_setting_entry_username_changed (GtkWidget *widget,
                                          gpointer   data)
{
    if (strlen (gtk_entry_get_text ((GtkEntry *) widget)) > 3)
        gtk_widget_set_sensitive (window_setting->entry_passwd, TRUE);
    else
        gtk_widget_set_sensitive (window_setting->button_save, FALSE);

    gtk_entry_set_text ((GtkEntry *) window_setting->entry_passwd, "");
    gtk_entry_set_text ((GtkEntry *) window_setting->entry_passwdc, "");
    gtk_widget_set_sensitive ((GtkWidget *) window_setting->entry_passwdc, FALSE);
}

void
on_window_setting_entry_passwd_changed (GtkWidget *widget,
                                        gpointer   data)
{
    const gchar *user, *passwd, *passwdc;

    gtk_entry_set_text ((GtkEntry *) window_setting->entry_passwdc, "");
    gtk_widget_set_sensitive (window_setting->entry_passwdc, TRUE);

    user = gtk_entry_get_text ((GtkEntry *) window_setting->entry_username);
    passwd = gtk_entry_get_text ((GtkEntry *) window_setting->entry_passwd);
    passwdc = gtk_entry_get_text ((GtkEntry *) window_setting->entry_passwdc);

    gtk_widget_set_sensitive (window_setting->button_save,
                              !strcmp (passwd, passwdc) && 
                              strlen (passwd) > 3 &&
                              strlen (user) > 3);
}

void
on_window_setting_entry_passwdc_changed (GtkWidget *widget,
                                         gpointer   data)
{
    const gchar *user, *passwd, *passwdc;

    user = gtk_entry_get_text ((GtkEntry *) window_setting->entry_username);
    passwd = gtk_entry_get_text ((GtkEntry *) window_setting->entry_passwd);
    passwdc = gtk_entry_get_text ((GtkEntry *) window_setting->entry_passwdc);

    gtk_widget_set_sensitive (window_setting->button_save,
                              !strcmp (passwd, passwdc) && 
                               strlen (passwd) > 3 &&
                               strlen (user) > 3);
}

void
on_window_setting_button_save_clicked (GtkWidget *widget,
                                       gpointer   data)
{
    const gchar *username, *passwd;
    gchar *hash;
    gint ret;

    username = gtk_entry_get_text ((GtkEntry *) window_setting->entry_username);
    passwd = gtk_entry_get_text ((GtkEntry *) window_setting->entry_passwd);
    hash = gbilling_str_checksum (passwd);
    g_key_file_set_string (key_file, SECURITY_GROUP, SECURITY_USERNAME, username);
    g_key_file_set_string (key_file, SECURITY_GROUP, SECURITY_PASSWORD, hash);
    ret = write_key_file ();
    g_free (hash);

    if (ret)
        gbilling_create_dialog (GTK_MESSAGE_ERROR, (GtkWindow *) window_setting->window,
                                "Setting Client",
                                "File setting tidak dapat ditulis, silahkan "
                                "periksa permisi anda.");
    else
        gtk_widget_set_sensitive (widget, FALSE);
}

void
on_button_adwall_clicked (GtkWidget *widget, 
                          gpointer   data)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    gchar *file;

    dialog  = gbilling_file_dialog ("Pilih gambar", window_setting->window, 
                        GTK_FILE_CHOOSER_ACTION_OPEN);
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "Semua Gambar");
    gtk_file_filter_add_pattern (filter, "*.png");
    gtk_file_filter_add_pattern (filter, "*.jpg");
    gtk_file_filter_add_pattern (filter, "*.jpeg");
    gtk_file_filter_add_pattern (filter, "*.jpe");
    gtk_file_filter_add_pattern (filter, "*.bmp");
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter);
    
    filter = gtk_file_filter_new (); /* yeah, refcounting */
    gtk_file_filter_set_name (filter, "PNG (*.PNG)");
    gtk_file_filter_add_pattern (filter, "*.png");
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter);
    
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "JPG (*.JPG;*.JPEG;*.JPE)");
    gtk_file_filter_add_pattern (filter, "*.jpg");
    gtk_file_filter_add_pattern (filter, "*.jpeg");
    gtk_file_filter_add_pattern (filter, "*.jpe");
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter);
    
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "BMP (*.BMP)");
    gtk_file_filter_add_pattern (filter, "*.bmp");
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter);

    if (gtk_dialog_run ((GtkDialog *) dialog) != GTK_RESPONSE_OK)
    {
        gtk_widget_destroy (dialog);
        return;
    }
    
    file = gtk_file_chooser_get_filename ((GtkFileChooser *) dialog);
    gtk_entry_set_text ((GtkEntry *) window_setting->entry_wallpaper, file);
    gtk_widget_destroy (dialog);
    g_free (file);
}

void
on_checkb_adcmd_toggled (GtkCheckButton *checkb, 
                         gpointer        data)
{
    gboolean active = gtk_toggle_button_get_active ((GtkToggleButton *) checkb);
    gtk_widget_set_sensitive (window_setting->label_restart, active);
    gtk_widget_set_sensitive (window_setting->entry_restart, active);
    gtk_widget_set_sensitive (window_setting->label_shutdown, active);
    gtk_widget_set_sensitive (window_setting->entry_shutdown, active);
}

void
on_button_adok_clicked (GtkWidget *widget, 
                        gpointer   data)
{
    GdkColor color;
    const gchar *tmp;
    gchar *str, ip[16];
    gboolean use_wall;
    gint port;

    tmp = gtk_entry_get_text ((GtkEntry *) window_setting->entry_ip);
    if (!strlen (tmp) || inet_addr (tmp) == INADDR_NONE)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_setting->window, 
                                "Setting Client",
                                "Silahkan masukkan alamat server dengan benar.");
        return;
    }
    str = g_strstrip (g_strdup (tmp));
    g_key_file_set_string (key_file, SERVER_GROUP, SERVER_IP_KEY, str);
    g_snprintf (ip, sizeof(ip), str);
    g_free (str);
    port = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_setting->spinb_port);
    g_key_file_set_integer (key_file, SERVER_GROUP, SERVER_PORT_KEY, port);


    use_wall = gtk_toggle_button_get_active ((GtkToggleButton *) 
                                             window_setting->checkb_wallpaper);
    if (use_wall)
    {
        tmp = gtk_entry_get_text ((GtkEntry *) window_setting->entry_wallpaper);
        g_key_file_set_string (key_file, DISPLAY_GROUP, DISPLAY_WALLPAPER_KEY, tmp);
    }
    g_key_file_set_boolean (key_file, DISPLAY_GROUP, DISPLAY_USEWALLPAPER_KEY, use_wall);


    gtk_color_button_get_color ((GtkColorButton *) 
                                window_setting->colorb_background, &color);
    str = gdk_color_to_string (&color);
    g_key_file_set_string (key_file, DISPLAY_GROUP, DISPLAY_BGCOLOR_KEY, str);
    g_free (str);
    
    if (write_key_file () == -1)
    {
        gbilling_create_dialog (GTK_MESSAGE_ERROR, window_setting->window, "Error", 
                "File pengaturan tidak dapat ditulis, client akan keluar.");
        exit (EXIT_FAILURE);
    }
    
    gtk_widget_modify_bg (window_main->window, GTK_STATE_NORMAL, &color);
    /* update wallpaper */
    if (use_wall)
    {
        tmp = gtk_entry_get_text ((GtkEntry *) window_setting->entry_wallpaper);
        if (g_file_test (tmp, G_FILE_TEST_EXISTS))
        {
            gtk_image_set_from_file ((GtkImage *) window_main->img, tmp);
            gtk_widget_show (window_main->img);
        }
    } else
        gtk_widget_hide (window_main->img);


    do_logout (TRUE);
    
    /* update address server setelah melakukan logout, ini agar sebisanya
       logout berhasil misalnya karena perubahan address atau port */
    memset (&client->saddr, 0, sizeof(client->saddr));
    client->saddr.sin_family = AF_INET;
    client->saddr.sin_addr.s_addr = inet_addr (ip);
    client->saddr.sin_port = htons (port);
}

void
on_window_item_destroy (GtkWidget *widget, 
                        gpointer   data)
{
    gtk_widget_destroy (window_item->window);
    g_free (window_item);
    window_item = NULL;
}

void
on_button_stop_clicked (GtkWidget *widget, 
                        gpointer   data)
{
    gint ret;

    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_timer->window, 
                "Selesai Pemakaian", "Silahkan konfirmasi untuk menyelesaikan pemakaian.");
    if (ret != GTK_RESPONSE_OK)
        return;

    do_logout (TRUE);
    /* lihat on_button_wlogstart_clicked(), knp @client->islogin */
    if (client->islogin)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_timer->window,
                "Logout", "Server tidak dapat dihubungi, hubungi operator anda.");
        return;
    }
}

void
on_button_about_clicked (GtkWidget *widget, 
                         gpointer   data)
{
    gbilling_show_about_window ((GtkWindow *) window_timer->window);
}

gboolean
on_window_term_delete_event (GtkWidget *widget, 
                             GdkEvent  *event, 
                             gpointer   data)
{


    return TRUE;
}

void
on_checkb_term_toggled (GtkToggleButton *toggleb, 
                        gpointer         data)
{
    gboolean state = gtk_toggle_button_get_active (toggleb);
    gtk_widget_set_sensitive ((GtkWidget *) data, state);
}

void
on_button_term_clicked (GtkButton *button, 
                        gpointer   data)
{

}

void
on_button_menu_clicked (GtkButton *button, 
                        gpointer   data)
{
    GbillingItemList *ptr;
    GbillingItem *item;
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!window_item)
        create_window_item ();
    if (!get_item ())
    {
        gbilling_create_dialog (GTK_MESSAGE_INFO, window_login->window, 
            "Daftar Menu", "Server tidak dapat dihubungi, hubungi operator anda.");
        return;
    }

    /*
     * Hanya tampilkan item yang diset aktif.
     * Hey, look at the GbillingItemList pointer member usage, 
     * if you are real man then you won't touch this with '->' operator ;p.
     */
    if ((ptr = item_list))
    {
        model = gtk_tree_view_get_model ((GtkTreeView *) window_item->treeview);
        gtk_list_store_clear ((GtkListStore *) model);
        while (ptr)
        {
            item = (*ptr).data;
            if ((*item).active)
            {
                gtk_list_store_append ((GtkListStore *) model, &iter);
                gtk_list_store_set ((GtkListStore *) model, &iter,
                                    ITEM_NAME, (*item).name,
                                    ITEM_COST, (*item).cost,
                                    -1);
            }
            ptr = (*ptr).next;
        }
    }
    if (window_item)
        gtk_window_present ((GtkWindow *) window_item->window);
    else
        gtk_widget_show (window_item->window);
}

void
on_button_chat_clicked (GtkWidget *widget, 
                        gpointer   data)
{
    if (!window_chat)
    {
        create_window_chat ();
        gtk_widget_show (window_chat->window);
    }
    else
        gtk_window_present ((GtkWindow *) window_chat->window);
    gtk_window_set_focus ((GtkWindow *) window_chat->window, window_chat->textv_msg);
}

void
on_window_chat_destroy (GtkWidget *widget,
                        gpointer   data)
{
    gtk_widget_destroy (window_chat->window);
    g_free (window_chat);
    window_chat = NULL;
}

gboolean
on_window_chat_key_press_event (GtkWidget   *widget, 
                                GdkEventKey *event,
                                gpointer     data)
{
    if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
    {
        gtk_widget_grab_focus (window_chat->button_chat);
        gtk_widget_activate (window_chat->button_chat);
    }

    return FALSE;
}

void
on_button_chatsend_clicked (GtkWidget *widget, 
                            gpointer   data)
{
    GtkTextIter start, end;
    GtkTextBuffer *buf;
    gchar *msg;
    
    buf = gtk_text_view_get_buffer ((GtkTextView *) window_chat->textv_msg);
    gtk_text_buffer_get_iter_at_offset (buf, &start, 0);
    gtk_text_buffer_get_iter_at_offset (buf, &end, 128);
    msg = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
    msg = g_strstrip (msg);
    if (strlen (msg))
    {
        if (!send_chat (msg))
        {
            gbilling_sysdebug ("send_chat");
            gtk_text_buffer_set_text (buf, "", -1);
            g_free (msg);
            gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                   (GtkWindow *) window_chat->window, "Chat",
                   "Server tidak dapat dihubungi, hubungi operator anda.");
            return;
        }
        fill_chat_log (msg, GBILLING_CHAT_MODE_CLIENT);
    }
    gtk_text_buffer_set_text (buf, "", -1);
    gtk_widget_grab_focus (window_chat->textv_msg);
    g_free (msg);
}

void
on_window_last_destroy (GtkWidget *widget,
                        gpointer   data)
{
    gtk_widget_destroy ((GtkWidget *) data);
}

void
on_window_about_destroy (GtkWidget *widget, 
                         gpointer   data)
{

}

static gboolean
window_main_keep_above (gpointer data)
{
    /* @window_main selalu on top */
    gtk_window_set_keep_above ((GtkWindow *) window_main->window, TRUE);
    return TRUE;
}

void
on_window_main_show (GtkWidget *widget, 
                     gpointer   data)
{
    static guint source = 0;

    if (source > 0)
        if (!g_source_remove (source))
            gbilling_debug ("%s: Could not remove source\n", __func__);
    source = g_timeout_add (100, (GSourceFunc) window_main_keep_above, NULL);
}

void
on_radiob_wlogregular_toggled (GtkToggleButton *toolb, 
                               gpointer         data)
{
    gtk_widget_set_sensitive (window_login->label_passwd, FALSE);
    gtk_widget_set_sensitive (window_login->label_prepaid, FALSE);
    gtk_widget_set_sensitive (window_login->entry_passwd, FALSE);
    gtk_widget_set_sensitive (window_login->combob_prepaid, FALSE);
    if (window_login)
        gtk_window_set_focus ((GtkWindow *) window_login->window, 
                              window_login->entry_username);
}

void
on_radiob_wlogpaket_toggled (GtkToggleButton *toolb, 
                             gpointer         data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GbillingPrepaidList *list;
    GbillingPrepaid *prepaid;
    guint len;

    gtk_widget_set_sensitive (window_login->label_passwd, FALSE);
    gtk_widget_set_sensitive (window_login->entry_passwd, FALSE);
    gtk_widget_set_sensitive (window_login->label_prepaid, TRUE);
    gtk_widget_set_sensitive (window_login->combob_prepaid, TRUE);
    len = g_list_length (group_list);
    gtk_label_set_text ((GtkLabel *) window_login->label_prepaid, "Paket:");
    if (len)
    {
        model = gtk_combo_box_get_model ((GtkComboBox *) window_login->combob_prepaid);
        gtk_list_store_clear ((GtkListStore *) model);
        for (list = prepaid_list; list; list = g_list_next (list))
        {
            prepaid = list->data;
            gtk_list_store_append ((GtkListStore *) model, &iter);
            gtk_list_store_set ((GtkListStore *) model, &iter, 0, prepaid->name, -1);
        }
        gtk_combo_box_set_active ((GtkComboBox *) window_login->combob_prepaid, 0);
    }
    gtk_window_set_focus ((GtkWindow *) window_login->window, window_login->entry_username);
}

void
on_radiob_wlogmember_toggled (GtkToggleButton *toolb,
                              gpointer         data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GbillingMemberGroupList *list;
    GbillingMemberGroup *group;
    guint len;

    gtk_widget_set_sensitive (window_login->label_passwd, TRUE);
    gtk_widget_set_sensitive (window_login->entry_passwd, TRUE);
    gtk_widget_set_sensitive (window_login->label_prepaid, TRUE);
    len = g_list_length (group_list);
    gtk_label_set_text ((GtkLabel *) window_login->label_prepaid, "Grup:");
    if (len)
    {
        gtk_widget_set_sensitive (window_login->combob_prepaid, TRUE);
        model = gtk_combo_box_get_model ((GtkComboBox *) window_login->combob_prepaid);
        gtk_list_store_clear ((GtkListStore *) model);
        for (list = group_list; list; list = g_list_next (list))
        {
            group = list->data;
            gtk_list_store_append ((GtkListStore *) model, &iter);
            gtk_list_store_set ((GtkListStore *) model, &iter, 0, group->name, -1);
        }
        gtk_combo_box_set_active ((GtkComboBox *) window_login->combob_prepaid, 0);
    }
    gtk_window_set_focus ((GtkWindow *) window_login->window, window_login->entry_username);
}

void
on_radiob_wlogadmin_toggled (GtkToggleButton *toolb, 
                             gpointer         data)
{
    gtk_widget_set_sensitive (window_login->label_passwd, TRUE);
    gtk_widget_set_sensitive (window_login->label_prepaid, FALSE);
    gtk_widget_set_sensitive (window_login->entry_passwd, TRUE);
    gtk_widget_set_sensitive (window_login->combob_prepaid, FALSE);
    gtk_window_set_focus ((GtkWindow *) window_login->window, window_login->entry_username);
}

