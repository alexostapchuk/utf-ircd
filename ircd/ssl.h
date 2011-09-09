/************************************************************************
 *   IRC - Internet Relay Chat, src/ssl.h
 *   Copyright (C) RusNet development
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
 */
 
#ifndef SSL_C
#define EXTERN extern
#else /* SSL_C */
#define EXTERN
#endif /* SSL_C */

#ifdef USE_SSL
EXTERN int sslable;
EXTERN int ssl_read(aClient *, void *, int);
EXTERN int ssl_write(aClient *, const void *, int);
EXTERN int ssl_accept(aClient *);
EXTERN int ssl_shutdown(SSL *);
EXTERN int ssl_init(void);
EXTERN void ssl_rehash(void);

#endif /* USE_SSL */
#undef EXTERN
