/************************************************************************
 *   IRC - Internet Relay Chat, ircd/chkconf.c
 *   Copyright (C) 1993 Darren Reed
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


#include "os.h"
#include "s_defines.h"
#define CHKCONF_C
#include "match_ext.h"
#undef CHKCONF_C

#define mystrdup(x) strdup(x)
#define MyMalloc(x)     malloc(x)
#define MyFree(x)       free(x)

static	char	*configfile = IRCDCONF_PATH;
#include "config_read.c"

#define CK_FILE filelist->file
#define CK_LINE filelist->linenum
static aConfig *findConfLineNumber(int);
static aConfig *files;

static	void	new_class(int);
static	char	*getfield(char *), confchar(u_int);
static	int	openconf(void);
static	void	validate(aConfItem *);
static	aClass	*get_class(int, int);
static	aConfItem	*initconf(void);
static	void	showconf(void);

static	int	numclasses = 0, *classarr = (int *)NULL, debugflag = 0;
static	char	nullfield[] = "";
static	char	maxsendq[12];
#ifndef USE_OLD8BIT
int	UseUnicode = 0;	/* for match() */
#endif

#define	SHOWSTR(x)	((x) ? (x) : "*")

int	main(int argc, char *argv[])
{
	aConfItem *result;
	int	showflag = 0;

	if (argc > 1 && !strncmp(argv[1], "-h", 2))
	{
		(void)printf("Usage: %s [-h | -s | -d[#]] [ircd.conf]\n",
			      argv[0]);
		(void)printf("\t-h\tthis help\n");
		(void)printf("\t-s\tshows preprocessed config file (after "
			"includes and/or M4)\n");
		(void)printf("\t-d[#]\tthe bigger number, the more verbose "
			"chkconf is in its checks\n");
		(void)printf("\tDefault ircd.conf = %s\n", IRCDCONF_PATH);
		exit(0);
	}
	new_class(0);

	if (argc > 1 && !strncmp(argv[1], "-d", 2))
   	{
		debugflag = 1;
		if (argv[1][2])
			debugflag = atoi(argv[1]+2);
		argc--;
		argv++;
	}
	if (argc > 1 && !strncmp(argv[1], "-s", 2))
   	{
		showflag = 1;
		argc--;
		argv++;
	}
	if (argc > 1)
		configfile = argv[1];
	if (showflag)
	{
		showconf();
		return 0;
	}
	/* If I do not use result as temporary return value
	 * I get loops when M4_PREPROC is defined - Babar */
	result = initconf();
	validate(result);
	config_free(files);
	return 0;
}

/*
 * openconf
 *
 * returns -1 on any error or else the fd opened from which to read the
 * configuration file from.  This may either be th4 file direct or one end
 * of a pipe from m4.
 */
static	int	openconf(void)
{
#ifdef	M4_PREPROC
	int	pi[2];

	/* ircd.m4 with full path now! Kratz */
	if (access(IRCDM4_PATH, R_OK) == -1)
	{
		(void)fprintf(stderr, "%s missing.\n", IRCDM4_PATH);
		return -1;
	}
	if (pipe(pi) == -1)
		return -1;
	switch(fork())
	{
	case -1:
		return -1;
	case 0:
		(void)close(pi[0]);
		(void)close(2);
		if (pi[1] != 1)
		    {
			(void)close(1);
			(void)dup2(pi[1], 1);
			(void)close(pi[1]);
		    }
		(void)dup2(1,2);
		/*
		 * m4 maybe anywhere, use execvp to find it.  Any error
		 * goes out with report_error.  Could be dangerous,
		 * two servers running with the same fd's >:-) -avalon
		 */
		(void)execlp(M4_PATH, "m4", IRCDM4_PATH, configfile, 0);
		perror("m4");
		exit(-1);
	default :
		(void)close(pi[1]);
		return pi[0];
	}
#else
	return open(configfile, O_RDONLY);
#endif
}

/* show config as ircd would see it before starting to parse it */
static	void	showconf(void)
{
	aConfig *p, *p2;
	int etclen = 0;
	FILE *fdn;
	int fd;

	fprintf(stderr, "showconf(): %s\n", configfile);
	if ((fd = openconf()) == -1)
	    {
#ifdef	M4_PREPROC
		(void)wait(0);
#endif
		return;
	    }
	if (debugflag)
	{
		etclen = strlen(IRCDCONF_DIR);
	}
	fdn = fdopen(fd, "r");
	p2 = config_read(fdn, 0, new_config_file(configfile, NULL, 0));
	for(p = p2; p; p = p->next)
	{
		if (debugflag)
			printf("%s:%d:", p->file->filename +
				(strncmp(p->file->filename, IRCDCONF_DIR,
				etclen) == 0 ? etclen : 0), p->linenum);
		printf("%s\n", p->line);
	}
	config_free(p2);
	(void)fclose(fdn);
#ifdef	M4_PREPROC
	(void)wait(0);
#endif
	return;
}


