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
 *  gbilling.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _WIN32
#  ifndef WINVER
#   define WINVER 0x0500
#  endif
# define _WIN32_WINNT WINVER
# include <windows.h>
# include <winsock2.h>
# include <winuser.h>
#else
# include <fcntl.h>
# include <unistd.h>
# include <signal.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/file.h>
# include <arpa/inet.h>
#endif

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <glib.h>
#include <glib/gstdio.h>

#ifdef GSTREAMER_SUPPORT
# include <gst/gst.h>
#endif

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#ifdef SQLITE_SUPPORT
# include "sqlite3.h"
#endif

#include "sockc.h"
#include "gbilling.h"
#include "dev.h"
#include "version.h"

/* tipe login */
const gchar * const gbilling_login_mode[] =
{
    "Admin",
    "Reguler",
    "Paket",
    "Member"
};

/* model tarif */
const gchar * const gbilling_cost_mode[] =
{
    "Menit Pertama",
    "Flat"
};

static gint debug_vargs (const gchar*, va_list);

#ifndef _WIN32
static gint lockfl;
#else
static FILE *lockfl = NULL;
#endif

#ifdef _WIN32
gboolean use_win32 = TRUE;
#else
gboolean use_win32 = FALSE;
#endif

#ifdef _WIN32
static gint lock_file_win32 (const gchar*);
static gint unlock_file_win32 (void);
static gboolean url_show_win32 (const gchar*);
static gboolean control_client_win32 (GbillingControl);
#else
static gint lock_file_unix (const gchar*);
static gint unlock_file_unix (void);
static gboolean url_show_unix (const gchar*);
static gboolean control_client_unix (GbillingControl);
#endif

/**
 * gbilling_data_send:
 * @s: Socket descriptor.
 * @data: Data yang akan dikirim.
 * @size: Jumlah byte yang akan dikirim.
 *
 * Returns: Jumlah byte yang dikirim, -1 jika gagal.
 *
 * Fungsi umum untuk kirim @data dengan panjang data @size.
 */
gssize
gbilling_data_send (gint          s,
                    gconstpointer data,
                    gsize         size)
{
    gint ret;
    gssize bytes = 0;

    while (bytes < size)
    {
        ret = gbilling_send (s, (gchar*) data + bytes, size - bytes, 0);
        if (ret <= 0)
            return ret;
        bytes += ret;
    }
    return bytes;
}

/**
 * gbilling_data_recv:
 * @s: Socket descriptor.
 * @data: Buffer untuk menerima data.
 * @size: Jumlah data yang akan diterima.
 *
 * Returns: Jumlah byte yang diterima, -1 jika gagal.
 * 
 * Fungsi umum untuk menerima @data dengan panjang data @size.
 */
gssize
gbilling_data_recv (gint     s,
                    gpointer data,
                    gsize    size)
{
    gint ret;
    gssize bytes = 0;

    while (bytes < size)
    {
        ret = gbilling_recv (s, (gchar*) data + bytes, size - bytes, 0);
        if (ret <= 0)
            return ret;
        bytes += ret;
    }
    return bytes;
}

/** 
 * gbilling_clientset_send:
 * @fd:
 * @set:
 *
 * Returns:
 *
 */
gboolean
gbilling_clientset_send (gint                     fd,
                         const GbillingClientZet *set)
{
    GbillingClientZet tmp;
    gint s;
    gsize len = 0;
    gboolean ret = FALSE;

    memcpy (&tmp, set, sizeof(tmp));
    tmp.start = g_htonl (tmp.start);
    while (len < sizeof(tmp))
    {
        if ((s = gbilling_send (fd, (gchar *) &tmp + len, sizeof(tmp) - len, 0)) == -1)
            return ret;
        len += s;
    }
    ret ^= 1;
    return ret;
}

/** 
 * gbilling_clientset_recv:
 * @fd:
 * @set:
 *
 * Returns:
 *
 */
gboolean
gbilling_clientset_recv (gint               fd,
                         GbillingClientZet *set)
{
    GbillingClientZet tmp;
    gint s;
    gsize len = 0;
    gboolean ret = FALSE;

    while (len < sizeof(tmp))
    {
        if ((s = gbilling_recv (fd, (gchar *) &tmp + len, sizeof(tmp) - len, 0)) == -1)
            return ret;
        len += s;
    }
    tmp.start = g_ntohl (tmp.start);
    memcpy (set, &tmp, sizeof(tmp));
    ret ^= 1;
    return ret;
}

/**
 * gbilling_str_checksum:
 * @str: String yang akan di hash.
 *
 * Returns: Hasil hash dari string @str, dealokasi dengan g_free().
 *
 * Hash string dengan metode hash MD5, string harus null-terminated.
 */
gchar*
gbilling_str_checksum (const gchar *str)
{
    return g_compute_checksum_for_string (G_CHECKSUM_MD5, str, -1);
}

/**
 * gbilling_server_info_new:
 *
 * Returns: Data kosong %GbillingServerInfo, dealokasi dengan gbilling_server_info_free().
 *
 * Buat data %GbillingServerInfo.
 */
GbillingServerInfo*
gbilling_server_info_new (void)
{
    GbillingServerInfo *info;
    info = g_malloc0 (sizeof(GbillingServerInfo));
    return info;
}

/**
 * gbilling_server_info_free:
 *
 * @info: Data %GbillingServerInfo yang akan di dealokasi.
 *
 * Dealokasi data %GbillingServerInfo.
 */
void
gbilling_server_info_free (GbillingServerInfo *info)
{
    if (info)
        g_free (info);
}

/**
 * gbilling_server_info_send:
 * @fd:
 * @info:
 *
 * Returns: #TRUE jika data berhasil dikirim, #FALSE jika gagal.
 *
 * Kirim struktur #GbillingServerInfo.
 */
gboolean
gbilling_server_info_send (gint                      fd,
                           const GbillingServerInfo *info)
{
    g_return_val_if_fail (info != NULL, FALSE);

    gint s;
    gsize len = 0;
    gboolean ret = FALSE;
    
    while (len < sizeof(*info))
    {
        if ((s = gbilling_send (fd, (gchar *) info + len, 
            sizeof(*info) - len, 0)) <= 0)
            return ret;
        len += s;
    }
    ret ^= 1;
    return ret;
}

/**
 * gbilling_server_info_recv:
 *
 * @fd: Socket descriptor yang telah dibuka.
 * @info:
 *
 * Terima info internet cafe dari server.
 */
gboolean
gbilling_server_info_recv (gint                fd,
                           GbillingServerInfo *info)
{
    GbillingServerInfo tmp;
    gboolean ret = FALSE;
    gint s;
    gsize len = 0;

    while (len < sizeof(tmp))
    {
        if ((s = gbilling_recv (fd, &tmp + len, sizeof(tmp) - len, 0)) <= 0)
            return ret;
        len += s;
    }
    memcpy (info, &tmp, sizeof(tmp));
    ret ^= 1;
    return ret;
}

/**
 * gbilling_client_new:
 * 
 * Returns: Data client, dealokasi dengan gbilling_client_free().
 * Buat data client %GbillingClient yang baru.
 */
GbillingClient*
gbilling_client_new (void)
{
    GbillingClient *client;
    client = g_malloc (sizeof(GbillingClient));
    client->name = NULL;
    client->ip = NULL;
    return client;
}

/**
 * gbilling_client_free:
 * @client: Data client.
 * 
 * Dealokasi data client %GbillingClient.
 */
void
gbilling_client_free (GbillingClient *client)
{
    if (client)
    {
        if (client->name)
            g_free (client->name);
        if (client->ip)
            g_free (client->ip);
        g_free (client);
    }
}

static void
gbilling_client_list_element_free (GbillingClient *client,
                                   gpointer        data)
{
    if (client)
        gbilling_client_free (client);
}

void
gbilling_client_list_free (GbillingClientList *list)
{
    if (list)
    {
        g_list_foreach (list, (GFunc) gbilling_client_list_element_free, NULL);
        g_list_free (list);
    }
}

/**
 * gbilling_cost_new:
 *
 * Buat struktur #GbillingCost.
 * Returns: #GbillingPrepaid, dealokasi dengan gbilling_cost_free().
 */
GbillingCost*
gbilling_cost_new (void)
{
    GbillingCost *cost;
    cost = g_malloc0 (sizeof(GbillingCost));
    return cost;
}

