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
 *  gbilling.h (functions prototype, definition, data types, and enumeration used in gBilling)
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef __GBILLING_H__
#define __GBILLING_H__

#include <gtk/gtk.h>
#include <glib.h>

#include <time.h>

/* setting default */
#define SERVER_DEFAULT_IP       "192.168.0.1"
#define	SERVER_DEFAULT_PORT     1705
#define MAX_CLIENT              500
#define MAX_IP                  15
#define MAX_PAKET               20
#define MAX_ITEM                20
#define MAX_MEMBER              500
#define MAX_MSG                 128
#define MAX_USERNAME            16
#define MAX_PASSWORD            16

#define MAX_CAFE_LOGOLEN        (1024 * 128)
#define	MAX_CAFE_NAME           32
#define MAX_CAFE_DESC           64
#define MAX_CAFE_ADDR           128

#define	MAX_SERVER_NAME         33
#define MAX_SERVER_DESC         97
#define MAX_SERVER_ADDR         129

#define MAX_PREPAID_NAME        16
#define MAX_ITEM_NAME           32

#define MAX_CLIENT_NAME         16
#define MAX_COST_NAME           16
#define MAX_PAKET_NAME          16
#define MAX_ITEM_NAME           32
#define MAX_GROUP_NAME          12
#define MAX_PASS_SUM            32

#define GBILLING_NAME           "gBilling"
#define GBILLING_VERS           "0.10"
#define GBILLING_CODE           "Rock in 82"
#define GBILLING_DESC           "Free Software Internet Cafe Billing System"
#define GBILLING_SITE           "http://gbilling.sourceforge.net"
#define GBILLING_PORT           "1705"
#define GBILLING_USER           "admin"
#define GBILLING_PASV           "::"
#define GBILLING_BACKUP         "3"
#define GBILLING_PASS           "c80b2d12b5d8e1fa68c3b160d33b5ae5"
#define GBILLING_LOGS           "86400"

#define GBILLING_COST_NAME      "Default"
#define GBILLING_COST_MODE      0
#define GBILLING_COST_TCOST     3000
#define	GBILLING_COST_FCOST     1500
#define	GBILLING_COST_FMIN      30
#define GBILLING_COST_IMIN      10
#define	GBILLING_COST_ICOST     500

typedef enum
{
    GBILLING_STATUS_NONE = 0,   /* Status belum diset (baru aktif) */
    GBILLING_STATUS_MOVE,       /* Client dipindahkan */
    GBILLING_STATUS_LOGIN,      /* Client sedang login */
    GBILLING_STATUS_LOGOUT,     /* Client telah logout */
} GbillingStatus;

typedef enum
{
    GBILLING_COMMAND_NONE = 0,  /* Tidak ada */
    GBILLING_COMMAND_ACCEPT,    /* Terima request client */
    GBILLING_COMMAND_REJECT,    /* Tolak request client */
    GBILLING_COMMAND_ACTIVE,    /* Client aktif */
    GBILLING_COMMAND_LOGIN,     /* Client login */
    GBILLING_COMMAND_RELOGIN,   /* Client login ulang */
    GBILLING_COMMAND_LOGOUT,    /* Client logout */
    GBILLING_COMMAND_CHAT,      /* Command chat */
    GBILLING_COMMAND_REQUEST,   /* Request data */
    GBILLING_COMMAND_UPDATE,    /* Client update */
    GBILLING_COMMAND_LOGOFF,    /* Client logoff */
    GBILLING_COMMAND_RESTART,   /* Client restart */
    GBILLING_COMMAND_SHUTDOWN,  /* Client shutdown */
    GBILLING_COMMAND_ENABLE,    /* Enable client */
    GBILLING_COMMAND_DISABLE,   /* Disable client */
    GBILLING_COMMAND_ITEM,      /* Ambil data item */
    GBILLING_COMMAND_TIME       /* Ambil waktu lokal server */
} GbillingCommand;

typedef enum
{
    GBILLING_CONN_STATUS_ERROR = 0,  /* Gagal komunikasi */
    GBILLING_CONN_STATUS_REJECT,     /* Akses ditolak oleh server */
    GBILLING_CONN_STATUS_SUCCESS     /* Sukses */
} GbillingConnStatus;