/*
** initconf()
**    Read configuration file.
**
**    returns a pointer to the config items chain if successful
**            NULL if config is invalid (some mandatory fields missing)
*/

static	aConfItem 	*initconf(void)
{
	int	fd;
	char	*tmp, *tmp3 = NULL, *s;
	int	ccount = 0, ncount = 0, flags = 0, nr = 0;
	aConfItem *aconf = NULL, *ctop = NULL;
	int	mandatory_found = 0, valid = 1;
	char    *line;
	aConfig *ConfigTop, *filelist;
	aFile	*ftop;
	FILE	*fdn;

	(void)fprintf(stderr, "initconf(): ircd.conf = %s\n", configfile);
	if ((fd = openconf()) == -1)
	    {
#ifdef	M4_PREPROC
		(void)wait(0);
#endif
		return NULL;
	    }

	ftop = new_config_file(configfile, NULL, 0);
	fdn = fdopen(fd, "r");
	files = ConfigTop = config_read(fdn, 0, ftop);
	for(filelist = ConfigTop; filelist; filelist = filelist->next)
	    {
		if (aconf)
		    {
			if (aconf->host && (aconf->host != nullfield))
				(void)free(aconf->host);
			if (aconf->passwd && (aconf->passwd != nullfield))
				(void)free(aconf->passwd);
			if (aconf->name && (aconf->name != nullfield))
				(void)free(aconf->name);
		    }
		else
			aconf = (aConfItem *)malloc(sizeof(*aconf));
		aconf->host = (char *)NULL;
		aconf->passwd = (char *)NULL;
		aconf->name = (char *)NULL;
		aconf->class = (aClass *)NULL;
		/* abusing clients to store ircd.conf line number */
		aconf->clients = ++nr;

		line = filelist->line;
		/*
		 * Do quoting of characters and # detection.
		 */
		for (tmp = line; *tmp; tmp++)
		    {
			if (*tmp == '\\')
			    {
				switch (*(tmp+1))
				{
				case 'n' :
					*tmp = '\n';
					break;
				case 'r' :
					*tmp = '\r';
					break;
				case 't' :
					*tmp = '\t';
					break;
				case '0' :
					*tmp = '\0';
					break;
				default :
					*tmp = *(tmp+1);
					break;
				}
				if (!*(tmp+1))
					break;
				else
					for (s = tmp; (*s = *(s+1)); s++)
						;
				tmp++;
			    }
			else if (*tmp == '#')
				*tmp = '\0';
		    }
		if (!*line || *line == '#' || *line == '\n' ||
		    *line == ' ' || *line == '\t')
			continue;

		if (line[1] != IRCDCONF_DELIMITER)
		    {
			config_error(CF_ERR, CK_FILE, CK_LINE,
				"wrong delimiter in line (%s)", line);
                        continue;
                    }

		if (debugflag)
			config_error(CF_NONE, CK_FILE, CK_LINE, "%s", line);

		tmp = getfield(line);
		if (!tmp)
		    {
			config_error(CF_ERR, CK_FILE, CK_LINE,
				"no field found");
			continue;
		    }

		aconf->status = CONF_ILLEGAL;

		switch (*tmp)
		{
			case 'A': /* Name, e-mail address of administrator */
			case 'a': /* of this server. */
				aconf->status = CONF_ADMIN;
				break;
			case 'B': /* Name of alternate servers */
			case 'b':
				aconf->status = CONF_BOUNCE;
				break;
			case 'C': /* Server where I should try to connect */
			  	  /* in case of lp failures             */
				ccount++;
				aconf->status = CONF_CONNECT_SERVER;
				break;
			case 'c':
				ccount++;
				aconf->status = CONF_ZCONNECT_SERVER;
				break;
			case 'D': /* auto connect restrictions */
			case 'd':
				aconf->status = CONF_DENY;
				break;
			case 'E': /* Exempt from kill user line */
			case 'e':
			        aconf->status = CONF_EXEMPT;
		        	break;
			case 'F': /* virtual interface to fasten to when  */
			case 'f': /* connecting to the server mentioned */
			        aconf->status = CONF_INTERFACE;
		        	break;
			case 'H': /* Hub server line */
			case 'h':
				aconf->status = CONF_HUB;
				break;
			case 'I': /* Just plain normal irc client trying  */
			          /* to connect me */
				aconf->status = CONF_CLIENT;
				break;
			case 'i' : /* Restricted client */
				aconf->status = CONF_RCLIENT;
				break;
			case 'K': /* Kill user line on ircd.conf*/
			case 'k':
				aconf->status = CONF_KILL;
				break;
			/* Operator. Line should contain at least */
			/* password and host where connection is  */
			case 'L': /* guaranteed leaf server */
				aconf->status = CONF_LEAF;
				break;
#ifdef LOG_EVENTS
			case 'l':
				aconf->status = CONF_LOG;
				break;
#endif
			/* Me. Host field is name used for this host */
			/* and port number is the number of the port */
			case 'M':
			case 'm':
				aconf->status = CONF_ME;
				break;
			case 'N': /* Server where I should NOT try to     */
			case 'n': /* connect in case of lp failures     */
				  /* but which tries to connect ME        */
				++ncount;
				aconf->status = CONF_NOCONNECT_SERVER;
				break;
			case 'O':
				aconf->status = CONF_OPERATOR;
				break;
			/* Local Operator, (limited privs --SRB) */
			case 'o':
				aconf->status = CONF_LOCOP;
				break;
#ifdef USE_SSL
			case 'p': /* SSL listen port line */
				aconf->status = CONF_SSL_LISTEN_PORT;
				break;
#endif /* USE_SSL */
			case 'P': /* listen port line */
				aconf->status = CONF_LISTEN_PORT;
				break;
			case 'Q': /* a server that you don't want in your */
			case 'q': /* network. USE WITH CAUTION! */
				aconf->status = CONF_QUARANTINED_SERVER;
				break;
#ifdef RUSNET_RLINES
			case 'R': /* Rusnet R-lines */
				aconf->status = CONF_RUSNETRLINE;
				break;
#endif
			case 'S': /* Service. Same semantics as   */
			case 's': /* CONF_OPERATOR                */
				aconf->status = CONF_SERVICE;
				break;
			case 'V': /* Server link version requirements */
				aconf->status = CONF_VER;
				break;
			case 'Y':
			case 'y':
			        aconf->status = CONF_CLASS;
		        	break;
		    default:
			(void)fprintf(stderr,
				"\tERROR: unknown conf line letter (%c)\n",
				*tmp);
		    }
		if (IsIllegal(aconf))
			continue;

		do
		{
			if ((tmp = getfield(NULL)) == NULL)
				break;
			DupString(aconf->host, tmp);
			if ((tmp = getfield(NULL)) == NULL)
				break;
			DupString(aconf->passwd, tmp);
			if ((tmp = getfield(NULL)) == NULL)
				break;
			DupString(aconf->name, tmp);
			if ((tmp = getfield(NULL)) == NULL)
				break;
			aconf->port = atoi(tmp);
			if ((tmp = getfield(NULL)) == NULL)
				break;
			if (aconf->status & CONF_CLIENT_MASK)
			    {
				if (!*tmp)
					config_error(CF_WARN, CK_FILE, CK_LINE,
						"no class, default 0");
				aconf->class = get_class(atoi(tmp), nr);
			    }
			tmp3 = getfield(NULL);
		} while (0); /* to use break without compiler warnings */

		if (!aconf->class && (aconf->status & (CONF_CONNECT_SERVER|
		     CONF_NOCONNECT_SERVER|CONF_OPS|CONF_CLIENT)))
		    {
			(void)fprintf(stderr,
				"\tWARNING: No class.  Default 0\n");
			aconf->class = get_class(0, nr);
		    }
		/*
                ** If conf line is a class definition, create a class entry
                ** for it and make the conf_line illegal and delete it.
                */
		if (aconf->status & CONF_CLASS)
		    {
			if (!aconf->host)
			    {
				(void)fprintf(stderr,"\tERROR: no class #\n");
				continue;
			    }
			if (!tmp)
			    {
				(void)fprintf(stderr,
					"\tWARNING: missing sendq field\n");
				(void)fprintf(stderr, "\t\t default: %d\n",
					QUEUELEN);
				(void)sprintf(maxsendq, "%d", QUEUELEN);
			    }
			else
				(void)sprintf(maxsendq, "%d", atoi(tmp));
			new_class(atoi(aconf->host));
			aconf->class = get_class(atoi(aconf->host), nr);
			goto print_confline;
		    }

		if (aconf->status & CONF_LISTEN_PORT)
		    {
#ifdef	UNIXPORT
			struct	stat	sb;

			if (!aconf->host)
				(void)fprintf(stderr, "\tERROR: %s\n",
					"null host field in P-line");
			else if (index(aconf->host, '/'))
			    {
				if (stat(aconf->host, &sb) == -1)
				    {
					(void)fprintf(stderr, "\tERROR: (%s) ",
						aconf->host);
					perror("stat");
				    }
				else if ((sb.st_mode & S_IFMT) != S_IFDIR)
					(void)fprintf(stderr,
						"\tERROR: %s not directory\n",
						aconf->host);
			    }
#else
			if (!aconf->host)
				(void)fprintf(stderr, "\tERROR: %s\n",
					"null host field in P-line");
			else if (index(aconf->host, '/'))
				(void)fprintf(stderr, "\t%s %s\n",
					"WARNING: / present in P-line", 
					"for non-UNIXPORT configuration");
#endif
			aconf->class = get_class(0, nr);
			goto print_confline;
		    }

#ifndef	UNIXPORT
# ifdef USE_SSL
		if (aconf->status & CONF_SSL_LISTEN_PORT)
		    {
			if (!aconf->host)
				(void)fprintf(stderr, "\tERROR: %s\n",
					"null host field in p-line");
			else if (index(aconf->host, '/'))
				(void)fprintf(stderr, "\tWARNING: %s\n",
					"UNIXHOST is not allowed in p-line");
			aconf->class = get_class(0, nr);
			goto print_confline;
		    }
# endif /* USE_SSL */
#endif /* UNIXPORT */
		if (aconf->status & CONF_SERVER_MASK &&
		    (!aconf->host || index(aconf->host, '*') ||
		     index(aconf->host, '?')))
		    {
			(void)fprintf(stderr, "\tERROR: bad host field\n");
			continue;
		    }

		if (aconf->status & CONF_SERVER_MASK && BadPtr(aconf->passwd))
		    {
			(void)fprintf(stderr,
					"\tERROR: empty/no password field\n");
			continue;
		    }

		if (aconf->status & CONF_SERVER_MASK && !aconf->name)
		    {
			(void)fprintf(stderr, "\tERROR: bad name field\n");
			continue;
		    }

		if (aconf->status & (CONF_SERVER_MASK|CONF_OPS))
			if (!index(aconf->host, '@'))
			    {
				char	*newhost;
				int	len = 3;	/* *@\0 = 3 */

				len += strlen(aconf->host);
				newhost = (char *)MyMalloc(len);
				(void)sprintf(newhost, "*@%s", aconf->host);
				(void)free(aconf->host);
				aconf->host = newhost;
			    }
		if (aconf->status & CONF_INTERFACE)
		    {
			if (!aconf->host)
			    {
				(void)fprintf(stderr, "X: ERROR: bad target IP field\n");
				continue;
			    }
			if (!aconf->name)
			    {
				(void)fprintf(stderr, "X: ERROR: bad source IP field\n");
				continue;
			    }
		    }
		if (!aconf->class)
			aconf->class = get_class(0, nr);
		(void)sprintf(maxsendq, "%d", aconf->class->class);

		if (!aconf->name)
			aconf->name = nullfield;
		if (!aconf->passwd)
			aconf->passwd = nullfield;
		if (!aconf->host)
			aconf->host = nullfield;
		if (aconf->status & (CONF_ME|CONF_ADMIN))
		    {
			if (flags & aconf->status)
				(void)fprintf(stderr,
					"ERROR: multiple %c-lines\n",
					toupper(confchar(aconf->status)));
			else
				flags |= aconf->status;
		    }

		if (aconf->status & CONF_VER)
		    {
			if (*aconf->host && !index(aconf->host, '/'))
				(void)fprintf(stderr,
					      "\tWARNING: No / in V line.");
			else if (*aconf->passwd && !index(aconf->passwd, '/'))
				(void)fprintf(stderr,
					      "\tWARNING: No / in V line.");
		    }
print_confline:
		if (debugflag > 8)
			(void)printf("(%d) (%s) (%s) (%s) (%d) (%s)\n",
			      aconf->status, aconf->host, aconf->passwd,
			      aconf->name, aconf->port, maxsendq);
		(void)fflush(stdout);
		if (aconf->status & (CONF_SERVER_MASK|CONF_HUB|CONF_LEAF))
		    {
			aconf->next = ctop;
			ctop = aconf;
			aconf = NULL;
		    }
	    }
	(void)close(fdn);
#ifdef	M4_PREPROC
	(void)wait(0);
#endif

	if (!(mandatory_found & CONF_ME))
	    {
	        fprintf(stderr, "No M-line found (mandatory)\n");
		valid = 0;
	    }

	if (!(mandatory_found & CONF_ADMIN))
	    {
		fprintf(stderr, "No A-line found (mandatory)\n");
		valid = 0;
	    }

	if (!(mandatory_found & CONF_LISTEN_PORT))
	    {
		fprintf(stderr, "No P-line found (mandatory)\n");
		valid = 0;
	    }

	if (!(mandatory_found & CONF_CLIENT))
	    {
		fprintf(stderr, "No I-line found (mandatory)\n");
		valid = 0;
	    }

	if (!valid)
	    {
		return NULL;
	    }

	return ctop;
}