/**
 * gbilling_cost_free:
 * 
 * Dealokasi #GbillingCost.
 * @cost: #GbillingCost yang akan didealokasi.
 */
void
gbilling_cost_free (GbillingCost *cost)
{
    if (cost)
        g_free (cost);
}

static void
gbilling_cost_list_element_free (GbillingCost *cost,
                                 gpointer      data)
{
    if (cost)
        gbilling_cost_free (cost);
}

/**
 * gbilling_cost_list_free:
 * @list: #GbillingCostList yang akan di dealokasikan.
 *
 * Dealokasi #GbillingCostList dan semua member-nya.
 */
void
gbilling_cost_list_free (GbillingCostList *list)
{
    if (list)
    {
        g_list_foreach (list, (GFunc) gbilling_cost_list_element_free, NULL);
        g_list_free (list);
    }
}

/**
 * gbilling_cost_list_send:
 *
 * @fd: Socket descriptor.
 * @list: List tarif %GbillingCostList.
 *
 * Returns: %TRUE jika data berhasil dikirim, %FALSE jika gagal.
 *
 * Kirim list tarif.
 */
gboolean
gbilling_cost_list_send (gint              fd,
                         GbillingCostList *list)
{
    g_return_val_if_fail (list != NULL, FALSE);
    
    GbillingCostList *ptr;
    GbillingCost *cost;
    gint16 list_len;
    gsize len;
    gint s;
    gboolean ret = FALSE;
    
    list_len = g_htons (g_list_length (list));
    if ((s = gbilling_send (fd, &list_len, sizeof(list_len), 0)) <= 0)
        goto done;
    
    for (ptr = list; ptr; ptr = ptr->next)
    {
        cost = g_memdup (ptr->data, sizeof(*cost));
        cost->fcost = g_htonl (cost->fcost);
        cost->icost = g_htonl (cost->icost);
        len = 0;
        while (len < sizeof(*cost))
        {
            if ((s = gbilling_send (fd, cost + len, sizeof(*cost) - len, 0)) <= 0)
            {
                gbilling_cost_free (cost);
                goto done;
            }
            len += s;
        }
        gbilling_cost_free (cost);
    }
    ret ^= 1;
    done:
    return ret;
}

/**
 * gbilling_cost_list_recv:
 *
 * @fd: Socket descriptor.
 * @list: List tarif %GbillingCostList yang akan di-isikan.
 *
 * Returns: %TRUE jika data berhasil dikirim, %FALSE jika gagal.
 *
 * Kirim list tarif.
 */
gboolean
gbilling_cost_list_recv (gint               fd,
                         GbillingCostList **list)
{
    GbillingCostList *clist = NULL;
    GbillingCost *cost;
    gint16 list_len;
    gsize len;
    gint s;
    gboolean ret = FALSE;
    
    if ((s = gbilling_recv (fd, &list_len, sizeof(list_len), 0)) <= 0)
        goto done;
    list_len = g_ntohs (list_len);

    while (list_len--)
    {
        cost = gbilling_cost_new ();
        len = 0;
        while (len < sizeof(*cost))
        {
            if ((s = gbilling_recv (fd, cost + len, sizeof(*cost) - len, 0)) <= 0)
            {
                gbilling_cost_free (cost);
                gbilling_cost_list_free (clist);
                goto done;
            }
            len += s;
        }
        cost->fcost = g_ntohl (cost->fcost);
        cost->icost = g_ntohl (cost->icost);
        clist = g_list_prepend (clist, cost);
    }
    clist = g_list_reverse (clist);
    *list = clist;
    ret ^= 1;
    
    done:
    return ret;
}
    

/**
 * gbilling_cost_per_host:
 *
 * Hitung tarif sesuai dengan #GbillingCost.
 * @cost: Data tarif.
 * Returns: Total tarif per-jam, atau 0 jika gagal.
 */
gint32
gbilling_cost_per_hour (const GbillingCost *cost)
{
    g_return_val_if_fail (cost != NULL && cost->imin != 0, 0);
    gint32 ret = 0;

    if (cost->mode == GBILLING_COST_MODE_FMIN)
        ret = cost->fcost + (((60 - cost->fmin) / cost->imin) * cost->icost);
    else
        ret = cost->icost * (60 / cost->imin);
    return ret;
}

/**
 * gbilling_cost_send:
 * @fd:
 * @cost:
 *
 * Returns:
 *
 */
gboolean
gbilling_cost_send (gint                fd,
                    const GbillingCost *cost)
{
    g_return_val_if_fail (cost != NULL, FALSE);

    GbillingCost tmp;
    gboolean ret = FALSE;
    gint s;
    gsize len = 0;

    memcpy (&tmp, cost, sizeof(tmp));
    tmp.fcost = g_htonl (cost->fcost);
    tmp.icost = g_htonl (cost->icost);

    while (len < sizeof(tmp))
    {
        if ((s = gbilling_send (fd, (gchar *) &tmp + len, sizeof(tmp) - len, 0)) <= 0)
            return ret;
        len += s;
    }
    ret ^= 1;
    return ret;
}

/**
 * gbilling_cost_recv:
 * @fd:
 * @cost:
 *
 * Returns:
 *
 */
gboolean
gbilling_cost_recv (gint          fd,
                    GbillingCost *cost)
{
    GbillingCost tmp;
    gint s;
    gsize len = 0;
    gboolean ret = FALSE;

    while (len < sizeof(tmp))
    {
        if ((s = gbilling_recv (fd, (gchar *) &tmp + len, sizeof(tmp) - len, 0)) <= 0)
            return ret;
        len += s;
    }
    tmp.fcost = g_ntohl (tmp.fcost);
    tmp.icost = g_ntohl (tmp.icost);
    memcpy (cost, &tmp, sizeof(tmp));
    ret ^= 1;
    return ret;
}
    
/**
 * gbilling_prepaid_new:
 *
 * Buat struktur #GbillingPrepaid yang baru.
 * Returns: Struktur baru #GbillingPrepaid, dealokasi dengan gbilling_prepaid_free().
 */
GbillingPrepaid*
gbilling_prepaid_new (void)
{
    return g_malloc0 (sizeof(GbillingPrepaid));
}

/**
 * gbilling_prepaid_free:
 * Dealokasi memori data #GbillingPrepaid.
 * @prepaid: Pointer #GbillingPrepaid yang akan di-dealokasikan.
 */
void
gbilling_prepaid_free (GbillingPrepaid *prepaid)
{
    if (prepaid)
        g_free (prepaid);
}

static void
gbilling_prepaid_list_element_free (GbillingPrepaid *prepaid,
                                    gpointer         data)
{
    gbilling_prepaid_free (prepaid);
}

void
gbilling_prepaid_list_free (GbillingPrepaidList *list)
{
    if (list)
    {
        g_list_foreach (list, (GFunc) gbilling_prepaid_list_element_free, NULL);
        g_list_free (list);
    }
}

/**
 * gbilling_prepaid_list_send:
 * @fd:
 * @list:
 *
 * Returns:
 *
 * Kirim hanya nama prepaid.
 */
gboolean
gbilling_prepaid_list_send (gint                 fd,
                            GbillingPrepaidList *list)
{    
    GbillingPrepaidList *ptr;
    GbillingPrepaid tmp;
    gint16 list_len;
    gint s;
    gsize len;
    gboolean ret = FALSE;

    list_len = g_htons (g_list_length (list));
    if ((s = gbilling_send (fd, &list_len, sizeof(list_len), 0)) <= 0)
        return ret;
    if (list_len == 0)
        return TRUE;

    for (ptr = list; ptr; ptr = ptr->next)
    {
        memcpy (&tmp, ptr->data, sizeof(tmp));
        tmp.duration = g_htonl (tmp.duration);
        tmp.cost = g_htonl (tmp.cost);
        tmp.active = g_htons (tmp.active);
        len = 0;
        while (len < sizeof(tmp))
        {
            if ((s = gbilling_send (fd, &tmp + len, sizeof(tmp) - len, 0)) <= 0) 
                return ret;
            len += s;
        }
    }
    ret ^= 1;
    return ret;
}

#define assign_bytes(ptr, bytes) (if ((ptr)) (*ptr) = (bytes))

