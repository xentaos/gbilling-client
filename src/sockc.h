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
 *  sockc.h
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net) 
 *  Copyright (C) 2008, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef __SOCKC_H__
#define __SOCKC_H__

#ifdef _WIN32
# include <winsock2.h>
#else
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

int gbilling_accept (int, struct sockaddr*, unsigned int*);
int gbilling_connect (int, const struct sockaddr*, int);
int gbilling_recv (int, void*, int, int);
int gbilling_send (int, const void*, int, int);
int gbilling_socket (int, int, int);
int gbilling_getsockname (int, struct sockaddr*, unsigned int*);
int gbilling_setsockopt (int, int, int, const void*, int);
int gbilling_ioctlsocket (int, unsigned long, unsigned long*);
int gbilling_bind (int, const struct sockaddr*, int);
int gbilling_listen (int, unsigned int);
int gbilling_select (int, fd_set*, fd_set*, fd_set*, struct timeval*);
int gbilling_shutdown (int, int);
int gbilling_close (int s);

#endif /* __SOCKC_H__ */
