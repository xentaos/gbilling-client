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
 *  client.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef _WIN32
# include <windows.h>
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "gbilling.h"
#include "client.h"
#include "setting.h"
#include "sockc.h"
#include "callback.h"
#include "gui.h"

#define CONNECT_TIMEOUT       10
#define UPDATE_TIME_INTERVAL  600
#define RECV_TIMEOUT          5

GbillingClientZet clientzet;
GbillingServerInfo server_info;
GbillingCost default_cost;

GbillingCostList *cost_list = NULL;
GbillingPrepaidList *prepaid_list = NULL;
GbillingItemList *item_list = NULL;
GbillingMemberGroupList *group_list = NULL;

struct _client *client = NULL;
struct _client *last = NULL;
GCond *client_cond = NULL;

static gboolean is_recv = FALSE;
static GThread *servcmd_t = NULL;

static volatile int init_reply = GBILLING_COMMAND_NONE;

static gint create_socket (void);
static gpointer get_servset (gpointer);
static gboolean expire_notify (gpointer);
static gpointer get_servcmd (gpointer);
static gboolean do_logout0 (gpointer);
static gboolean do_chat (gpointer);
static gboolean do_control (gpointer);

G_LOCK_DEFINE_STATIC (timesource_lock);

static time_t timex = 0L;

/**
 * Rutin timesource client, fungsi ini sangat sederhana yaitu hanya
 * dengan increment value lokal time server (timex). Ada delta waktu 
 * saat komunikasi kirim/terima waktu yang membuat waktu tidak presisi, 
 * perbedaan ini bukan masalah besar.
 *
 * Program seperti billing tidak memerlukan waktu yang sangat presisi, 
 * delta waktu sekitar 5 detik antara server dan client masih bisa 
 * ditoleransi, itupun jika terjadi. Nilai delta ini mungkin mempunyai 
 * satuan dalam mili-detik.
 */
gpointer
do_timesource (gpointer data)
{
    timex = time (NULL);
    do {
        gbilling_sleep (1);
        G_LOCK (timesource_lock);
        timex++;
        G_UNLOCK (timesource_lock);
    } while (1);

    return NULL;
}

static void
update_time (time_t value)
{
    G_LOCK (timesource_lock);
    timex = value;
    G_UNLOCK (timesource_lock);
}

time_t
get_time (void)
{
    time_t retval;

    G_LOCK (timesource_lock);
    retval = timex;
    G_UNLOCK (timesource_lock);
    return retval;
}

/**
 * create_socket:
 *
 * Returns: Socket descriptor.
 * Buat socket, abort jika socket gagal dibuat.
 **/
static gint
create_socket (void)
{
    gint s;

    s = gbilling_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        gbilling_debug ("%s: socket error: %s\n", __func__, gbilling_unix_debug (errno));
        exit (EXIT_FAILURE);
    }
    return s;
}

/**
 * setup_client:
 *
 * Returns: TRUE jika inisilasi berhasil, FALSE jika gagal.
 * Inisialisasi client, seperti pada Windows rutin ini meng-inisialisasi Winsock.
 */
gboolean
setup_client (void)
{
    gboolean retval = TRUE;

#ifdef _WIN32
    WORD wsaver;
    WSADATA wsadata;

    wsaver = MAKEWORD (2, 2);
    if (WSAStartup (wsaver, &wsadata))
    {
        errno = WSAGetLastError ();
        WSACleanup ();
        WSASetLastError (errno);
        return FALSE;
    }

    if (LOBYTE (wsadata.wVersion) < 2)
    {
        gbilling_debug ("%s: invalid winsock version %i.%i\n", __func__,
                        LOBYTE (wsadata.wVersion), HIBYTE (wsadata.wVersion));
        WSACleanup ();
        return FALSE;
    }
#endif

    return retval;
}

/**
 * cleanup_client:
 *
 * Rutin cleanup client sebelum keluar, pada WIN32 seperti cleanup Winsock.
 */
gint
cleanup_client (void)
{
    gint retval = 0;

#ifdef _WIN32
    retval = WSACleanup ();
#endif
    return retval;
}

