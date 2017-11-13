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
 *  main.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 */

#include <gtk/gtk.h>
#include <glib.h>
#include <errno.h>

#include "client.h"
#include "gui.h"
#include "setting.h"

gint
main (gint   argc, 
      gchar *argv[])
{
    if (gbilling_lock_file ("gbilling-client.lock") == -1)
    {
#ifndef _WIN32
        if (errno == EAGAIN)
            gbilling_debug ("another instance is currently holds the lock\n");
        else
#endif
        gbilling_sysdebug ("gbilling_lock_file");
        return 0;
    }

    gtk_init (&argc, &argv);
    if (!g_thread_supported ())
        g_thread_init (NULL);

    client = g_new0 (struct _client, 1);
    client->cname = NULL;
    client->username = NULL;

    if (read_key_file () == -1)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, NULL, "Setting File",
                "File pengaturan salah atau tidak ditemukan, client "
                "akan membuat file pengaturan yang baru.");
        if (create_key_file () == -1)
        {
            gbilling_create_dialog (GTK_MESSAGE_ERROR, NULL, "Setting File",
                    "File pengaturan tidak dapat dibuat, periksa permisi "
                    "anda dan pastikan file ini tidak dipakai oleh proses "
                    "lainnya. Client tidak dapat dilanjutkan.");
            gbilling_debug ("%s: Could not create key file\n", __func__);
            return -1;
        }
    }

    if (!setup_client ())
    {
        gbilling_create_dialog (GTK_MESSAGE_ERROR, NULL, "Network Error",
                                "Inisialisasi rutin network gagal, periksa "
                                "sistem anda.");
        gbilling_sysdebug ("setup_client");
        return -1;
    }

    if (!setup_saddr ())
    {
        gint ret;
        ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, NULL, "Server", 
                        "Informasi server salah atau tidak lengkap, atur "
                        "client sekarang?");
        if (ret == GTK_RESPONSE_OK)
        {
            create_window_setting ();
            gtk_widget_show (window_setting->window);
        }
        else
            return 0;
    }

    create_window_main ();
    create_window_init ();
    create_window_login ();
    start_label_istat ();

    gtk_widget_show (window_main->window);
    gtk_widget_show (window_init->window);

    g_thread_create ((GThreadFunc) do_timesource, NULL, FALSE, NULL);
    g_timeout_add (1000, (GSourceFunc) first_init, NULL);
    gtk_main ();

    cleanup_client ();
    return 0;
}