static	aClass	*get_class(int cn, int nr)
{
	static	aClass	cls;
	int	i = numclasses - 1;
	aConfig *filelist;

	cls.class = -1;
	for (; i >= 0; i--)
		if (classarr[i] == cn)
		    {
			cls.class = cn;
			break;
		    }
	if (i == -1)
	    {
	    	filelist = findConfLineNumber(nr);
		config_error(CF_WARN, CK_FILE, CK_LINE,
			"class %d not found", cn);
	    }
	return &cls;
}

static	void	new_class(int cn)
{
	numclasses++;
	if (classarr)
		classarr = (int *)realloc(classarr, sizeof(int) * numclasses);
	else
		classarr = (int *)malloc(sizeof(int));
	classarr[numclasses-1] = cn;
}

/*
 * field breakup for ircd.conf file.
 */
static	char	*getfield(char *irc_newline)
{
	static	char *line = NULL;
	char	*end, *field;

	if (irc_newline)
		line = irc_newline;
	if (line == NULL)
		return(NULL);

	field = line;
	if ((end = (char *)index(line, IRCDCONF_DELIMITER)) == NULL)
	    {
		line = NULL;
		if ((end = (char *)index(field,'\n')) == NULL)
			end = field + strlen(field);
	    }
	else
		line = end + 1;
	*end = '\0';
	return(field);
}