/**
 * setup_saddr:
 *  
 * Returns: TRUE jika inisialisasi alamat internet server berhasil, FALSE jika gagal.
 * Inisialisasi alamat internet server dari setting file.
 **/
gboolean
setup_saddr (void)
{
    gchar *ip;
    gint port;

    ip = g_key_file_get_string (key_file, SERVER_GROUP, SERVER_IP_KEY, NULL);
    if (!ip)
        return FALSE;
    port = g_key_file_get_integer (key_file, SERVER_GROUP, SERVER_PORT_KEY, NULL);
    if (!port)
        return FALSE;
    memset (&client->saddr, 0, sizeof(client->saddr));
    client->saddr.sin_family = AF_INET;
    client->saddr.sin_addr.s_addr = inet_addr (ip);
    client->saddr.sin_port = htons (port);
    g_free (ip);

    return TRUE;
}

gboolean
send_command (gint8 command)
{
    gint op, s;
    gboolean ret = FALSE;
    
    s = create_socket ();
    g_return_val_if_fail (s != -1, ret);
    op = gbilling_connect (s, (struct sockaddr *) &client->saddr, sizeof(client->saddr));
    if (op == -1)
    {
        gbilling_sysdebug ("connect");
        goto done;
    }
    op = gbilling_send (s, &command, sizeof(command), 0);
    if (op == -1)
    {
        gbilling_sysdebug ("send");
        goto done;
    }
    ret ^= 1;
done:
    gbilling_close (s);
    return ret;
}

/**
 * get_servset:
 *
 * Returns: Selalu NULL.
 * Ambil informasi dari server.
 **/
static gpointer
get_servset (gpointer data)
{
    GMutex *mutex;
    static struct timeval val = { .tv_sec = RECV_TIMEOUT, .tv_usec = 0 };
    gint s, ret, timeout = 0;
    guint8 cmd;
    gint32 server_time;
    gchar name[17];

    mutex = g_mutex_new ();
    g_mutex_lock (mutex);

    is_recv = FALSE;
    s = create_socket ();

    do
    {
        if (timeout++ >= (CONNECT_TIMEOUT - 1))
            goto done;
        ret = gbilling_connect (s, (struct sockaddr *) &client->saddr, 
                        sizeof(client->saddr));
        if (ret == -1)
        {
            gbilling_sysdebug ("connect");
            gbilling_sleep (1);
        }
    } while (ret == -1);

    cmd = GBILLING_COMMAND_ACTIVE;
    ret = gbilling_send (s, &cmd, sizeof(cmd), 0);
    if (ret <= 0)
    {
        gbilling_sysdebug ("send");
        goto done;
    }

    ret = gbilling_setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &val, sizeof(val));
    if (ret == -1)
    {
        gbilling_sysdebug ("setsockopt");
        goto done;
    }

    ret = gbilling_recv (s, &cmd, sizeof(cmd), 0);
    if (ret <= 0)
    {
        gbilling_sysdebug ("recv server's cmd");
        goto done;
    }
    else
    {
        gbilling_debug ("%s: server cmd: %i\n", __func__, cmd);
        init_reply = cmd;
    }

    /* Terima waktu lokal server */
    ret = gbilling_data_recv (s, &server_time, sizeof(server_time));
    if (ret <= 0)
    {
        gbilling_sysdebug ("gbilling_data_recv server's localtime");
        goto done;
    }
    server_time = g_ntohl (server_time);
    update_time (server_time);
    gbilling_debug ("%s: server's localtime is %lu\n", __func__, server_time);

    /* Terima informasi server */
    if (!gbilling_server_info_recv (s, &server_info))
    {
        gbilling_sysdebug ("gbilling_server_info_recv");
        goto done;
    }

    /* Terima nama client */
    ret = gbilling_recv (s, &name, sizeof(name), 0);
    if (ret <= 0)
    {
        gbilling_sysdebug ("recv client's name");
        goto done;
    }

    name[sizeof(name) - 1] = 0;
    g_free (client->cname);
    client->cname = g_strdup (name);

    /* Terima daftar prepaid/paket */
    gbilling_prepaid_list_free (prepaid_list);
    if (!gbilling_prepaid_list_recv (s, &prepaid_list))
    {
        gbilling_sysdebug ("gbilling_prepaid_list_recv");
        goto done;
    }

    /* Terima tarif */
    ret = gbilling_cost_recv (s, &default_cost);
    if (!ret)
    {
        gbilling_sysdebug ("gbilling_cost_recv");
        goto done;
    }
    
    is_recv = TRUE;

    done:
    gbilling_close (s);
    g_mutex_unlock (mutex);
    g_mutex_free (mutex);

    return NULL;
}

