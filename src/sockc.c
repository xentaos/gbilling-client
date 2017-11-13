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
 *  sockc.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Inspired by libc_interface.c from Pidgin Project (http://www.pidgin.im)
 *  Copyright (C) 2008, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/select.h>
# include <sys/time.h>
# include <fcntl.h>
# include <unistd.h>
#endif

#include <errno.h>

int
gbilling_accept (int              s, 
                 struct sockaddr *addr, 
                 unsigned int    *n)
{
    int ret;

    ret = accept (s, addr, n);
#ifdef _WIN32
    if (ret == INVALID_SOCKET)
    {
        errno = WSAGetLastError ();
        if (errno == WSAEWOULDBLOCK || errno == WSAEINPROGRESS)
            errno = EAGAIN;
        return -1;
    }
#endif
    return ret;
}

int
gbilling_connect (int                    s, 
                  const struct sockaddr *addr, 
                  int                    n)
{
    int ret;

    ret = connect (s, addr, n);
#ifdef _WIN32
    if (ret == SOCKET_ERROR) 
    {
        errno = WSAGetLastError ();
        if (errno == WSAEWOULDBLOCK)
            errno = EAGAIN;
        return -1;
    }
#endif
    return ret;
}

int
gbilling_recv (int   s, 
               void *buffer, 
               int   n, 
               int   flag) 
{
    int ret;

    ret = recv (s, buffer, n, flag);
#ifdef _WIN32
    if (ret == SOCKET_ERROR)
    {
        errno = WSAGetLastError ();
        if (errno == WSAEWOULDBLOCK || errno == WSAEINPROGRESS)
            errno = EAGAIN;
        return -1;
    }
#endif
    return ret;
}

int
gbilling_send (int         s, 
               const void *buffer, 
               int         n, 
               int         flag)
{
    int ret;

    ret = send (s, buffer, n, flag);
#ifdef _WIN32
    if (ret == SOCKET_ERROR) 
    {
        errno = WSAGetLastError ();
        if (errno == WSAEWOULDBLOCK || errno == WSAEINPROGRESS) 
            errno = EAGAIN;
        return -1;
    }
#endif
    return ret;
}

int
gbilling_socket (int name, 
                 int type, 
                 int family) 
{
    int ret;

    ret = socket (name, type, family);
#ifdef _WIN32
    if (ret == INVALID_SOCKET)
    {
        errno == WSAGetLastError ();
        return -1;
    }
#endif
    return ret;
}

int
gbilling_getsockname (int              s, 
                      struct sockaddr *addr, 
                      unsigned int    *addrlen) 
{
    int ret;
    
    ret = getsockname (s, addr, addrlen);
#ifdef _WIN32
    if (ret == SOCKET_ERROR)
    {
        errno = WSAGetLastError ();
        return -1;
    }
#endif
    
    return ret;
}

int
gbilling_setsockopt (int         s, 
                     int         level, 
                     int         optname, 
                     const void *optval, 
                     int         optlen)
{
    int ret;
        
    ret = setsockopt (s, level, optname, optval, optlen);
#ifdef _WIN32
    if (ret == SOCKET_ERROR)
    {
        errno == WSAGetLastError ();
        return -1;
    }
#endif
    return ret;
}

#ifdef _WIN32
int
gbilling_ioctlsocket (int            s, 
                      unsigned long  cmd, 
                      unsigned long *mode) 
{
    int ret;
        
    ret = ioctlsocket (s, cmd, mode);
    if (ret == SOCKET_ERROR)
    {
        errno == WSAGetLastError ();
        return -1;
    }

    return 0;
}
#endif

int
gbilling_bind (int                    s, 
               const struct sockaddr *addr, 
               int                    len) 
{
    int ret;
    
    ret = bind (s, addr, len);
#ifdef _WIN32
    if (ret == SOCKET_ERROR)
    {
       errno = WSAGetLastError ();
       return -1;
    }
#endif

    return ret;
}

int
gbilling_listen (int          s, 
                 unsigned int n) 
{
    int ret;

    ret = listen (s, n);
#ifdef _WIN32
    if (ret == SOCKET_ERROR)
    {
        errno = WSAGetLastError ();
        return -1;
    }
#endif

    return ret;
}

int
gbilling_select (int             s, 
                 fd_set         *r, 
                 fd_set         *w, 
                 fd_set         *e, 
                 struct timeval *t) 
{
    int ret;

    ret = select (s, r, w, e, t);
#ifdef _WIN32
    if (ret == SOCKET_ERROR)
    {
        errno == WSAGetLastError ();
        return -1;
    }
#endif

    return ret;
}

int
gbilling_shutdown (int s, 
                   int dir) 
{
    int ret;
    
    ret = shutdown (s, dir);
#ifdef _WIN32
    if (ret == SOCKET_ERROR)
    {
        errno = WSAGetLastError ();
        return -1;
    }
#endif

    return ret;
}

int
gbilling_close (int s)
{
    int ret;

#ifdef _WIN32
    ret = closesocket (s);
    if (ret == SOCKET_ERROR)
    {
        errno = WSAGetLastError ();
        return -1;
    }
#else
    ret = close (s);
#endif

    return ret;
}
