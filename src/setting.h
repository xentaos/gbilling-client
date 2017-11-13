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
 *  setting.h
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 *
 */

#ifndef __SETTING_H__
#define __SETTING_H__

#include <gtk/gtk.h>

#define	SETTING_FILE              "client.conf"
#define SERVER_GROUP              "Server"
#define SERVER_IP_KEY             "ServerIP"
#define SERVER_PORT_KEY           "ServerPort"
#define DISPLAY_GROUP             "Display"
#define DISPLAY_USEWALLPAPER_KEY  "DisplayUseWallpaper"
#define DISPLAY_WALLPAPER_KEY     "DisplayWallpaper"
#define DISPLAY_BGCOLOR_KEY       "DisplayBgColor"
#define SECURITY_GROUP            "Security"
#define SECURITY_USERNAME         "Username"
#define SECURITY_PASSWORD         "Password"

GKeyFile *key_file;

gint read_key_file (void);
gint write_key_file (void);
gint create_key_file (void);

#endif /* __SETTING_H__ */