/**
 * get_servcmd:
 * @data: Tidak dipakai.
 * Returns: Selalu %NULL.
 *
 * Cek perintah server per interval waktu.
 * TODO: Proteksi operasi client, misalnya client yang sedang login
 * akan mengabaikan perintah login lagi, dst.
 *
 **/
static gpointer
get_servcmd (gpointer data)
{
    GbillingServerSet scmd;
    const gint8 cmd = GBILLING_COMMAND_REQUEST;
    const gint8 time_update_cmd = GBILLING_COMMAND_TIME;
    gint32 time_value;
    gint s, ret, time_update = 0;
    gsize bytes;

    while (1)
    {
        s = gbilling_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == -1)
        {
            gbilling_sysdebug ("could not create socket");
            exit (EXIT_FAILURE);
        }

        ret = gbilling_connect (s, (struct sockaddr *) &client->saddr, sizeof(client->saddr));
        if (ret == -1)
        {
            gbilling_sysdebug ("connect");
            goto try;
        }

        /*
         * Ambil waktu lokal server untuk update timersource client
         * per UPDATE_TIME_INTERVAL.
         */
        if (time_update >= UPDATE_TIME_INTERVAL)
        {
            time_update = 0;
            ret = gbilling_send (s, &time_update_cmd, sizeof(time_update_cmd), 0);
            if (ret == -1)
            {
                gbilling_sysdebug ("send");
                goto try;
            }

            bytes = 0;
            while (bytes < sizeof(time_value))
            {
                ret = gbilling_recv (s, (gchar *) &time_value + bytes,
                                     sizeof(time_value) - bytes, 0);
                if (ret <= 0)
                {
                    if (ret == -1)
                        gbilling_sysdebug ("recv");
                    goto try;
                }
                bytes += ret;
            }

            time_value = g_ntohl (time_value);
            update_time (time_value);
            gbilling_debug ("%s: update localtime to %li\n", __func__, time_value);
            goto try;
        }

        ret = gbilling_send (s, &cmd, sizeof(cmd), 0);
        if (ret == -1)
        {
            gbilling_sysdebug ("send");
            goto try;
        }
        else if (ret == 0)
            goto try;

        ret = gbilling_server_cmd_recv (s, &scmd);
        if (ret <= 0)
        {
            if (ret == -1)
                gbilling_sysdebug ("recv");
            goto try;
        }

        gbilling_debug ("%s: recv: command: %i\n", __func__, scmd.cmd);

        switch (scmd.cmd)
        {
            case GBILLING_COMMAND_LOGIN:
            case GBILLING_COMMAND_RELOGIN:
                client->type = scmd.type;
                client->idtype = scmd.idtype;
                client->start = scmd.tstart;
                if (!client->username)
                    g_free (client->username);
                client->username = g_strdup (scmd.username);

                g_idle_add ((GSourceFunc) do_login, 
                            (scmd.cmd == GBILLING_COMMAND_LOGIN) ? 
                            GINT_TO_POINTER (TRUE) : NULL);
                break;

            case GBILLING_COMMAND_LOGOUT:
                g_idle_add ((GSourceFunc) do_logout0, GINT_TO_POINTER (TRUE));
                break;
                      
            case GBILLING_COMMAND_CHAT:
                g_idle_add ((GSourceFunc) do_chat, scmd.msg);
                break;
                      
            case GBILLING_COMMAND_RESTART:
                /* untuk konfirmasi, jalankan
                 * gbilling_create_dialog() di main thread dgn 
                 * g_idle_add(), di win32... perintah ini akan di-suspend
                 * setelah thread berhenti
                 */
                g_idle_add ((GSourceFunc) do_control, 
                             GINT_TO_POINTER (GBILLING_CONTROL_RESTART));
                break;

            case GBILLING_COMMAND_SHUTDOWN:
                g_idle_add ((GSourceFunc) do_control, 
                            GINT_TO_POINTER (GBILLING_CONTROL_SHUTDOWN));
                break;

            case GBILLING_COMMAND_ENABLE:
                g_idle_add ((GSourceFunc) set_button_wlogstart, GINT_TO_POINTER (TRUE));
                break;
                        
            case GBILLING_COMMAND_DISABLE:
                g_idle_add ((GSourceFunc) set_button_wlogstart, GINT_TO_POINTER (FALSE));
                break;
                    
            default:
                gbilling_debug ("%s: unknown server cmd %i\n", __func__, scmd.cmd);
                break;
        } /* switch */
try:
        gbilling_close (s);
        gbilling_sleep (1);
        time_update++;
    } /* while */
    
    return NULL;