typedef enum
{
    GBILLING_LOGIN_TYPE_ADMIN = 0,  /* Mode login admin   */
    GBILLING_LOGIN_TYPE_REGULAR,    /* Mode login reguler */
    GBILLING_LOGIN_TYPE_PREPAID,    /* Mode login paket   */
    GBILLING_LOGIN_TYPE_MEMBER      /* Mode login member  */
} GbillingLoginType;

typedef enum
{
    GBILLING_CONTROL_LOGOFF = 0,    /* Logout client   */
    GBILLING_CONTROL_RESTART,       /* Reboot client   */
    GBILLING_CONTROL_SHUTDOWN       /* Shutdown client */
} GbillingControl;

typedef enum
{
    GBILLING_CHAT_MODE_SERVER = 0,  /* Mode chat server */
    GBILLING_CHAT_MODE_CLIENT       /* Mode chat client */
} GbillingChatMode;

typedef enum
{
    GBILLING_COST_MODE_FMIN = 0,    /* Mode menit pertama */
    GBILLING_COST_MODE_FLAT = 1     /* Mode tarif flat    */
} GbillingCostMode;

typedef enum
{
    GBILLING_UPDATE_MODE_ADD = 0,  /* Mode update dengan menambah */
    GBILLING_UPDATE_MODE_EDIT,     /* Mode update dengan mengedit */
    GBILLING_UPDATE_MODE_DELETE    /* Mode update dengan manghapus */
} GbillingUpdateMode;

typedef enum
{
    DESKTOP_GNOME = 0,  /* Desktop manager GNOME */
    DESKTOP_KDE,        /* Destkop manager KDE */
    DESKTOP_XFCE,       /* Desktop manager XFCE */
} GbillingDesktopMode;

typedef enum
{
    GBILLING_FONT_SMALL = 0,
    GBILLING_FONT_MEDIUM,
    GBILLING_FONT_LARGE,
    GBILLING_FONT_X_LARGE,
    GBILLING_FONT_XX_LARGE,
} GbillingFontSize;

typedef enum
{
    GBILLING_CHECK_MODE_NAME = 0,
    GBILLING_CHECK_MODE_IP
} GbillingCheckMode;

typedef enum
{
    GBILLING_EVENT_SERVER_ACTIVE = 0,
    GBILLING_EVENT_SERVER_NONACTIVE,
    GBILLING_EVENT_CLIENT_ACTIVE,
    GBILLING_EVENT_CLIENT_NONACTIVE,
    GBILLING_EVENT_LOGIN_ADMIN,
    GBILLING_EVENT_LOGIN_REGULAR,
    GBILLING_EVENT_LOGIN_PREPAID,
    GBILLING_EVENT_LOGIN_MEMBER,
    GBILLING_EVENT_LOGOUT,
    GBILLING_EVENT_CHAT_RECEIVE,
    GBILLING_EVENT_CHAT_SEND
} GbillingEvent;

typedef struct
{
    gchar name[33];     /* Nama warnet */
    gchar desc[97];     /* Deskripsi warnet */
    gchar addr[128];    /* Alamat warnet */
} GbillingServerInfo;

typedef struct
{
    gint id;         /* ID client */
    gchar *name;     /* Nama client */
    gchar *ip;       /* IP address client */
    gboolean state;  /* State aktif-nonaktif client */
} GbillingClient;

typedef GList GbillingClientList;

typedef struct
{
    gint8 id;       /* ID tarif */
    gchar name[9];  /* Nama tarif */
    gint8 def;      /* Sebagai tarif default */
    gint8 mode;     /* Mode tarif */
    gint8 fmin;     /* Menit pertama */
    gint8 imin;     /* Interval menit */
    gint32 fcost;   /* Tarif menit pertama */
    gint32 icost;   /* Tarif selanjutnya */
} GbillingCost;

typedef GList GbillingCostList;

typedef struct
{
    gint shour;      /* Jam rotasi mulai */
    gint smin;       /* Menit rotasi mulai */
    gint ehour;      /* Jam rotasi selesai */
    gint emin;       /* Menit rotasi selesai */
    gboolean state;  /* State aktif dan nonaktif */
    gchar cost[9];   /* Nama tarif yang digunakan */
} GbillingCostRotation;

typedef GList GbillingCostRotationList;

typedef struct
{
    gint16 id;        /* ID prepaid */
    gchar name[9];    /* Nama prepaid */
    gint32 duration;  /* Durasi prepaid (detik) */
    gint32 cost;      /* Tarif prepaid */
    gint16 active;    /* Status aktif-nonaktif prepaid */
} GbillingPrepaid;