static	void	validate(aConfItem *top)
{
	Reg	aConfItem *aconf, *bconf;
	u_int	otype = 0, valid = 0;
	int	nr;
	aConfig *filelist;

	if (!top)
		return;

	for (aconf = top; aconf; aconf = aconf->next)
	{
		if (aconf->status & CONF_MATCH)
			continue;

		if (aconf->status & CONF_SERVER_MASK)
		    {
			if (aconf->status & (CONF_CONNECT_SERVER|CONF_ZCONNECT_SERVER))
				otype = CONF_NOCONNECT_SERVER;
			else if (aconf->status & CONF_NOCONNECT_SERVER)
				otype = CONF_CONNECT_SERVER;

			for (bconf = top; bconf; bconf = bconf->next)
			    {
				if (bconf == aconf || !(bconf->status & otype))
					continue;
				if (bconf->class == aconf->class &&
				    !mycmp(bconf->name, aconf->name) &&
				    !mycmp(bconf->host, aconf->host))
				    {
					aconf->status |= CONF_MATCH;
					bconf->status |= CONF_MATCH;
						break;
				    }
			    }
		    }
		else
			for (bconf = top; bconf; bconf = bconf->next)
			    {
				if ((bconf == aconf) ||
				    !(bconf->status & CONF_SERVER_MASK))
					continue;
				if (!mycmp(bconf->name, aconf->name))
				    {
					aconf->status |= CONF_MATCH;
					break;
				    }
			    }
	}

	for (aconf = top; aconf; aconf = aconf->next)
		if (aconf->status & CONF_MATCH)
			valid++;
		else
		    {
			nr = aconf->clients;
			filelist = findConfLineNumber(nr);
			config_error(CF_WARN, CK_FILE, CK_LINE,
				"unmatched %c%c%s%c%s%c%s",
				confchar(aconf->status), IRCDCONF_DELIMITER,
				aconf->host, IRCDCONF_DELIMITER,
				SHOWSTR(aconf->passwd), IRCDCONF_DELIMITER,
				aconf->name);
		    }
	return;
}


static	char	confchar(u_int status)
{
	static	char	letrs[] = "QIicNCoOMKARYSLPHV";
	char	*s = letrs;

	status &= ~(CONF_MATCH|CONF_ILLEGAL);

	for (; *s; s++, status >>= 1)
		if (status & 1)
			return *s;
	return '-';
}


static aConfig *findConfLineNumber(int nr)
{
	int     mynr = 1;
	aConfig *p = files;

	for( ; p->next && mynr < nr; p = p->next)
		mynr++;
	return p;
}