#undef debug
}

/**
 * Kirim data login client.
 * Metallica - Nothing Else Matters
 */
GbillingConnStatus
send_login (const GbillingClientSet *cset)
{
    gint8 cmd;
    gint s, ret;
    struct timeval val = { .tv_sec = RECV_TIMEOUT, .tv_usec = 0 };

    s = gbilling_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        gbilling_sysdebug ("socket");
        return GBILLING_CONN_STATUS_ERROR;
    }

    ret = gbilling_setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &val, sizeof(val));
    if (ret == -1)
    {
        gbilling_close (s);
        return GBILLING_CONN_STATUS_ERROR;
    }

    ret = gbilling_connect (s, (struct sockaddr *) &client->saddr, 
                            sizeof(client->saddr));
    if (ret == -1)
    {
        gbilling_sysdebug ("connect");
        gbilling_close (s);
        return GBILLING_CONN_STATUS_ERROR;
    }
    cmd = GBILLING_COMMAND_LOGIN;
    ret = gbilling_send (s, &cmd, sizeof(cmd), 0);
    if (ret == -1)
    {
        gbilling_sysdebug ("send");
        gbilling_close (s);
        return GBILLING_CONN_STATUS_ERROR;
    }
    ret = gbilling_clientset_send (s, cset);
    if (!ret)
    {
        gbilling_sysdebug ("gbilling_clientset_send");
        gbilling_close (s);
        return GBILLING_CONN_STATUS_ERROR;
    }
    
    /*
     * Cek balasan server untuk login administrasi.
     */
    if (cset->type == GBILLING_LOGIN_TYPE_ADMIN)
    {
        ret = gbilling_recv (s, &cmd, sizeof(cmd), 0);
        if (ret == -1)
        {
            gbilling_sysdebug ("recv");
            gbilling_close (s);
            return GBILLING_CONN_STATUS_ERROR;
        }

        if (cmd == GBILLING_COMMAND_REJECT)
        {
            gbilling_debug ("%s: server reject login\n", __func__);
            gbilling_close (s);
            return GBILLING_CONN_STATUS_REJECT;
        }
    }
    else if (cset->type == GBILLING_LOGIN_TYPE_REGULAR)
    {
        /**
         * Terima daftar tarif disini.
         */
        ret = gbilling_cost_recv (s, &default_cost);
        if (!ret)
        {
            gbilling_sysdebug ("gbilling_cost_recv");
            gbilling_close (s);
            return FALSE;
        }
    }

    gbilling_close (s);
    return GBILLING_CONN_STATUS_SUCCESS;
}

/**
 * get_item:
 * 
 * Ambil daftar menu dari server.
 * Returns: %TRUE jika berhasil, %FALSE jika gagal.
 **/
