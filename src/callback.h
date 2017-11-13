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
 *	callback.h
 *	File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *	Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef	__CALLBACK_H__
#define	__CALLBACK_H__

#include <gtk/gtk.h>

#include "client.h"

void widget_destroy (GtkWidget*);
/* window_main */
gboolean on_window_main_key_press_event (GtkWidget*, GdkEventKey*, gpointer);
gboolean on_window_login_key_press_event (GtkWidget*, GdkEventKey*, gpointer);
void on_radio_regular_toggled (GtkWidget*, gpointer);
void on_radio_paket_toggled (GtkWidget*, gpointer);
void on_radio_admin_toggled (GtkWidget*, gpointer);
gboolean on_window_timer_delete_event (GtkWidget*, GdkEvent*, gpointer);
gboolean on_window_admin_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_window_setting_entry_ip_insert_text (GtkEditable*, gchar*, gint, gint*, gpointer);
void on_checkb_adwall_toggled (GtkCheckButton*, gpointer);
void on_button_adexit_clicked (GtkWidget*, gpointer);
void on_button_adrestart_clicked (GtkWidget*, gpointer);
void on_button_adshutdown_clicked (GtkWidget*, gpointer);
void on_button_adinfo_clicked (GtkWidget*, gpointer);
void on_window_setting_entry_username_changed (GtkWidget*, gpointer);
void on_window_setting_entry_passwd_changed (GtkWidget*, gpointer);
void on_window_setting_entry_passwdc_changed (GtkWidget*, gpointer);
void on_window_setting_button_save_clicked (GtkWidget*, gpointer);
gboolean enable_button_start (gpointer);
void on_window_chat_destroy (GtkWidget*, gpointer);
gboolean on_window_chat_key_press_event (GtkWidget*, GdkEventKey*, gpointer);
void on_button_chatsend_clicked (GtkWidget*, gpointer);
void on_window_last_destroy (GtkWidget*, gpointer);
void window_main_show (GtkWidget*, gpointer);
void on_window_main_show (GtkWidget*, gpointer);
void on_button_adwall_clicked (GtkWidget*, gpointer);
void on_checkb_adcmd_toggled (GtkCheckButton*, gpointer);
void on_button_adok_clicked (GtkWidget*, gpointer);
void on_window_item_destroy (GtkWidget*, gpointer);
/* window_login */
void on_radiob_wlogregular_toggled (GtkToggleButton*, gpointer);
void on_radiob_wlogpaket_toggled (GtkToggleButton*, gpointer);
void on_radiob_wlogmember_toggled (GtkToggleButton*, gpointer);
void on_radiob_wlogadmin_toggled (GtkToggleButton*, gpointer);
void on_button_wlogstart_clicked (GtkButton*, gpointer);
void on_button_wlogrestart_clicked (GtkButton*, gpointer);
void on_button_wlogshutdown_clicked (GtkButton*, gpointer);
/* window_timer */
void on_button_menu_clicked (GtkButton*, gpointer);
void on_button_chat_clicked (GtkWidget*, gpointer);
void on_button_stop_clicked (GtkWidget*, gpointer);
void on_button_about_clicked (GtkWidget*, gpointer);
/* window_term */
gboolean on_window_term_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_checkb_term_toggled (GtkToggleButton*, gpointer);
void on_button_term_clicked (GtkButton*, gpointer);

#endif /* __CALLBACK_H__ */