/**
 * gbilling_prepaid_list_recv:
 * @fd: Socket descriptor.
 * @list: %GbillingPrepaidList yang disikan.
 *
 * Returns:
 *
 * Terima %GbillingPrepaidList data list paket, gunakan hanya nama %prepaid->name.
 */
gboolean
gbilling_prepaid_list_recv (gint                  fd,
                            GbillingPrepaidList **list)
{
    GbillingPrepaidList *plist = NULL;
    GbillingPrepaid *prepaid;
    gint16 list_len;
    gsize len;
    gint s;
    gboolean ret = FALSE;

    if ((s = gbilling_recv (fd, &list_len, sizeof(list_len), 0)) <= 0)
        goto done;
    list_len = g_ntohs (list_len);
    if (list_len == 0)
        return TRUE;

    while (list_len--)
    {
        prepaid = gbilling_prepaid_new ();
        len = 0;
        while (len < sizeof(*prepaid))
        {
            if ((s = gbilling_recv (fd, prepaid + len, sizeof(*prepaid) - len, 0)) <= 0)
            {
                gbilling_prepaid_free (prepaid);
                if (plist)
                    gbilling_prepaid_list_free (plist);
                goto done;
            }
            len += s;
        }
        prepaid->name[sizeof(prepaid->name) - 1] = 0;
        prepaid->duration = g_ntohl (prepaid->duration);
        prepaid->cost = g_ntohl (prepaid->cost);
        prepaid->active = g_ntohs (prepaid->active);
        plist = g_list_prepend (plist, prepaid);
    }
    plist = g_list_reverse (plist);
    *list = plist;
    ret ^= 1;

    done:
    return ret;
}

/**
 * gbilling_item_new:
 *
 * Buat struktur baru #GbillingItem.
 * Returns: Struktur #GbillingItem yang baru, dealokasi dengan gbilling_item_free().
 */
GbillingItem*
gbilling_item_new (void)
{
    return g_malloc0 (sizeof(GbillingItem));
}

/**
 * gbilling_item_free:
 *
 * Dealokasi struktur #GbillingItem.
 * @item: Struktur #GbillingItem.
 */
void
gbilling_item_free (GbillingItem *item)
{   
    if (item)
        g_free (item);
}

static void
gbilling_item_list_element_free (GbillingItem *item,
                                 gpointer      data)
{
    if (item)
        gbilling_item_free (item);
}

void
gbilling_item_list_free (GbillingItemList *list)
{
    if (list)
    {
        g_list_foreach (list, (GFunc) gbilling_item_list_element_free, NULL);
        g_list_free (list);
    }
}

/**
 * gbilling_item_list_send:
 * @fd: Socket descriptor.
 * @list: %GbillingItemList yang akan dikirim.
 *
 * Returns: %TRUE jika list berhasil dikirim, %FALSE jika gagal.
 *
 * Kirim list item (Daftar Menu).
 */
gboolean
gbilling_item_list_send (gint              fd,
                         GbillingItemList *list)
{
    GbillingItemList *ptr;
    GbillingItem *item, *tmp;
    gint16 list_len;
    gint s;
    gsize len;
    gboolean ret = FALSE;

    list_len = g_htons (g_list_length (list));
    if ((s = gbilling_send (fd, &list_len, sizeof(list_len), 0)) <= 0)
        goto done;
    if (list_len == 0)
        return TRUE;

    for (ptr = list; ptr; ptr = ptr->next)
    {
        item = ptr->data;
        tmp = g_memdup (item, sizeof(*item));
        tmp->cost = g_htonl (tmp->cost);
        tmp->id = g_htons (tmp->id);
        len = 0;
        while (len < sizeof(*item))
        {
            if ((s = gbilling_send (fd, (gchar *) tmp + len, 
                 sizeof(*item) - len, 0)) <= 0)
            {
                gbilling_item_free (tmp);
                goto done;
            }
            len += s;
        }
        gbilling_item_free (tmp);
    }
    ret ^= 1;

    done:
    return ret;
}

/**
 * gbilling_item_list_recv:
 * @fd: Socket descriptor.
 * @list: %GbillingItemList yang disikan.
 *
 * Returns:
 *
 * Terima %GbillingItemList data list item.
 */
gboolean
gbilling_item_list_recv (gint               fd,
                         GbillingItemList **list)
{
    GbillingItemList *ilist = NULL;
    GbillingItem *item = NULL;
    gint16 list_len;
    gint s;
    gsize len;
    gboolean ret = FALSE;

    if ((s = gbilling_recv (fd, &list_len, sizeof(list_len), 0)) <= 0)
        goto done;
    list_len = g_ntohs (list_len);
    if (list_len == 0)
        return TRUE;

    while (list_len--)
    {
        item = gbilling_item_new ();
        len = 0;
        while (len < sizeof(*item))
        {
            if ((s = gbilling_recv (fd, (gchar *) item + len, 
                sizeof(*item) - len, 0)) <= 0)
            {
                gbilling_item_free (item);
                if (ilist)
                    gbilling_item_list_free (ilist);
                goto done;
            }
            len += s;
        }
        item->cost = g_ntohl (item->cost);
        item->id = g_ntohs (item->id);
        ilist = g_list_prepend (ilist, item);
    }
    ilist = g_list_reverse (ilist);
    *list = ilist;
    ret ^= 1;

    done:
    return ret;
}


GbillingMemberGroup*
gbilling_member_group_new (void)
{
    return g_malloc0 (sizeof(GbillingMemberGroup));
}

void
gbilling_member_group_free (GbillingMemberGroup *group)
{
    if (group)
        g_free (group);
}

static void
gbilling_member_group_list_element_free (GbillingMemberGroup *group,
                                         gpointer             data)
{
    if (group)
        gbilling_member_group_free (group);
}

void
gbilling_member_group_list_free (GbillingMemberGroupList *list)
{
    if (list)
    {
        g_list_foreach (list, (GFunc) gbilling_member_group_list_element_free, NULL);
        g_list_free (list);
    }
}


/**
 * gbilling_member_group_list_send:
 * @fd: Socket descriptor.
 *
 * Returns: #TRUE jika data berhasil dikirim, #FALSE jika gagal.
 *
 * Kirim list data %GbillingMemberGroupList.
 */
gboolean
gbilling_member_group_list_send (gint                     fd,
                                 GbillingMemberGroupList *list)
{
    GbillingMemberGroupList *ptr;
    GbillingMemberGroup *group, *tmp;
    gint16 len;
    gint s;
    gboolean ret = FALSE;

    len = g_htons (g_list_length (list));
    if ((s = gbilling_send (fd, &len, sizeof(len), 0)) <= 0) 
        goto done;

    for (ptr = list; ptr; ptr = ptr->next)
    {
        group = ptr->data;
        tmp = g_memdup (group, sizeof(*tmp));
        tmp->id = g_htons (tmp->id);
        len = 0;
        while (len < sizeof(*tmp))
        {
            if ((s = gbilling_send (fd, tmp + len, sizeof(*tmp) - len, 0)) <= 0)
            {
                gbilling_member_group_free (tmp);
                goto done;
            }
            len += s;
        }
        gbilling_member_group_free (tmp);
    }
    ret ^= 1;

    done:
    return ret;
}

/**
 * gbilling_member_group_list_recv:
 * @fd: Socket descriptor.
 *
 * Returns: #TRUE jika data berhasil diterima, #FALSE jika gagal.
 *
 * Terima list data %GbillingMemberGroupList.
 */
gboolean
gbilling_member_group_list_recv (gint                      fd,
                                 GbillingMemberGroupList **list)
{
    GbillingMemberGroupList *mlist = NULL;
    guint16 list_len;
    gint s;
    gsize len;
    gboolean ret = FALSE;

    if ((s = gbilling_recv (fd, &list_len, sizeof(list_len), 0)) <= 0) 
        goto done;
    list_len = g_ntohs (list_len);

    while (list_len--)
    {
        GbillingMemberGroup *group;       
        group = gbilling_member_group_new ();
        len = 0;
        
        while (len < sizeof(*group))
        {
            if ((s = gbilling_recv (fd, group + len, sizeof(*group) - len, 0)) <= 0)
            {
                gbilling_member_group_free (group);
                gbilling_member_group_list_free (mlist);
                goto done;
            }
            len += s;
        }
        group->id = g_ntohs (group->id);
        mlist = g_list_prepend (mlist, group);
    }
    mlist = g_list_reverse (mlist);
    *list = mlist;
    ret ^= 1;

    done:
    return ret;
}