gboolean
get_item (void)
{
    gboolean retval = FALSE, ret;
    gint s;
    gint8 cmd = GBILLING_COMMAND_ITEM;
    struct timeval tv = { .tv_sec = RECV_TIMEOUT, .tv_usec = 0 };
    
    s = gbilling_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        gbilling_sysdebug ("socket");
        return retval;
    }
    gbilling_setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, 
                         &tv, sizeof(struct timeval));
    ret = gbilling_connect (s, (struct sockaddr *) &client->saddr, 
                            sizeof(client->saddr));
    if (ret == -1)
    {
        gbilling_sysdebug ("gbilling_connect");
        goto done;
    }
    
    ret = gbilling_send (s, &cmd, sizeof(cmd), 0);
    if (ret == -1)
    {
        gbilling_sysdebug ("send");
        goto done;
    }

    if (item_list)
        gbilling_item_list_free (item_list);
    ret = gbilling_item_list_recv (s, &item_list);
    if (!ret)
    {
        gbilling_sysdebug ("gbilling_item_recv");
        goto done;
    }
    
    retval = TRUE;

done:
    gbilling_close (s);
    return retval;
}

gboolean
login_init (GbillingClientZet cset)
{
    gint8 cmd;
    gint s;
    gsize bytes;
    gboolean ret = FALSE;

    s = create_socket ();
    g_return_val_if_fail (s != -1, ret);
    ret = gbilling_connect (s, (struct sockaddr *) &client->saddr, sizeof(client->saddr));
    if (ret == -1)
    {
        gbilling_debug ("%s: connect: %i %s\n", __func__, errno, 
                gbilling_unix_debug (errno));
        goto done;
    }
    cmd = GBILLING_COMMAND_LOGIN;
    bytes = gbilling_send (s, &cmd, sizeof(cmd), 0);
    if (bytes <= 0)
    {
        gbilling_debug ("%s: send: %i %s\n", __func__, errno, 
                gbilling_unix_debug (errno));
        goto done;
    }

    if (!gbilling_clientset_send (s, &cset))
    {
        gbilling_debug ("%s: gbilling_clientset_send: %i %s\n", __func__, errno, 
                gbilling_unix_debug (errno));
        goto done;
    }

    done:
    ret ^= 1;
    
    gbilling_close (s);
    return ret;
}

gpointer
retrieve_data (gpointer data)
{
    GTimeVal val = { .tv_sec = 5, .tv_usec = 0 }; /* timeout 5 detik */
    GMutex *mutex;

    mutex = g_mutex_new ();
    g_mutex_lock (mutex);
    g_cond_timed_wait (client_cond, mutex, &val);
    g_mutex_free (mutex);
    return NULL;
}

static gboolean
show_window_login (gpointer data)
{
    if (!client->islogin)
        gtk_widget_show (window_login->window);
    return FALSE;
}

/* init, progress koneksi client ke server
 * respect dgn @CONNECT_TIMEOUT, tanyakan user utk
 * pengaturan client jika timeout.
 *
 * TODO: Need recode, this sucks!
 */
