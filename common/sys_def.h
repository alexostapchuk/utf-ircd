/************************************************************************
 *   IRC - Internet Relay Chat, common/sys_def.h
 *   Copyright (C) 1990 University of Oulu, Computing Center
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *   $Id: sys_def.h,v 1.5 2005/10/02 18:56:30 cj Exp $
 */
 
#if defined(DEBUGMODE) && !defined(CLIENT_COMPILE) && !defined(CHKCONF_COMPILE) && defined(DO_DEBUG_MALLOC)
# define	free(x)		MyFree(x)
#else
# define	MyFree(x)	do { if ((x) != NULL) free(x); (x) = NULL; } while(0)
#endif

#define	SETSOCKOPT(fd, o1, o2, p1, o3)	setsockopt(fd, o1, o2, (char *)p1,\
						   (SOCK_LEN_TYPE) sizeof(o3))
#define	GETSOCKOPT(fd, o1, o2, p1, p2)	getsockopt(fd, o1, o2, (char *)p1,\
						   (SOCK_LEN_TYPE *)p2)
#ifdef INET6
# define __recv__(fd, buf, size) recvfrom(fd, buf, size, 0, 0, 0)
#else /* INET6 */
# define __recv__(fd, buf, size) recv(fd, buf, size, 0)
#endif /* INET6 */
#ifdef INET6
# define __send__(fd, buf, size) sendto(fd, buf, size, 0, 0, 0)
#else /* INET6 */
# define __send__(fd, buf, size) send(fd, buf, size, 0)
#endif /* INET6 */

#ifdef USE_SSL
# define RECV(from, buf, len) ((IsSSL(from) && from->ssl) ?\
                                       ssl_read(from, buf, len) :\
                                       __recv__(from->fd, buf, len))
# define SEND(to, buf, len) ((IsSSL(to) && to->ssl) ?\
                                       ssl_write(to, buf, len) :\
                                       __send__(to->fd, buf, len))
# define SEND1(to, buf, len) ((IsSSL(to) && to->ssl) ?\
                                       ssl_write(to, buf, len) :\
                                       __send__(to->fd, buf, len))
//#define SEND_NOSSL(to, buf, len)   do { if(!IsSSL(to)) { __send__(to, buf, len); } } while(0);
#else /* USE_SSL */
# define RECV(from, buf, len) __recv__(from->fd, buf, len)
# define SEND(from, buf, len) __send__(from->fd, buf, len)
# define SEND1(from, buf, len) __send__(from->fd, buf, len)
//#define SEND_NOSSL __send__
#endif /* USE_SSL */
