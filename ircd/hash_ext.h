/************************************************************************
 *   IRC - Internet Relay Chat, ircd/hash_ext.h
 *   Copyright (C) 1997 Alain Nissen
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

/*  This file contains external definitions for global variables and functions
    defined in ircd/hash.c.
 */

/*  External definitions for global variables.
 */
#ifndef HASH_C
extern int _HASHSIZE;
extern int _CHANNELHASHSIZE;
extern int _SERVERSIZE;
#endif /* HASH_C */

/*  External definitions for global functions.
 */
#ifndef HASH_C
#define EXTERN extern
#else /* HASH_C */
#define EXTERN
#endif /* HASH_C */
EXTERN void inithashtables();
EXTERN int add_to_client_hash_table(char *name, aClient *cptr);
EXTERN int add_to_channel_hash_table(char *name, aChannel *chptr);
EXTERN int add_to_server_hash_table(aServer *sptr, aClient *cptr);
EXTERN int del_from_client_hash_table(char *name, aClient *cptr);
EXTERN int del_from_channel_hash_table(char *name, aChannel *chptr);
EXTERN int del_from_server_hash_table(aServer *sptr, aClient *cptr);
EXTERN aClient *hash_find_client(char *name, aClient *cptr);
EXTERN aClient *hash_find_server(char *server, aClient *cptr);
EXTERN aChannel *hash_find_channel(char *name, aChannel *chptr);
EXTERN aChannel *hash_find_channels(char *name, aChannel *chptr);
EXTERN aServer *hash_find_stoken(int tok, aClient *cptr, void *dummy);
EXTERN int m_hash(aClient *cptr, aClient *sptr, int parc, char *parv[]);
EXTERN void gen_crc32table(void);
EXTERN unsigned long gen_crc(char *);   /* calculate crc32 value */
EXTERN char * b64enc(unsigned long id);   /* encode with b64e */
EXTERN void add_to_collision_map(const char *nick, const char *collnick,
							unsigned long id);
EXTERN void del_from_collision_map(const char *nick, unsigned long id);
EXTERN char * find_collision(const char *nick, unsigned long id);
EXTERN void expire_collision_map(time_t time);
#ifndef USE_OLD8BIT
EXTERN void transcode_collmaps(conversion_t *old);
#endif
#undef EXTERN
