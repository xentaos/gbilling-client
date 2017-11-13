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
 *  setting.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>

#include "gbilling.h"
#include "setting.h"

GKeyFile *key_file = NULL;

/**
 * read_key_file:
 * Returns: 0 jika file setting berhasil di-load -1 jika gagal.
 *
 * Baca file setting.
 */
gint
read_key_file (void)
{
	gchar *tmp;
	gint ret = 0;
	
	tmp = g_build_filename (g_get_home_dir (), ".gbilling", SETTING_FILE, NULL);
	key_file = g_key_file_new ();
	if (!g_key_file_load_from_file (key_file, tmp, G_KEY_FILE_NONE, NULL))
        ret = -1;
	g_free (tmp);

	return ret;
}

/**
 * write_key_file:
 * Returns: 0 jika file bisa ditulis, -1 jika gagal.
 *
 * Tulis file setting.
 */
gint
write_key_file (void)
{
	GIOChannel *write;
	GIOStatus status;
	gchar *tmp;
	gint ret = 0;

    tmp = g_build_filename (g_get_home_dir (), ".gbilling", SETTING_FILE, NULL);
    write = g_io_channel_new_file (tmp, "w", NULL);
    g_free (tmp);
	if (!write)
		return -1;

    tmp = g_key_file_to_data (key_file, NULL, NULL);
	tmp = g_strstrip (tmp);
	status = g_io_channel_write_chars (write, tmp, -1, NULL, NULL);
	if (status == G_IO_STATUS_ERROR)
        ret = -1;
	g_io_channel_shutdown (write, TRUE, NULL);
	g_io_channel_unref (write);
	g_free (tmp);
	
	return ret;
}

/** 
 * create_key_file:
 * Returns: 0 jika file setting bisa dibuat, -1 jika gagal.
 *
 * Buat file setting.
 */
gint
create_key_file (void)
{
    GIOChannel *write;
    GIOStatus status;
    gchar *tmp, *file;
    gint mode = 0;
    gint ret = 0;

#ifndef _WIN32
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; /* 0755 */
#endif

    tmp = g_build_filename (g_get_home_dir (), ".gbilling", NULL);
    g_mkdir (tmp, mode);
    file = g_build_filename (tmp, SETTING_FILE, NULL);
    write = g_io_channel_new_file (file, "w", NULL);
    g_free (tmp);
    g_free (file);
    if (!write)
        return -1;

    key_file = g_key_file_new ();
#ifdef PACKAGE_DATA_DIR
    tmp = g_build_filename (PACKAGE_DATA_DIR, "gbilling-client", "blue.png", NULL);
#else
    gchar *dir = g_get_current_dir ();
    tmp = g_build_filename (dir, "share", "background", "blue.png", NULL);
    g_free (dir);
#endif

    g_key_file_set_string (key_file, SERVER_GROUP, SERVER_IP_KEY, SERVER_DEFAULT_IP);
    g_key_file_set_integer (key_file, SERVER_GROUP, SERVER_PORT_KEY, SERVER_DEFAULT_PORT);

    g_key_file_set_boolean (key_file, DISPLAY_GROUP, DISPLAY_USEWALLPAPER_KEY, FALSE);
    g_key_file_set_string (key_file, DISPLAY_GROUP, DISPLAY_WALLPAPER_KEY, tmp);
    g_key_file_set_string (key_file, DISPLAY_GROUP, DISPLAY_BGCOLOR_KEY, "#0E71A6");

    g_key_file_set_string (key_file, SECURITY_GROUP, SECURITY_USERNAME, "admin");

    /* 
     * This is a md5sum of the combination of `gbilling' string
     */
    g_key_file_set_string (key_file, SECURITY_GROUP, SECURITY_PASSWORD, 
                           "c80b2d12b5d8e1fa68c3b160d33b5ae5");

    g_free (tmp);
    tmp = g_key_file_to_data (key_file, NULL, NULL);
    tmp = g_strstrip (tmp);
    status = g_io_channel_write_chars (write, tmp, -1, NULL, NULL);
    g_free (tmp);
    if (status == G_IO_STATUS_ERROR)
        ret = -1;
    g_io_channel_shutdown (write, TRUE, NULL);
    g_io_channel_unref (write);
    
    return ret;
}