typedef GList GbillingPrepaidList;

typedef struct
{
    gint16 id;       /* ID item */
    gchar name[17];  /* Nama item */
    gint32 cost;     /* Harga item */
    gint8 active;    /* Status aktif-nonaktif item */
} GbillingItem;

typedef GList GbillingItemList;

typedef struct
{
    gint8 id;       /* ID group */
    gchar name[9];  /* Nama group */
    gchar cost[9];  /* Tarif yang digunakan */
} GbillingMemberGroup;

typedef GList GbillingMemberGroupList;

typedef struct
{
    gint      id;        /* ID member */
    glong     reg;       /* Tanggal registrasi */
    gint      status;    /* Status aktif-nonaktif member */
    gchar    *username;  /* Username member */
    gchar    *pass;      /* Password member */
    gchar    *group;     /* Group */
    gchar    *fullname;  /* Nama lengkap member */
    gchar    *address;   /* Alamat member */
    gchar    *phone;     /* No. telpon member */
    gchar    *email;     /* E-mail member */
    gchar    *idcard;    /* Nomor kartu pengenal member */
} GbillingMember;

typedef GList GbillingMemberList;

/* tipe data autentikasi, username dan password */
typedef struct
{
    gchar username[9];
    gchar passwd[33];
} GbillingAuth;

typedef struct
{
    gchar username[9];
    gchar passwd[33];
} GbillingLogin;

typedef struct
{
    gint16 msg_len;
    gchar *msg;
} GbillingChat;

/* Data login client */
struct _GbillingClientSet
{
    gint8 type;        /* Tipe login             */
    gint8 id;          /* ID prepaid atau member */
    gint32 start;      /* Waktu login            */
    gint32 voucher;    /* Voucher (reserved)     */
    GbillingAuth auth; /* Data autentikasi       */
};

typedef struct _GbillingClientSet GbillingClientSet;
typedef struct _GbillingClientSet GbillingClientZet;

struct _GbillingServerSet
{
    gint8 stat;         /* Status buffer (queue) */
    gint8 id;           /* ID client yang akan dikirimkan (queue) */
    gint8 cmd;          /* Perintah dari server */
    gint8 type;         /* Tipe login client */
    gint8 idtype;       /* ID tipe login client */
    gint32 texpire;     /* Waktu expire buffer (queue) */
    gint32 tstart;      /* Waktu mulai client untuk recovery */
    gchar username[9];  /* Username client */
    gchar msg[129];     /* Message client untuk chat */
};

typedef struct _GbillingServerSet GbillingServerSet;
typedef struct _GbillingServerSet GbillingServerZmd;

typedef struct _GbillingLog
{
    time_t tstart;     /* Waktu mulai */
    time_t tend;       /* Waktu selesai */
    guint32 voucher;   /* Nomor voucher */
    glong ucost;       /* Tarif pemakaian */
    glong acost;       /* Tarif tambahan */
    gchar *client;     /* Nama client */
    gchar *username;   /* Username pemakai */
    gchar *type;       /* Tipe login */
    gchar *desc;       /* Keterangan tambahan */
} GbillingLog;

typedef struct
{
    GtkWidget *window;
    GtkWidget *img_dev;
    GtkWidget *img_gtk;
    GtkWidget *img_sqlite;
    GtkWidget *label_name;
    GtkWidget *label_desc;
    GtkWidget *label_build;
    GtkWidget *label_power;
    GtkWidget *button_copy;
    GtkWidget *button_ok;
    GtkWidget *linkb_site;
    GtkWidget *textv_dev;
    GtkWidget *textv_license;
    GtkWidget *textv_thanks;
} GbillingAboutWindow;


extern const gchar * const gbilling_login_mode[];
extern const gchar * const gbilling_cost_mode[];
extern gboolean use_win32;

#define gbilling_total_cost(fmin, imin, fcost, icost)   ((fcost) + ((60 - (fmin)) / (imin) * (icost)))

#define gbilling_unix_debug(error) (use_win32 ? " " : g_strerror (error))

gchar* gbilling_str_checksum (const gchar*);

gssize gbilling_data_send (gint, gconstpointer, gsize);
gssize gbilling_data_recv (gint, gpointer, gsize);

gboolean gbilling_clientset_send (gint, const GbillingClientZet*);
gboolean gbilling_clientset_recv (gint, GbillingClientZet*);

