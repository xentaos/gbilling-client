#ifndef __CLIENT_H__
#define __CLIENT_H__

#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h>
#endif
#include <time.h>

#include "gbilling.h"
#include "setting.h"

struct _client
{
    gint type;                /* Tipe/mode login client */
    gint idtype;              /* ID prepaid/group */
    gboolean islogin;         /* Client sedang login? */
    gboolean isadmin;         /* Masuk ke dialog admin? */
    time_t start;             /* Waktu mulai client */
    glong duration;           /* Durasi pemakaian client */
    time_t end;               /* Waktu selesai client */
    gulong cost;              /* Tarif pemakaian client */
    gint socket;              /* Socket descriptor */
    gchar *cname;             /* Nama client */
    gchar *username;          /* Username login client */
    guint sid;                /* Source id refresh_timer */
    GbillingAuth cauth;       /* Autentikasi admin client */
    struct sockaddr_in saddr; /* Alamat internet client */
};

extern GbillingClientZet clientzet;
extern struct _client *client;
extern struct _client *last;
extern guint update_serverdatasid;
extern guint update_serverdatasid;

extern GbillingServerInfo server_info;
extern GbillingCost default_cost;

extern GbillingCostList *cost_list;
extern GbillingPrepaidList *prepaid_list;
extern GbillingItemList *item_list;
extern GbillingMemberGroupList *group_list;

gpointer do_timesource (gpointer);
time_t get_time ();
gboolean setup_client (void);
gint cleanup_client (void);
gboolean setup_saddr (void);
gboolean send_command (gint8);
GbillingConnStatus send_login (const GbillingClientZet*);
gboolean get_item (void);
gboolean login_init (GbillingClientZet);
gboolean first_init (gpointer);
gpointer update_data (gpointer);
gboolean refresh_timer (gpointer);
void fill_chat_log (const gchar*, GbillingChatMode);
gboolean do_login (gpointer);
gboolean do_logout (gboolean);
gboolean send_chat (const char*);

#endif /* __CLIENT_H__ */