gboolean
first_init (gpointer data)
{
    GbillingAuth auth;
    gchar *username, *passwd;
    static gint i = 0;
    gint id;

    if (!i)
        g_thread_create ((GThreadFunc) get_servset, NULL, TRUE, NULL);
    if (i >= CONNECT_TIMEOUT)
    {
        i = 0;
        gtk_widget_hide (window_init->window);
        gtk_widget_show (window_main->window);
        id = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                     (GtkWindow *) window_main->window,
                                     "Koneksi Gagal", "Koneksi ke server gagal, "
                                     "masuk ke pengaturan client?");
        if (id == GTK_RESPONSE_OK)
        {
            id = create_dialog_auth ((GtkWindow *) window_main->window, 
                                     "Pengaturan Client", &auth);
            if (id != GTK_RESPONSE_OK)
            {
                gtk_widget_show (window_init->window);
                return TRUE;
            }

            username = g_key_file_get_string (key_file, SECURITY_GROUP, SECURITY_USERNAME, NULL);
            passwd = g_key_file_get_string (key_file, SECURITY_GROUP, SECURITY_PASSWORD, NULL);

            if (username && passwd)
                id = strcmp (auth.username, username) || strcmp (auth.passwd, passwd);
            else
                id = TRUE;
            g_free (username);
            g_free (passwd);

            if (id == TRUE)
            {
                gtk_widget_show (window_init->window);
                return TRUE;
            }

            client->isadmin = TRUE;
            create_window_setting ();
            gtk_widget_hide (window_init->window);
            set_window_setting ();
            gtk_widget_show (window_setting->window);
            return FALSE;
        }
        else
        {
            gtk_widget_show (window_init->window);
            return TRUE;
        }
    }
    if (is_recv)
    {
        switch (init_reply)
        {
            case GBILLING_COMMAND_ACCEPT:
                set_window_login (NULL);
                gtk_widget_show (window_main->window);
                g_timeout_add (125, (GSourceFunc) show_window_login, NULL);
                gtk_widget_hide (window_init->window);

                if (!servcmd_t)
                    servcmd_t = g_thread_create ((GThreadFunc) get_servcmd, 
                                        NULL, FALSE, NULL);
                is_recv = FALSE;
                i = 0;
                return FALSE;

            case GBILLING_COMMAND_REJECT:
                gtk_widget_hide (window_init->window);
                gbilling_create_dialog (GTK_MESSAGE_ERROR, NULL, "Error", 
                    "Akses client ditolak, client akan keluar.");
                exit (EXIT_SUCCESS);
                break;

            default:
                gtk_widget_hide (window_init->window);
                gbilling_debug ("Unknown server command %i\n", init_reply);
        }
    }
    i++;

    return TRUE;
}

static gboolean
expire_notify (gpointer data)
{
#ifdef _WIN32
    /* 
     * Di windows, window parent tidak ditampilkan sebelum dialog.
     */
    gtk_window_deiconify ((GtkWindow *) window_timer->window);
#endif
    /* 
     * Dialog ini bisa saja tidak di-respon oleh user, dialog ini akan 
     * di-destroy setelah @window_timer di-destroy.
     */
    gbilling_create_dialog (GTK_MESSAGE_INFO, (GtkWindow *) window_timer->window, 
            "Waktu Paket", "Paket akan habis dalam waktu 10 menit dari sekarang.");

    return FALSE;
}

gboolean
refresh_timer (gpointer data)
{
    if (!window_timer)
        return FALSE;

    GbillingPrepaid *prepaid;
    GbillingMemberGroup *group;
    gchar *tmp;

    tmp = time_to_string (client->duration);
    gtk_window_set_title ((GtkWindow *) window_timer->window, tmp);
    g_free (tmp);
    gtk_entry_set_text ((GtkEntry *) window_timer->entry_username, client->username);
    if (client->type == GBILLING_LOGIN_TYPE_PREPAID)
    {
        prepaid = g_list_nth_data (prepaid_list, client->idtype);
        tmp = (gchar *) prepaid->name;
    }
    else if (client->type == GBILLING_LOGIN_TYPE_MEMBER)
    {
        group = g_list_nth_data (group_list, client->idtype);
        tmp = (gchar *) group->name;
    }
    else
        tmp = (gchar *) gbilling_login_mode[client->type];
    gtk_entry_set_text ((GtkEntry *) window_timer->entry_type, tmp);

    tmp = time_t_to_string (client->start);
    gtk_entry_set_text ((GtkEntry *) window_timer->entry_start, tmp);
    g_free (tmp);

    tmp = time_to_string (client->duration > 0 ? client->duration : 0);
    gtk_entry_set_text ((GtkEntry *) window_timer->entry_duration, tmp);
    g_free (tmp);

    tmp = g_strdup_printf ("%li", client->cost > 0 ? client->cost : 0);
    gtk_entry_set_text ((GtkEntry *) window_timer->entry_cost, tmp);
    g_free (tmp);
    
    return FALSE;
}