GbillingServerInfo* gbilling_server_info_new (void);
void gbilling_server_info_free (GbillingServerInfo*);
gboolean gbilling_server_info_send (gint, const GbillingServerInfo*);
gboolean gbilling_server_info_recv (gint, GbillingServerInfo*);

GbillingClient* gbilling_client_new (void);
void gbilling_client_free (GbillingClient*);
void gbilling_client_list_free (GbillingClientList*);

GbillingCost* gbilling_cost_new (void);
void gbilling_cost_free (GbillingCost*);
GbillingCost* gbilling_cost_get_by_id (guint);
GbillingCost* gbilling_cost_get_by_name (const gchar*);
GbillingCostList* gbilling_cost_get_list (void);
void gbilling_cost_list_free (GbillingCostList*);
gboolean gbilling_cost_list_send (gint, GbillingCostList*);
gboolean gbilling_cost_list_recv (gint, GbillingCostList**);
gint32 gbilling_cost_per_hour (const GbillingCost*);

gboolean gbilling_cost_send (gint, const GbillingCost*);
gboolean gbilling_cost_recv (gint, GbillingCost*);

GbillingPrepaid* gbilling_prepaid_new (void);
void gbilling_prepaid_free (GbillingPrepaid*);
void gbilling_prepaid_list_free (GbillingPrepaidList*);
gboolean gbilling_prepaid_list_send (gint, GbillingPrepaidList*);
gboolean gbilling_prepaid_list_recv (gint, GbillingPrepaidList**);

GbillingItem* gbilling_item_new (void);
void gbilling_item_free (GbillingItem*);
void gbilling_item_list_free (GbillingItemList*);
gboolean gbilling_item_list_send (gint, GbillingItemList*);
gboolean gbilling_item_list_recv (gint, GbillingItemList**);

GbillingMemberGroup* gbilling_member_group_new (void);
void gbilling_member_group_free (GbillingMemberGroup*);
void gbilling_member_group_list_free (GbillingMemberGroupList*);
gboolean gbilling_member_group_list_send (gint, GbillingMemberGroupList*);
gboolean gbilling_member_group_list_recv (gint, GbillingMemberGroupList**);

GbillingMember* gbilling_member_new (void);
void gbilling_member_free (GbillingMember*);
void gbilling_member_list_free (GbillingMemberList*);

GbillingAuth* gbilling_auth_new (const gchar*, const gchar*);
void gbilling_auth_free (GbillingAuth*);
gboolean gbilling_auth_compare (const GbillingAuth*, const GbillingAuth*);
gboolean gbilling_auth_send (gint, GbillingAuth*);
gboolean gbilling_auth_recv (gint, GbillingAuth*);

GbillingChat* gbilling_chat_new (void);
void gbilling_chat_free (GbillingChat*);

gboolean gbilling_server_cmd_send (gint, GbillingServerZmd*);
gint gbilling_server_cmd_recv (gint, GbillingServerZmd*);

gint gbilling_debug (const gchar*, ...);
gint gbilling_sysdebug0(const gchar*, const gchar*, const gchar*, gint);

#define gbilling_sysdebug(msg) gbilling_sysdebug0 (__FILE__, __func__, msg, __LINE__)

gint gbilling_prepare_dir (void);
gint gbilling_lock_file (const gchar*);
gint gbilling_unlock_file (void);
gint gbilling_create_dialog (GtkMessageType, gpointer, const gchar*, const gchar*, ...);
GtkWidget* gbilling_file_dialog (const gchar*, gpointer, GtkFileChooserAction);
gboolean gbilling_url_show (const gchar*);
gboolean gbilling_control_client (GbillingControl);
gulong gbilling_calculate_cost (gulong, const GbillingCost*);
const GbillingCost* gbilling_default_cost (GbillingCostList*);

GbillingLog* gbilling_log_new (void);
void gbilling_log_free (GbillingLog*);

void gbilling_sleep (gint);
gint gbilling_kill (GPid);
GtkTreeIter* gbilling_find_iter (GtkTreeModel*, const gchar*, gint);
gchar* time_to_string (glong);
glong time_t_to_sec (time_t);
gchar* time_t_to_string (time_t);
glong string_to_sec (const gchar*);
gchar* time_t_to_date (time_t);
time_t date_to_time_t (const gchar*);
gchar* cost_to_string (glong);
void set_markup (GtkLabel*, GbillingFontSize, gboolean, const gchar*);
void gbilling_show_about_window (GtkWindow*);

#endif /* __GBILLING_H__ */

