/* 
** Copyright (c) 1998 [RusNet IRC Network]
**
** File     : rusnet_cmds_ext.h
** Desc.    : rusnet specific commands
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
 */
#ifndef RUSNET_CMDS_H
#define RUSNET_CMDS_H
extern int m_codepage(aClient *cptr _UNUSED_, aClient *sptr, int parc, char *parv[]);
extern int m_force(aClient *cptr, aClient *sptr, int parc, char *parv[]);
extern int m_rmode(aClient *cptr, aClient *sptr, int parc, char *parv[]);
extern int m_svsnick(aClient *cptr, aClient *sptr _UNUSED_, int parc, char *parv[]);
extern int m_svsmode(aClient *cptr, aClient *sptr _UNUSED_, int parc, char *parv[]);
extern int m_rcpage(aClient *cptr, aClient *sptr _UNUSED_, int parc, char *parv[]);
extern int m_nickserv(aClient *cptr, aClient *sptr, int parc, char *parv[]);
extern int m_chanserv(aClient *cptr, aClient *sptr, int parc, char *parv[]);
extern int m_memoserv(aClient *cptr, aClient *sptr, int parc, char *parv[]);
extern int m_operserv(aClient *cptr, aClient *sptr, int parc, char *parv[]);
#endif
