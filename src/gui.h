/*
 *  gBilling is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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
 *  gui.h
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef __GUI_H__
#define __GUI_H__

#include <gtk/gtk.h>

typedef enum 
{
    ITEM_NAME,
    ITEM_COST,
    ITEM_COLUMN
} ItemColumn;

typedef struct _WindowInit
{
    GtkWidget *window;
    GtkWidget *img;
    GtkWidget *progressbar;
} WindowInit;

extern WindowInit *window_init;

typedef struct _WindowLogin
{
    GtkWidget *window;
    GtkWidget *vbox_info;
    GtkWidget *label_cafe;
    GtkWidget *label_desc;
    GtkWidget *label_address;
    GtkWidget *label_client;
    GtkWidget *radiob_regular;
    GtkWidget *radiob_prepaid;
    GtkWidget *radiob_admin;
    GtkWidget *entry_username;
    GtkWidget *label_passwd;
    GtkWidget *entry_passwd;
    GtkWidget *label_prepaid;
    GtkWidget *combob_prepaid;
    GtkWidget *button_start;
    GtkWidget *button_restart;
    GtkWidget *button_shutdown;
    GtkWidget *label_time;
    GtkWidget *label_gbilling;
} WindowLogin;

extern WindowLogin *window_login;

typedef struct _WindowMain
{
    GtkWidget *window;
    GtkWidget *img;
} WindowMain;

extern WindowMain *window_main;

typedef struct _WindowTimer
{
    GtkWidget *window;
    GtkWidget *label_client;
    GtkWidget *entry_username;
    GtkWidget *entry_type;
    GtkWidget *entry_start;
    GtkWidget *entry_duration;
    GtkWidget *entry_cost;
    GtkWidget *button_menu;
    GtkWidget *button_chat;
    GtkWidget *button_stop;
    GtkWidget *button_about;
} WindowTimer;

extern WindowTimer *window_timer;

typedef struct _WindowSetting
{
    GtkWidget *window;
    GtkWidget *entry_ip;
    GtkWidget *spinb_port;
    GtkWidget *checkb_wallpaper;
    GtkWidget *label_wallpaper;
    GtkWidget *entry_wallpaper;
    GtkWidget *button_search;
    GtkWidget *colorb_background;
    GtkWidget *label_restart;
    GtkWidget *entry_restart;
    GtkWidget *label_shutdown;
    GtkWidget *entry_shutdown;
    GtkWidget *button_exit;
    GtkWidget *button_restart;
    GtkWidget *button_shutdown;
    GtkWidget *button_info;
    GtkWidget *entry_username;
    GtkWidget *entry_passwd;
    GtkWidget *entry_passwdc;
    GtkWidget *button_save;
    GtkWidget *button_ok;
} WindowSetting;

extern WindowSetting *window_setting;

typedef struct _WindowChat
{
    GtkWidget *window;
    GtkWidget *textv_log;
    GtkWidget *textv_msg;
    GtkWidget *button_chat;
} WindowChat;

extern WindowChat *window_chat;

typedef struct _WindowItem
{
    GtkWidget *window;
    GtkWidget *treeview;
    GtkWidget *button;
} WindowItem;

extern WindowItem *window_item;

extern guint label_istatsid;

gchar* get_data_file (const gchar*);
void start_label_istat (void);
void stop_label_istat (void);
void create_window_init (void);
void create_window_main (void);
gboolean set_window_login (gpointer);
void window_login_grab_keyboard (void);
void create_window_login (void);
gboolean set_button_wlogstart (gpointer);
void create_window_timer (void);
gboolean hide_window_timer (gpointer);
void create_window_setting (void);
void set_window_setting (void);
void create_window_item (void);
void create_window_chat (void);
gint create_dialog_auth (GtkWindow*, const gchar*, GbillingAuth*);

#endif /* __GUI_H__ */

