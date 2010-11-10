/* 
** Copyright (c) 2005 skold [RusNet IRC Network]
**
** File     : iserv.h
** Desc.    : Configuration lines module headers file
**            For further info see the rusnet-doc/RUSNET_DOC file.
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
** $Id: iserv.h,v 1.3 2005/08/27 16:05:24 skold Exp $
 */

#ifndef ISERV_C
# define EXTERN extern
#else /* ISERV_C */
# define EXTERN
#endif /* ISERV_C */

EXTERN void addPendingLine(char *line);

#define SEP IRCDCONF_DELIMETER
#define KILLSCONFTMP KILLSCONF ".tmp"
#define KILLSCONFBAK KILLSCONF ".bak"
#define KILLSCONF KILLSCONF_PATH

#define ISREADBUF 8192
#define ISINBUF 10
#define ISOUTBUF 16384
#define ISCHECK 10

typedef struct  ConfLine isConfLine;

struct	ConfLine {
	u_int	status;		
	char	type;		/* Line type symbol */		
	char	*userhost;
	char	*name;		/* nick information */
	char	*passwd;	/* Line reason */
	char	*port;
	time_t	expire;
};

#define	LINE_REMOVE		0x001
#define	LINE_ADD		0x002
#define LINE_ILLEGAL		0x004