/**
 * gbilling_member_new:
 * Buat struktur #GbillingMember yang baru.
 * Returns: Struktur baru #GbillingMember, dealokasi dengan gbilling_member_free().
 */
GbillingMember*
gbilling_member_new (void)
{
    return g_malloc0 (sizeof(GbillingMember));
}

/**
 * Dealokasi memori data #GbillingMember.
 * @member: Pointer GbillingMember yang akan di-dealokasikan.
 */
void
gbilling_member_free (GbillingMember *member)
{
    if (member)
    {
        if (member->username)
            g_free (member->username);
        if (member->group)
            g_free (member->group);
        if (member->pass)
            g_free (member->pass);
        if (member->fullname)
            g_free (member->fullname);
        if (member->address)
            g_free (member->address);
        if (member->phone)
            g_free (member->phone);
        if (member->email)
            g_free (member->email);
        if (member->idcard)
            g_free (member->idcard);
        g_free (member);
    }
}

static void
gbilling_member_list_element_free (GbillingMember *member,
                                   gpointer        data)
{
    if (member)
        gbilling_member_free (member);
}

void
gbilling_member_list_free (GbillingMemberList *list)
{
    if (list)
    {
        g_list_foreach (list, (GFunc) gbilling_member_list_element_free, NULL);
        g_list_free (list);
    }
}

/**
 * gbilling_auth_new:
 *
 * @username: Username yang akan di hash.
 * @passwd: Password yang akan di hash.
 * Returns: %GbillingAuth dengan username dan password yang di hash.
 *
 *  Buat data autentikasi dengan username dan password yang telah di hash, 
 *  free dengan gbilling_auth_free().
 */
GbillingAuth*
gbilling_auth_new (const gchar *username,
                   const gchar *passwd)
{
    g_return_val_if_fail (username != NULL && passwd != NULL, NULL);
    GbillingAuth *auth;
    gchar *tmp;
    
    auth = g_malloc (sizeof(GbillingAuth));
    g_snprintf (auth->username, sizeof(auth->username), username);
    tmp = gbilling_str_checksum (passwd);
    g_snprintf (auth->passwd, sizeof(auth->passwd), tmp);
    g_free (tmp);
    return auth;
}

/**
 * gbilling_auth_free:
 * @auth: Data autentikasi.
 *
 * Dealokasi data autentikasi %GbillingAuth.
 */
void
gbilling_auth_free (GbillingAuth *auth)
{
    if (auth)
        g_free (auth);
}

/**
 * gbilling_auth_compare:
 * @auth1: Data autentikasi pertama.
 * @auth2: Data autentikasi kedua.
 *
 * Returns: #TRUE jika data sama, #FALSE jika berbeda.
 *
 * Bandingkan data autentikasi.
 */
gboolean
gbilling_auth_compare (const GbillingAuth *auth1,
                       const GbillingAuth *auth2)
{
    g_return_val_if_fail (auth1 != NULL && auth2 != NULL, FALSE);

    return (!strcmp (auth1->username, auth2->username) &&
            !strcmp (auth1->passwd, auth2->passwd));
}

/**
 * gbilling_auth_send:
 * @fd: Socket descriptor.
 * @auth: Data autentikasi.
 *
 * Returns: #TRUE jika berhasil dikirim, #FALSE jika gagal.
 *
 * Kirim data autentikasi.
 */
gboolean
gbilling_auth_send (gint          fd,
                    GbillingAuth *auth)
{
    g_return_val_if_fail (auth != NULL, FALSE);
    gint s;
    gsize bytes = 0;
    gboolean ret = FALSE;
    
    while (bytes < sizeof(*auth))
    {
        if ((s = gbilling_send (fd, auth + bytes, sizeof(*auth) - bytes, 0)) == -1)
            return ret;
        bytes += s;
    }
    ret ^= 1;
    return ret;
}

/**
 * gbilling_auth_recv:
 * @fd: Socket descriptor.
 * @auth: Data autentikasi.
 *
 * Returns: #TRUE jika berhasil diterima, #FALSE jika gagal.
 *
 * Terima data autentikasi.
 */
gboolean
gbilling_auth_recv (gint          fd,
                    GbillingAuth *auth)
{
    GbillingAuth tmp;
    gint s;
    gsize bytes = 0;
    gboolean ret = FALSE;
    
    while (bytes < sizeof(tmp))
    {
        if ((s = gbilling_recv (fd, &tmp + bytes, sizeof(tmp) - bytes, 0)) == -1)
            return ret;
        bytes += s;
    }
    memcpy (auth, &tmp, sizeof(tmp));
    ret ^= 1;
    return ret;
}    

/**
 * gbilling_chat_new:
 *
 * Returns:
 *
 */
GbillingChat*
gbilling_chat_new (void)
{
    return g_malloc0 (sizeof(GbillingChat));
}

/**
 * gbilling_chat_free:
 * @chat:
 * 
 * Returns:
 *
 */
void
gbilling_chat_free (GbillingChat *chat)
{
    if (chat)
    {
        g_free (chat->msg);
        g_free (chat);
    }
}

gboolean
gbilling_server_cmd_send (gint               fd,
                          GbillingServerZmd *cmd)
{
    GbillingServerZmd scmd;
    gint s;
    gsize bytes = 0;
    
    memcpy (&scmd, cmd, sizeof(scmd));
    scmd.tstart = g_htonl (scmd.tstart);
    while (bytes < sizeof(scmd))
    {
        if ((s = gbilling_send (fd, &scmd + bytes, sizeof(scmd) - bytes, 0)) <= 0)
            return FALSE;
        bytes += s;
    }
    return TRUE;
}

gint
gbilling_server_cmd_recv (gint               fd,
                          GbillingServerZmd *cmd)
{
    GbillingServerZmd scmd;
    gint s;
    gsize bytes = 0;
    
    while (bytes < sizeof(scmd))
    {
        s = gbilling_recv (fd, &scmd + bytes, sizeof(scmd) - bytes, 0);
        if (s == 0) return 0;
        else if (s == -1) return -1;
        bytes += s;
    }
    scmd.tstart = g_ntohl (scmd.tstart);
    memcpy (cmd, &scmd, sizeof(*cmd));
    return bytes;
}

/**
 * gbilling_log_new:
 *
 * Returns: Struktur %GbillingLog yang telah dialokasi.
 *
 * Buat struktur %GbillingLog yang telah dialokasi, free dengan gbilling_log_free().
 */
GbillingLog*
gbilling_log_new (void)
{
    GbillingLog *log;
    log = g_malloc0 (sizeof(GbillingLog));
    return log;
}

/**
 * gbilling_log_free:
 *
 * Dealokasi struktur %GbillingLog.
 */
void
gbilling_log_free (GbillingLog *log)
{
    if (log)
    {
        g_free (log->client);
        g_free (log->username);
        g_free (log->type);
        g_free (log->desc);
        g_free (log);
    }
}

static gint
debug_vargs (const gchar *format,
             va_list      args)
{
    g_return_val_if_fail (format != NULL, -1);

    gchar *arg_s;
    gint retval;
    
    arg_s = g_strdup_vprintf (format, args);
    retval = g_fprintf (stderr, "%s", arg_s);
    g_free (arg_s);

    return retval;
}

/**
 * debug ke stdout (shell, terminal)
 * params: @format, printf() format
 */
gint
gbilling_debug (const gchar *format, ...)
{
    g_return_val_if_fail (format != NULL, -1);

    va_list args;

    va_start (args, format);
    debug_vargs (format, args);
    va_end (args);

    return 0;
}

gint
gbilling_sysdebug0 (const gchar *file,
                    const gchar *func,
                    const gchar *msg,
                    gint         line)
{
    g_return_val_if_fail (file && func && msg, -1);
    gint retval;

#ifndef _WIN32
    retval = g_fprintf (stderr, "(%s:%i) %s: %s: %s\n", file, line, func, 
                        msg, g_strerror (errno));
#else
    retval = g_fprintf (stderr, "(%s:%i) %s: %s: error %i\n", file, line, func,
                        msg, WSAGetLastError ());
#endif
    return retval;
}