gpointer
update_data (gpointer data)
{
    GbillingPrepaid *prepaid;
    gboolean notified = FALSE;

    while (client->islogin)
    {
        client->duration = get_time () - client->start;
        switch (client->type)
        {
            case GBILLING_LOGIN_TYPE_REGULAR:
                client->cost = gbilling_calculate_cost (client->duration, &default_cost);
                break;

            case GBILLING_LOGIN_TYPE_PREPAID:
                prepaid = g_list_nth_data (prepaid_list, client->idtype);
                client->cost = prepaid->cost;
                /*
                 * Notify user jika waktu paket akan habis pada saat
                 * 10 menit (60 * 10) 600 detik sebelum habis dan
                 * Otomatis logout jika waktu paket habis.
                 */
                if ((prepaid->duration - client->duration <= 600) && !notified)
                {
                    notified = TRUE;
                    g_idle_add ((GSourceFunc) expire_notify, NULL);
                }
                else if (prepaid->duration - client->duration <= 0)
                    g_idle_add ((GSourceFunc) do_logout0, GINT_TO_POINTER (TRUE));
                break;

            default:
                return NULL;
        }
        client->duration = get_time () - client->start;
        client->end = get_time ();

        /*
         * FIXME: Durasi dan tarif 0 pada saat login pertama kali 
         * karena get_time() dan client->start sama.
         */
        g_idle_add ((GSourceFunc) refresh_timer, NULL);
        gbilling_sleep (1);
    }
    return NULL;
}

void
fill_chat_log (const gchar      *msg, 
               GbillingChatMode  mode)
{
    g_return_if_fail (msg != NULL);
    g_return_if_fail (mode >= GBILLING_CHAT_MODE_SERVER && 
                      mode <= GBILLING_CHAT_MODE_CLIENT);

    GtkTextBuffer *tbuf;
    GtkTextIter iter;
    GtkTextTag *tag;
    GtkTextMark *mark;
    gchar *buf;
    time_t t = get_time ();
    struct tm *st = localtime (&t);

    tbuf = gtk_text_view_get_buffer ((GtkTextView *) window_chat->textv_log);
    gtk_text_buffer_get_end_iter (tbuf, &iter);
    if (mode == GBILLING_CHAT_MODE_SERVER)
    {
        buf = g_strdup_printf ("%.2i:%.2i  ", st->tm_hour, st->tm_min);
        gtk_text_buffer_insert (tbuf, &iter, buf, -1);
        g_free (buf);
        tag = gtk_text_buffer_create_tag (tbuf, NULL, "weight", PANGO_WEIGHT_BOLD, NULL);
        gtk_text_buffer_insert_with_tags (tbuf, &iter, "Operator", -1, tag, NULL);
        buf = g_strdup_printf (": %s", msg);
        gtk_text_buffer_insert (tbuf, &iter, buf, -1);
        g_free (buf);
    }
    else /* GBILLING_CHAT_MODE_CLIENT */
    {
        buf = g_strdup_printf ("%.2i:%.2i  %s", st->tm_hour, st->tm_min, msg);
        gtk_text_buffer_insert (tbuf, &iter, buf, -1);
        g_free (buf);
    }

    gtk_text_buffer_insert (tbuf, &iter, "\n", -1);
    gtk_text_buffer_get_end_iter (tbuf, &iter);
    mark = gtk_text_buffer_create_mark (tbuf, NULL, &iter, FALSE);
    gtk_text_view_scroll_mark_onscreen ((GtkTextView *) window_chat->textv_log, mark);
}

gboolean
do_login (gpointer data)
{
    GbillingClientSet *cset;
    gboolean send = GPOINTER_TO_INT (data);

    if (send)
    {
        cset = g_new0 (GbillingClientSet, 1);
        cset->type = client->type;
        cset->id = client->idtype;
        g_snprintf (cset->auth.username, sizeof(cset->auth.username), client->username);
        send_login (cset);
        g_free (cset);
    }

    gtk_widget_hide (window_main->window);
    gtk_widget_hide (window_login->window);
    client->islogin = TRUE;

    switch (client->type)
    {
        case GBILLING_LOGIN_TYPE_ADMIN:
          create_window_setting ();
          set_window_setting ();
          gtk_widget_show (window_setting->window);
          break;

        default:
          create_window_timer ();
          g_thread_create ((GThreadFunc) update_data, NULL, TRUE, NULL);
          gtk_widget_show (window_timer->window);
          g_timeout_add (3000, (GSourceFunc) hide_window_timer, NULL);
          break;
    }
    return FALSE;
}

/*
 * Tampilkan window_init setelah beberapa saat setelah window_main 
 * ditampilkan, ini trik agar window_init akan menjadi window yang 
 * terfokus saat kedua window ditampilkan.
 */
static gboolean
show_window_init (gpointer data)
{
    gtk_widget_show (window_init->window);
    return FALSE;
}

/** 
 * do_logout:
 * @send: Apakah perintah login akan dikirim.
 * Returns: TRUE jika logout berhasil, FALSE jika gagal.
 *
 * Logout client, GBILLING_LOGIN_TYPE_ADMIN tidak mengagalkan
 * fungsi ini jika data tidak berhasil dikirim.
 */
gboolean
do_logout (gboolean send)
{
    switch (client->type)
    {
        case GBILLING_LOGIN_TYPE_ADMIN:
            if (send)
                send_command (GBILLING_COMMAND_LOGOUT);
            gtk_widget_destroy (window_setting->window);
            g_free (window_setting);
            client->isadmin = FALSE;

            break;

        default:
            if (send)
                if (!send_command (GBILLING_COMMAND_LOGOUT))
                    return FALSE;
            gtk_widget_destroy (window_timer->window);
            g_free (window_timer);
            window_timer = NULL;

            if (window_chat)
                gtk_widget_destroy (window_chat->window);
            if (window_item)
                gtk_widget_destroy (window_item->window);
    }

    gtk_widget_show (window_main->window);
    g_timeout_add (250, (GSourceFunc) show_window_init, NULL);
    g_timeout_add (1000, (GSourceFunc) first_init, NULL);
    client->islogin = FALSE;

    /* isi data terakhir pemakaian */
    if (last)
    {
        g_free (last->username);
        g_free (last);
    }
    last = g_memdup (client, sizeof(struct _client));
    last->username = g_strdup (client->username);

    return TRUE;
}

/**
 * Wrapper do_logout(), untuk dipanggil selain di main thread.
 */
static gboolean
do_logout0 (gpointer data)
{
    g_return_val_if_fail (data != NULL, FALSE);

    do_logout (GPOINTER_TO_INT (data));
    return FALSE;
}

static gboolean
do_chat (gpointer data)
{
    g_return_val_if_fail (data != NULL, FALSE);

    if (!window_chat)
    {
        create_window_chat ();
        gtk_widget_show (window_chat->window);
    }
    else
        gtk_window_present ((GtkWindow *) window_chat->window);
    fill_chat_log (data, GBILLING_CHAT_MODE_SERVER);

    return FALSE;
}

/**
 * server tidak seharusnya me-logout client pada waktu 
 * restart/shutdown. ini jika saja data gagal dikirim 
 * dan client masih login (di server, status client telah
 * logout), biarkan client mengirim perintah logout!
 */
static gboolean
do_control (gpointer data)
{
    // do_logout (GINT_TO_POINTER (TRUE));
    gbilling_control_client (GPOINTER_TO_INT (data));

    return FALSE;
}

gboolean
send_chat (const char *msg)
{
    g_return_val_if_fail (msg != NULL, FALSE);

    gchar buffer[129];
    gint s, op;
    gboolean ret = FALSE;
    gsize len, bytes = 0;
    gint8 cmd = GBILLING_COMMAND_CHAT;

    s = gbilling_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
        return ret;
    len = sizeof(client->saddr);
    op = gbilling_connect (s, (struct sockaddr *) &client->saddr, len);
    if (op == -1)
        goto done;
    op = gbilling_send (s, &cmd, sizeof(cmd), 0);
    if (op == -1)
        goto done;
    g_snprintf (buffer, sizeof(buffer) - 1, msg);
    
    len = strlen (msg);
    g_snprintf (buffer, sizeof(buffer), msg);
    buffer[sizeof(buffer) - 1] = 0;
    while (bytes < len)
    {
        op = gbilling_send (s, &buffer + bytes, len - bytes, 0);
        if (op == -1)
            goto done;
        bytes += op;
    }
    ret = TRUE;
    done:
    gbilling_close (s);
    return ret;
}