gint
gbilling_prepare_dir (void)
{
    gint ret, mode = 0;
    gchar *dir;

    dir = g_build_filename (g_get_home_dir (), ".gbilling", NULL);
#ifndef _WIN32
    /* di win32 mode diabaikan, ini hanya wrapper mkdir() */
    mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; /* 0755 */
#endif
    ret = g_mkdir (dir, mode);
    g_free (dir);
    if (ret == -1)
        if (errno == EEXIST)
            ret = 0;

    return ret;
}

#ifndef _WIN32
static gint
lock_file_unix (const gchar *filename)
{
    gint ret;
    gchar *path;

    path = g_build_filename ("/tmp", filename, NULL);
    lockfl = open (path, O_RDWR | O_CREAT, 0600);
    g_free (path);
    ret = lockf (lockfl, F_TLOCK, 0);

    return ret;
}

static gint
unlock_file_unix (void)
{
    gint ret;

    ret = lockf (lockfl, F_ULOCK, 0);
    if (ret == -1) return ret;
    close (lockfl);

    return ret;
}
#endif

#ifdef _WIN32
static gint
lock_file_win32 (const gchar *filename)
{
    gchar *path, *current_dir;
    gboolean test;
    gint ret;

    current_dir = g_get_current_dir ();
    path = g_build_filename (current_dir, filename, NULL);
    g_free (current_dir);
    test = g_file_test (path, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
    if (test)
    {
        ret = g_remove (path);
        if (ret == -1)
        {
            g_free (path);
            return ret;
        }
    }
    lockfl = g_fopen (path, "w");
    g_free (path);
    if (!lockfl)
        return -1;
    return 0;
}

static gint
unlock_file_win32 (void)
{
    return (fclose (lockfl));
}
#endif

gint
gbilling_lock_file (const gchar *filename)
{
    g_return_val_if_fail (filename != NULL, -1);
#ifndef _WIN32
    return lock_file_unix (filename);
#else
    return lock_file_win32 (filename);
#endif
}

gint
gbilling_unlock_file (void)
{
#ifndef _WIN32
    return unlock_file_unix ();
#else
    return unlock_file_win32 ();
#endif
}

/**
 * gbilling_create_dialog:
 * @type = Tipe dialog.
 * @parent = Parent window (transient).
 * @head = Teks sekunder (secondary).
 * @msg = Format pesan dialog seperti printf().
 * Return: Response id dialog (GtkResponseType) > 0, atau 0 jika error.
 *
 * Buat dialog.
 */
gint
gbilling_create_dialog (GtkMessageType  type,
                        gpointer        parent,
                        const gchar    *head,
                        const gchar    *msg,
                        ...)
{
    g_return_val_if_fail (type >= GTK_MESSAGE_INFO && type <= GTK_MESSAGE_OTHER, 0);
    g_return_val_if_fail (msg != NULL, 0);

    static const gchar *title[] =
    {
        "Informasi",
        "Peringatan",
        "Konfirmasi",
        "Error",
        "gBilling"
    };
    GtkWidget *dialog;
    gchar *tmp, *markup = NULL;
    gint ret;
    va_list args;

    va_start (args, msg);
    tmp = g_strdup_vprintf (msg, args);
    va_end (args);
    dialog = gtk_message_dialog_new ((GtkWindow *) parent, GTK_DIALOG_MODAL,
                    type, GTK_BUTTONS_NONE, head ? head : tmp);
    if (head)
    {
        markup = g_markup_printf_escaped (tmp);
        gtk_message_dialog_format_secondary_markup ((GtkMessageDialog *) dialog, markup);
    }
    switch (type)
    {
        case GTK_MESSAGE_QUESTION:
            gtk_dialog_add_buttons ((GtkDialog *) dialog,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
            break;
        default:
            if (type == GTK_MESSAGE_OTHER)
            {
                GtkWidget *img = gtk_image_new_from_stock ("gtk-dialog-info",
                                            GTK_ICON_SIZE_DIALOG);
                if (img != NULL)
                {
                    gtk_message_dialog_set_image ((GtkMessageDialog *) dialog, img);
                    gtk_widget_show (img);
                }
            }
            gtk_dialog_add_button ((GtkDialog *) dialog, GTK_STOCK_OK, GTK_RESPONSE_OK);
    }
    gtk_window_set_title ((GtkWindow *) dialog, title[type]);
    gtk_window_set_destroy_with_parent ((GtkWindow *) dialog, TRUE);
    g_free (tmp);
    if (parent == NULL)
    {
#ifdef PACKAGE_PIXMAPS_DIR
        tmp = g_build_filename (PACKAGE_PIXMAPS_DIR, "gbilling.png", NULL);
#else
        tmp = g_build_filename ("share", "pixmaps", "gbilling.png", NULL);
#endif
        gtk_window_set_icon_from_file ((GtkWindow *) dialog, tmp, NULL);
        gtk_window_set_position ((GtkWindow *) dialog, GTK_WIN_POS_CENTER);
        g_free (tmp);
    }
    if ((ret = gtk_dialog_run ((GtkDialog *) dialog)) != GTK_RESPONSE_NONE)
        gtk_widget_destroy (dialog);

    g_free (markup);

    return ret;
}

/**
 * wrapper utk #GtkFileChooserDialog
 * params: @title = window title, @parent = parent window,
 * @action = GtkFileChooserAction
 * return: #GtkFileChooserDialog, NULL jika gagal
 */
GtkWidget*
gbilling_file_dialog (const gchar          *title,
                      gpointer              parent,
                      GtkFileChooserAction  action)
{
    g_return_val_if_fail (action >= GTK_FILE_CHOOSER_ACTION_OPEN &&
                          action <= GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, NULL);

    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new (title, (GtkWindow *) parent,
                                          action, "_Tutup",
                                          GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
                                          GTK_RESPONSE_OK, NULL);
    gtk_window_set_destroy_with_parent ((GtkWindow *) dialog, TRUE);

    return dialog;
}

/**
 * eksekusi command di shell windows
 *
 * taken from glade-utils.c by the Glade Project (http://glade.gnome.org)
 * this is a modified version (ajhwb)
 *
 * param: @url = url website
 * return: TRUE jike berhasil, FALSE gagal
 */
#ifdef _WIN32

static gboolean
url_show_win32 (const gchar *url)
{
    HINSTANCE h;

    h = ShellExecuteA (NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    if ((gint)h <= 32)
        return FALSE;

    return TRUE;
}

#else

/**
 * spawning, cari browser di linux
 *
 * taken from glade-utils.c by the Glade Project (http://glade.gnome.org)
 * this is a modified version (ajhwb)
 *
 * param: @url = url website
 * return: TRUE jike berhasil, FALSE gagal
 *
 * Dipakai oleh GTK+ versi yang tidak mendukung hook URI otomatis,
 * setelah gBilling versi 0.2 fungsi ini akan ditinggalkan
 */

static gboolean
url_show_unix (const gchar *url)
{
    static struct
    {
        const gchar *prg, *arg1, *prefix, *postfix;
        gboolean asyncronous;
        volatile gboolean disabled;
    } browsers[] =
    {
        { "gnome-open",             NULL,           "", "", 0 },
        { "exo-open",               NULL,           "", "", 0 },
        { "kfmclient",              "openURL",      "", "", 0 },
        { "gnome-moz-remote",       "--newwin",     "", "", 0 },
        { "x-www-browser",          NULL,           "", "", 1 },
        { "firefox",                NULL,           "", "", 1 },
        { "mozilla-firefox",        NULL,           "", "", 1 },
        { "mozilla",                NULL,           "", "", 1 },
        { "konqueror",              NULL,           "", "", 1 },
        { "opera",                  "-newwindow",   "", "", 1 },
        { "epiphany",               NULL,           "", "", 1 },
        { "galeon",                 NULL,           "", "", 1 },
        { "amaya",                  NULL,           "", "", 1 },
        { "dillo",                  NULL,           "", "", 1 },

    };

    guint i;
    for (i = 0; i < G_N_ELEMENTS (browsers); i++)
        if (!browsers[i].disabled)
        {
            gchar *args[128] = { 0, };
            guint n = 0;
            gchar *string;
            gchar fallback_error[64] = "Ok";
            gboolean success;

            args[n++] = (gchar*) browsers[i].prg;
            if (browsers[i].arg1)
                args[n++] = (gchar*) browsers[i].arg1;

            string = g_strconcat (browsers[i].prefix, url, browsers[i].postfix, NULL);
            args[n] = string;

            if (!browsers[i].asyncronous)
            {
                gint exit_status = -1;
                success = g_spawn_sync (NULL,
                                        args,
                                        NULL,
                                        G_SPAWN_SEARCH_PATH,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &exit_status,
                                        NULL);
                 success = success && !exit_status;
                 if (exit_status)
                    g_snprintf (fallback_error, sizeof (fallback_error),
                                "exitcode: %u", exit_status);
            }
            else
            {
                success = g_spawn_async (NULL,
                                         args,
                                         NULL,
                                         G_SPAWN_SEARCH_PATH,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);
            }
            g_free (string);
            if (success)
                return TRUE;
            browsers[i].disabled = TRUE;
        }

        for (i = 0; i < G_N_ELEMENTS (browsers); i++)
            browsers[i].disabled = FALSE;

        return FALSE;
}

#endif

gboolean
gbilling_url_show (const gchar *url)
{
    g_return_val_if_fail (url != NULL, FALSE);

#ifdef _WIN32
    return url_show_win32 (url);
#else
    return url_show_unix (url);
#endif
}


#ifdef _WIN32
/**
 * control_client_win32:
 * @mode: Mode control client
 * Returns: TRUE jika berhasil, FALSE jika gagal.
 *
 * Kontrol client di Win32.
 */
static gboolean
control_client_win32 (GbillingControl mode)
{
    HANDLE token;
    TOKEN_PRIVILEGES tp;

    OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES |
                      TOKEN_QUERY, &token);
    LookupPrivilegeValue (NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges (token, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)
                           NULL, 0);
    if (GetLastError() != ERROR_SUCCESS)
        return FALSE;
    switch (mode)
    {
        case GBILLING_CONTROL_LOGOFF:
          ExitWindowsEx (EWX_LOGOFF | EWX_FORCEIFHUNG, 0);
          break;
        case GBILLING_CONTROL_RESTART:
          ExitWindowsEx (EWX_REBOOT | EWX_FORCEIFHUNG, 0);
          break;
        case GBILLING_CONTROL_SHUTDOWN:
          ExitWindowsEx (EWX_POWEROFF | EWX_FORCEIFHUNG, 0);
          break;
        default:
          return FALSE;
    }

    return TRUE;
}

#else

/**
 * control_client_unix:
 * @mode: Mode kontrol client.
 *
 * Returns: TRUE jika shutdown berhasil dijalankan, FALSE jika gagal.
 * Shutdown client di UNIX/Linux dengan spawning program 'shutdown' sistem.
 * %GBILLING_CONTROL_LOGOFF tidak didukung di sini.
 */
static gboolean
control_client_unix (GbillingControl mode)
{
    g_return_val_if_fail (mode >= GBILLING_CONTROL_LOGOFF &&
                          mode <= GBILLING_CONTROL_SHUTDOWN, FALSE);
    if (mode == GBILLING_CONTROL_LOGOFF)
        return FALSE;

    GError *error = NULL;
    gchar *cmd, **argv;
    gboolean status;
    gint flags, retval;

    if (mode == GBILLING_CONTROL_RESTART)
        cmd = g_strdup_printf ("/sbin/shutdown -r now");
    else
        cmd = g_strdup_printf ("/sbin/shutdown -h now");

    status = g_shell_parse_argv (cmd, &flags, &argv, &error);
    g_free (cmd);
    if (!status)
    {
err:
        gbilling_debug ("%s: %s\n", __func__, error->message);
        g_error_free (error);
        return FALSE;
    }

    flags = G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL;
    status = g_spawn_sync ("/sbin",
                           argv,
                           NULL,
                           flags,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           &retval,
                           &error );
    g_strfreev (argv);
    if (!status)
        goto err;
    return (retval == 0) ? TRUE : FALSE;
}

#endif /* _WIN32 */

gboolean
gbilling_control_client (GbillingControl mode)
{
#ifdef _WIN32
    return control_client_win32 (mode);
#else
    return control_client_unix (mode);
#endif
}

/**
 * gbilling_calculate_cost:
 * @dur: Durasi pemakaian.
 * @cost: Tarif.
 *
 * Hitung tarif pemakaian berdasarkan tarif.
 *
 * Returns: Tarif pemakaian.
 */
gulong
gbilling_calculate_cost (gulong              dur,
                         const GbillingCost *cost)
{
    g_return_val_if_fail (cost != NULL && cost->imin > 0, 0L);
    gulong ret, dif;

    ret = cost->fcost;
    if (dur >= cost->fmin * 60)
    {
        dur -= cost->fmin * 60;
        dif = (gulong) dur / (cost->imin * 60);
        ret += ++dif * cost->icost;
    }
    return ret;
}

const GbillingCost*
gbilling_default_cost (GbillingCostList *list)
{
    g_return_val_if_fail (list != NULL, NULL);
    
    GbillingCostList *ptr;
    GbillingCost *cost = NULL;
    
    for (ptr = list; ptr; ptr = ptr->next)
    {
        cost = ptr->data;
        if (cost->def)
            break;
    }
    return cost;
}

/**
 * Wrapper g_usleep().
 * @sec = Detik untuk sleep.
 *
 * g_usleep() di Windows memakai fungsi WinAPI Sleep(), dan di UNIX untuk
 * #HAVE_NSLEEP memakai system-call nanosleep(), akan dilanjutkan jika ada
 * interrupt dari signal #EINTR.
 */
void
gbilling_sleep (gint sec)
{
    g_usleep (sec * G_USEC_PER_SEC);
}

/* wrapper kill proses
 * param: @pid = process id atau #HANDLE di win32
 * return: 0 jika berhasil, -1 jika gagal
 */
gint
gbilling_kill (GPid pid)
{
    gint ret;

#ifndef _WIN32
    ret = kill (pid, SIGKILL);
#else
    ret = TerminateProcess (pid, 0) ? 0 : -1;
    errno = GetLastError ();
#endif

    return ret;
}

GtkTreeIter*
gbilling_find_iter (GtkTreeModel *model, const gchar *str, gint col)
{
    g_return_val_if_fail(model != NULL, NULL);
    g_return_val_if_fail(str != NULL, NULL);

    GtkTreeIter *ret = NULL;
    gchar *tmp;
    gboolean valid = gtk_tree_model_get_iter_first (model, ret);

    while (valid)
    {
        gtk_tree_model_get (model, ret, col, &tmp, -1);
        if (!g_utf8_collate (str, tmp))
        {
            g_free (tmp);
            return ret;
        }
        g_free (tmp);
        valid = gtk_tree_model_iter_next (model, ret);
    }

    return NULL;
}

/**
 * time_to_string:
 * time: Jumlah detik seperti time_t.
 *
 * Returns: Format hh:mm:ss dalam string.
 *
 * Konversi jumlah detik ke format string, dealokasi dengan g_free.
 */
gchar*
time_to_string (glong time)
{
    glong secs, min, hour;

    secs = time % 60;
    min = time / 60;
    hour = min / 60;
    min %= 60;

    return (g_strdup_printf ("%.2li:%.2li:%.2li", hour, min, secs));
}

/**
 * time_t ke n detik, time_to_rock later ;p
 * param: @t #time_t
 * return: jumlah detik
 */
glong
time_t_to_sec (time_t t)
{
    struct tm *st = localtime (&t);

    return (st->tm_hour * 3600 + st->tm_min * 60 + st->tm_sec);
}

/**
 * #time_t ke string hh:mm:ss
 * params: @t = #time_t
 * return: string waktu (hh:mm:ss)
 * free dgn g_free()
 */
gchar*
time_t_to_string (time_t t)
{
    struct tm *st = localtime (&t);

    return (g_strdup_printf ("%.2i:%.2i:%.2i", st->tm_hour, st->tm_min, st->tm_sec));
}

/**
 * string hh:mm:ss ke detik
 * param: @string = string waktu (hh:mm:ss)
 * return: jumlah detik (time_t) utk waktu tsb.
 */
glong
string_to_sec (const gchar *string)
{
    g_return_val_if_fail(string != NULL, 0);
    gchar **set = g_strsplit (string, ":", 0);
    g_return_val_if_fail(set != NULL, 0);
    if (g_strv_length (set) != 3)
    {
        g_strfreev (set);
        return 0;
    }
    gint h = atoi (set[0]);
    gint m = atoi (set[1]);
    gint s = atoi (set[2]);
    if (h == 0) /* cek hh, mis. 03, atoi ("03") = 0 */
    {
        h = atoi (++set[0]);
        set[0]--; /* kembalikan address skg juga, atau... invalid ;p */
    }
    if (m == 0)
    {
        m = atoi (++set[1]);
        set[1]--;
    }
    if (s == 0)
    {
        s = atoi (++set[2]);
        set[2]--;
    }
    g_strfreev (set);

    return (h * 3600 + m * 60 + s);
}

/**
 * time_t ke dd-mm-yyyy
 * param: @t = #time_t
 * return: string tanggal
 * free dgn g_free()
 */
gchar*
time_t_to_date (time_t t)
{
    struct tm *st = localtime (&t);

    return (g_strdup_printf ("%.2i-%.2i-%i", st->tm_mday, st->tm_mon + 1, st->tm_year + 1900));
}

/**
 * dd-mm-yyyy ke time_t
 * params: @date = tanggal (string)
 * return: #time_t utk tanggal tsb.
 */
time_t
date_to_time_t (const gchar *date)
{
    g_return_val_if_fail(date != NULL, 0);
    gchar **set = g_strsplit (date, "-", 0);
    g_return_val_if_fail(set != NULL, 0);
    if (g_strv_length (set) != 3)
    {
        g_strfreev (set);
        return 0;
    }

    gint d = atoi (set[0]);
    gint m = atoi (set[1]);
    gint y = atoi (set[2]);
    struct tm st;

    if (d == 0)
    {
        d = atoi (++set[0]);
        set[0]--;
    }
    if (m == 0)
    {
        m = atoi (++set[1]);
        set[1]--;
    }
    /* y tidak akan pernah mulai dengan 0 */
    g_strfreev (set);
    memset (&st, 0, sizeof(st));
    st.tm_mday = d;
    st.tm_mon = m - 1;
    st.tm_year = y - 1900;

    return (mktime (&st));
}

/**
 * tambah .00 rupiah
 * param: @cost = tarif
 * return: string tarif
 * free dgn g_free()
 */
gchar*
cost_to_string (glong cost)
{
    return (g_strdup_printf ("%li", cost));
}

/*
 * Subtitusi font di X11/UNIX dan Windows.
 */
void
set_markup (GtkLabel          *label,
            GbillingFontSize   size,
            gboolean           bold,
            const gchar       *text)
{
    g_return_if_fail (size >= GBILLING_FONT_SMALL && size <= GBILLING_FONT_XX_LARGE);

#ifdef _WIN32
    const gchar *s[5] = { "small", "medium", "large",  "x-large", "xx-large" };
#else
    const gchar *s[5] = { "small", "medium", "medium", "large",   "x-large"  };
#endif

    gchar *tmp = NULL;
    if (bold)
        tmp = g_markup_printf_escaped ("<span size=\"%s\"><b>%s</b></span>", s[size], text);
    else
        tmp = g_markup_printf_escaped ("<span size=\"%s\">%s</span>", s[size], text);
    gtk_label_set_markup (label, tmp);
    g_free (tmp);
}


static gboolean
window_about_delete_event_cb (GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data)
{
    gtk_widget_destroy (((GbillingAboutWindow*) data)->window);
    g_free (data);

    return FALSE;
}

static void
window_about_button_ok_clicked_cb (GtkWidget *widget,
                                   gpointer   data)
{
    window_about_delete_event_cb (NULL, NULL, data);
}

static void
button_copy_toggled_cb (GtkToggleButton *toggleb,
                        gpointer         data)
{
    GbillingAboutWindow *window = data;
    GdkPixdata pixdata;
    GdkPixbuf *pixbuf;
    gboolean des, state;
    GError *err = NULL;
    gchar *tmp;

    state = gtk_toggle_button_get_active (toggleb);
    if (!state)
    {
#ifdef PACKAGE_DATA_DIR
        tmp = g_build_filename (PACKAGE_DATA_DIR, "splash.png", NULL);
#else
        tmp = g_build_filename ("share", "data", "splash.png", NULL);
#endif
        gtk_image_set_from_file ((GtkImage *) window->img_dev, tmp);
        g_free (tmp);
        return;
    }
    des = gdk_pixdata_deserialize (&pixdata, 0x1b726, dev, &err);
    if (!des)
    {
        gbilling_debug ("%s: %s\n", __func__, err->message);
        g_error_free (err);
        return;
    }
    pixbuf = gdk_pixbuf_from_pixdata (&pixdata, TRUE, NULL);
    g_return_if_fail (pixbuf != NULL);
    gtk_image_set_from_pixbuf ((GtkImage *) window->img_dev, pixbuf);
    g_object_unref (pixbuf);
}

void
gbilling_show_about_window (GtkWindow *parent)
{
    GbillingAboutWindow *window_about;
    GtkBuilder *builder;
    gchar *ui;
    guint ret;
    
    GError *error = NULL;

#define get_widget(name) ((GtkWidget *) gtk_builder_get_object (builder, name))


#ifdef PACKAGE_DATA_DIR
    ui = g_build_filename (PACKAGE_DATA_DIR, "ui", "about.ui", NULL);
#else
    ui = g_build_filename ("share", "ui", "about.ui", NULL);
#endif

    builder = gtk_builder_new ();
    ret = gtk_builder_add_from_file (builder, ui, &error);
    g_free (ui);
    if (ret == 0) 
    {
        gbilling_debug ("%s: %s\n", __func__, error->message);
        g_object_unref (builder);
        g_free (ui);
        return;
    }

    struct info
    {
        const gchar *name;
        const gchar *email;
        const gchar *work;
    };

    static struct info dev[] =
    {
        { "Ardhan Madras", "ajhwb@knac.com", "Network & system programmer" },
    };

#if 0
    static struct info contrib[] =
    {
        { "Iman Hermawan", "imanhermawan@yahoo.com", "Linux warnet support, dokumentasi" },
    };
#endif

    const gchar license[] =
    (
        "gBilling is free software; you can redistribute it and/or modify it under "
        "the terms of the GNU General Public License as published by the Free "
        "Software Foundation; version 2 of the License.\n\n"

        "gBilling is distributed in the hope that it will be useful, but WITHOUT "
        "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
        "FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for "
        "more details.\n\n"

        "You should have received a copy of the GNU General Public License along "
        "with gBilling; if not, write to the Free Software Foundation, Inc., 51 "
        "Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA."
    );

    static gchar *thanks[] =
    {
        "Anty & Randi (Untuk dukungannya, I'd die without you!)\n",
        "Firmansyah a.k.a. leonard_gimsong (SQL Ninja)\n",
        "Billy Montana (Too much smoke will kill you!)\n",
        "Mr. Shidiq (For the electric shock that make the first version)\n",
        "Mr. Barman\n",
        "Udin AC/DC\n",
        "Vera\n",
        "Mina (For the PowerPC MacBook)\n",
        "Jemy & Iyut (For save my days)\n",
        "Fandy Blues\n",
        "Febby\n",
        "My friend at DB\n",
        "My PRS Singlecut Guitar\n",
        "\n... and these following rock band that most accompany when coding: "
        "AC/DC, Alterbridge, Motley Crue, Edane.",
    };

    const gchar copy[] = "Copyright\xc2\xa9 2008 - 2011 Tim Developer gBilling";
    gchar *tmp;
    gint i;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    GtkTextTag *bold_tag, *email_tag, *head_tag;

    window_about = g_new (GbillingAboutWindow, 1);

    window_about->window = get_widget ("window");
    gtk_window_set_title ((GtkWindow *) window_about->window, "Tentang gBilling");
    gtk_window_set_transient_for ((GtkWindow *) window_about->window, parent);
    g_signal_connect ((GObject *) window_about->window, "delete-event", 
                      (GCallback) window_about_delete_event_cb, window_about);

    window_about->img_dev = get_widget ("img_dev");
#ifdef PACKAGE_DATA_DIR
    tmp = g_build_filename (PACKAGE_DATA_DIR, "splash.png", NULL);
#else
    tmp = g_build_filename ("share", "data", "splash.png", NULL);
#endif
    gtk_image_set_from_file ((GtkImage *) window_about->img_dev, tmp);
    g_free (tmp);

    window_about->img_gtk = get_widget ("img_gtk");
#ifdef PACKAGE_DATA_DIR
    tmp = g_build_filename (PACKAGE_DATA_DIR, "gtk.png", NULL);
#else
    tmp = g_build_filename ("share", "data", "gtk.png", NULL);
#endif
    gtk_image_set_from_file ((GtkImage *) window_about->img_gtk, tmp);
    g_free (tmp);

    window_about->img_sqlite = get_widget ("img_sqlite");
#ifdef PACKAGE_DATA_DIR
    tmp = g_build_filename (PACKAGE_DATA_DIR, "sqlite.png", NULL);
#else
    tmp = g_build_filename ("share", "data", "sqlite.png", NULL);
#endif
    gtk_image_set_from_file ((GtkImage *) window_about->img_sqlite, tmp);
    g_free (tmp);

    window_about->label_name = get_widget ("label_name");
    tmp = g_strdup_printf ("%s %s", PROGRAM_NAME, PROGRAM_VERSION);
    set_markup ((GtkLabel *) window_about->label_name, GBILLING_FONT_X_LARGE, TRUE, tmp);
    g_free (tmp);

    window_about->label_build = get_widget ("label_build");

#if defined(SQLITE_SUPPORT) && defined(GSTREAMER_SUPPORT)
    tmp = g_strdup_printf ("Build on %s, compiler GCC %s\nLibs GTK+ "
                  "%i.%.i.%i GLib %i.%i.%i GStreamer %i.%i.%i "
                  "SQLite %s", __DATE__, __VERSION__,
                  GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
                  GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION,
                  GST_VERSION_MAJOR, GST_VERSION_MINOR, GST_VERSION_MICRO,
                  SQLITE_VERSION);
#elif defined(SQLITE_SUPPORT)
    tmp = g_strdup_printf ("Build on %s, compiler GCC %s\nLibs GTK+ "
                  "%i.%.i.%i GLib %i.%i.%i SQLite %s", __DATE__, __VERSION__,
                  GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
                  GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION,
                  SQLITE_VERSION);
#else
    tmp = g_strdup_printf ("Build on %s, compiler GCC %s\nLibs GTK+ "
                  "%i.%.i.%i GLib %i.%i.%i", __DATE__, __VERSION__,
                  GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
                  GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
#endif

#ifdef _WIN32
    set_markup ((GtkLabel *) window_about->label_build, GBILLING_FONT_MEDIUM, FALSE, tmp);
#else
    set_markup ((GtkLabel *) window_about->label_build, GBILLING_FONT_SMALL, FALSE, tmp);
#endif
    g_free (tmp);

    window_about->label_desc = get_widget ("label_desc");
    set_markup ((GtkLabel *) window_about->label_desc, 
                GBILLING_FONT_LARGE, TRUE, GBILLING_DESC);

    window_about->label_power = get_widget ("label_power");
    set_markup ((GtkLabel *) window_about->label_power, 
                GBILLING_FONT_LARGE, TRUE, "Powered by");

    window_about->button_copy = get_widget ("button_copy");
    gtk_button_set_label ((GtkButton *) window_about->button_copy, copy);
    g_signal_connect ((GObject *) window_about->button_copy, "toggled", 
                      (GCallback) button_copy_toggled_cb, window_about);

    window_about->linkb_site = get_widget ("linkb_site");
    gtk_button_set_label ((GtkButton *) window_about->linkb_site, GBILLING_SITE);
    gtk_link_button_set_uri ((GtkLinkButton *) window_about->linkb_site, GBILLING_SITE);    

    window_about->textv_dev = get_widget ("textv_dev");
    buffer = gtk_text_view_get_buffer ((GtkTextView *) window_about->textv_dev);
    bold_tag = gtk_text_buffer_create_tag (buffer, NULL, 
                                           "style", PANGO_STYLE_ITALIC,
                                           "weight", PANGO_WEIGHT_BOLD,
                                           NULL);
    head_tag = gtk_text_buffer_create_tag (buffer, NULL,
                                           "font", "Sans 12",
                                           "weight", PANGO_WEIGHT_BOLD,
                                           NULL);
    email_tag = gtk_text_buffer_create_tag (buffer, NULL, 
                                            "foreground", "blue",
                                            "underline", PANGO_UNDERLINE_SINGLE,
                                            NULL);
    gtk_text_buffer_get_start_iter (buffer, &iter);    
    gtk_text_buffer_insert (buffer, &iter, "Untuk adinda tercinta... ", -1);
    gtk_text_buffer_insert_with_tags (buffer, &iter, "Ana Rea Sita Madras", 
                                      -1, bold_tag, NULL);
    gtk_text_buffer_insert (buffer, &iter, " (alm) 13 tahun, yang telah melawan "
                            "leukimia dengan hebat...", -1);
    gtk_text_buffer_insert (buffer, &iter, "\n\n", -1);
    gtk_text_buffer_insert_with_tags (buffer, &iter, "Penulis:", -1, head_tag, NULL);
    gtk_text_buffer_insert (buffer, &iter, "\n", -1);
    for (i = 0; i < G_N_ELEMENTS (dev); i++)
    {
        gtk_text_buffer_insert (buffer, &iter, "   ", -1);
        gtk_text_buffer_insert (buffer, &iter, dev[i].name, -1);
        gtk_text_buffer_insert (buffer, &iter, " <", -1);
        gtk_text_buffer_insert_with_tags (buffer, &iter, dev[i].email, -1, 
                                          email_tag, NULL);
        gtk_text_buffer_insert (buffer, &iter, ">\n", -1);
        gtk_text_buffer_insert (buffer, &iter, "   ", -1);
        gtk_text_buffer_insert (buffer, &iter, dev[i].work, -1);
    }

#if 0
    gtk_text_buffer_insert_with_tags (buffer, &iter, "Kontributor:", -1, head_tag, NULL);
    gtk_text_buffer_insert (buffer, &iter, "\n", -1);
    for (i = 0; i < G_N_ELEMENTS (contrib); i++)
    {
        gtk_text_buffer_insert (buffer, &iter, "   ", -1);
        gtk_text_buffer_insert (buffer, &iter, contrib[i].name, -1);
        gtk_text_buffer_insert (buffer, &iter, " <", -1);
        gtk_text_buffer_insert_with_tags (buffer, &iter, contrib[i].email, -1, 
                                          email_tag, NULL);
        gtk_text_buffer_insert (buffer, &iter, ">\n", -1);
        gtk_text_buffer_insert (buffer, &iter, "   ", -1);
        gtk_text_buffer_insert (buffer, &iter, contrib[i].work, -1);
        if (i + 1 < G_N_ELEMENTS (contrib))
            gtk_text_buffer_insert (buffer, &iter, "\n\n", -1);
    }
#endif

    window_about->textv_license = get_widget ("textv_license");
    buffer = gtk_text_view_get_buffer ((GtkTextView *) window_about->textv_license);
    head_tag = gtk_text_buffer_create_tag (buffer, NULL,
                                           "font", "Sans 12",
                                           "weight", PANGO_WEIGHT_BOLD,
                                           NULL);
    gtk_text_buffer_get_start_iter (buffer, &iter);
    gtk_text_buffer_insert_with_tags (buffer, &iter, "GNU General Public License Version 2",
                                      -1, head_tag, NULL);
    gtk_text_buffer_insert (buffer, &iter, "\n", -1);
    gtk_text_buffer_insert (buffer, &iter, license, -1);

    window_about->textv_thanks = get_widget ("textv_thanks");
    buffer = gtk_text_view_get_buffer ((GtkTextView *) window_about->textv_thanks);
    gtk_text_buffer_get_start_iter (buffer, &iter);
    for (i = 0; i < G_N_ELEMENTS (thanks); i++)
        gtk_text_buffer_insert (buffer, &iter, thanks[i], -1);

    window_about->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_about->button_ok, "clicked", 
                      (GCallback) window_about_button_ok_clicked_cb, window_about);

    g_object_unref (builder);
    gtk_widget_show_all (window_about->window);

#undef get_widget
}

