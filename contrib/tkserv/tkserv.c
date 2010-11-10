/* 
** Copyright (c) 2003 Erra, Umka. RusNet IRC Network
** Copyright (c) 1998 Kaspar 'Kasi' Landsberg, <kl@berlin.Snafu.DE> 
**
** File     : tkserv.c v1.4.4
** Author   : Kaspar 'Kasi' Landsberg, <kl@snafu.de>
** Desc.    : Temporary K-line Service.
**            For further info see the README file.
** Location : http://www.snafu.de/~kl/tkserv/
** Usage    : tkserv <server> <port>
** E.g.     : tkserv localhost 6667
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
**
** Note: The C version of this service is based on parts of the 
**       ircII-EPIC IRC client by the EPIC Software Labs and on
**       the NoteServ service by Kai 'Oswald' Seidler - see
**       http://oswald.pages.de for more about NoteServ. 
**       Thanks to both. =)
**
** PS: Casting rules the world! (doh)
** 
** INET6 and fprintf() bug fixes by mro - 20000828
** some buffer overflows fixes and general cleanup by Beeth -- 20010307
** 
**   $Id: tkserv.c,v 1.17 2005/08/23 22:28:22 skold Exp $
*/

#include "os.h"
#undef strcasecmp
#include "config.h"
#include "tkconf.h"
#include "proto.h"


/* Max. kill reason length */
#define TKS_MAXKILLREASON 128

/* Used max. length for an absolute Unix path */
#define TKS_MAXPATH 128

/* Max. buffer length (don't change this?) */
#define TKS_MAXBUFFER 8192

/* don't change this either(?) */
#define TKS_MAXARGS 250

/* The version information */
#define TKS_VERSION "TkServ v1.4.5"

#ifdef INET6
#define	SEP	"%"	/* who wanted to use "%" as separator for INET6? */
#define	SEP_C	'%'
#else
#define SEP	":"
#define SEP_C	':'
#endif

static char *nuh;
static char tks_inbuf[TKS_MAXBUFFER] = "";
int fd = -1, tklined = 0;

/*
** Returns the current time in a formated way.
** Yes, "ts" stands for "time stamp". I know,
** this hurts, but i couldn't find any better
** description. ;->
*/
char *tks_ts(void)
{
    static char tempus[256];
    time_t now;
    struct tm *loctime;

    /* Get the current time */
    now = time(NULL);

    /* Convert it to local time representation */
    loctime = localtime(&now);
    
    strftime(tempus, 256, "[%H:%M] %m/%d", loctime);

    return(tempus);
}

/* logging routine, with timestamps */
void tks_log(char *text, ...)
{
    char txt[TKS_MAXBUFFER];
    va_list va;
    FILE *tks_logf;

    tks_logf = fopen(TKSERV_LOGFILE, "a");
    va_start(va, text);
    vsprintf(txt, text, va);

    if (tks_logf != NULL)
        fprintf(tks_logf, "%s %s\n", txt, tks_ts());
    else
    {
        perror(TKSERV_LOGFILE);
        va_end(va);

        return;
    }

    va_end(va);
    fclose(tks_logf);
}

/* sends a string (<= TKS_MAXBUFFER) to the server */
void sendto_server(char *buf, ...)
{
    char buffer[TKS_MAXBUFFER];
    va_list va;

    va_start(va, buf);
    vsprintf(buffer, buf, va);
    write(fd, buffer, strlen(buffer));
    va_end(va);
    
#ifdef TKSERV_DEBUG
    printf("%s\n", buffer);
#endif
}

/* sends a NOTICE to the SQUERY origin */
void sendto_user(char *text, ...)
{
    char *nick, *ch;
    char txt[TKS_MAXBUFFER];
    va_list va;

    nick = (char *) strdup(nuh);

    if ((ch = (char *) strchr(nick, '!')) == NULL) {
        tks_log("Race condition: no nick for squery origin: %s", nuh);
	return;
    }

    *ch  = '\0';

    va_start(va, text);
    vsprintf(txt, text, va);
    sendto_server("NOTICE %s :%s\n", nick, txt);

#ifdef TKSERV_DEBUG
    printf("NOTICE %s :%s\n", nick, txt);
#endif

    va_end(va);
    free(nick);
}

/* tells the user how to use the TKLINE/EXEMPT command */
void usage(char *cmd)
{
    sendto_user("Usage: %s <password> <lifetime> <user@host> <reason>", cmd);
    sendto_user("       %s <password> -1         <user@host>", cmd);
}

void process_server_output(char *line)
{
    char *ptr;
    static char *args[TKS_MAXARGS];
    int i, argc = 0;

    while ((ptr = (char *) strchr(line, ' ')) && argc < TKS_MAXARGS)
    {
        args[argc] = line;
        argc++;
        *ptr = '\0';
        line = ptr + 1;
    }

    if (argc < TKS_MAXARGS)
        args[argc++] = line;

    for (i = argc; i < TKS_MAXARGS; i++)
        args[i] = "";

    /* 
    ** After successfull registering, backup the ircd.conf file
    ** and set the perms of the log file -- the easy way.
    */
    if ((*args[0] == ':') && (!strcmp(args[1], "SERVSET")))
    {
        chmod(TKSERV_ACCESSFILE, S_IRUSR | S_IWRITE);
        chmod(TKSERV_LOGFILE, S_IRUSR | S_IWRITE);
/*
        system("cp "CPATH" "TKSERV_IRCD_CONFIG_BAK);
*/
        tks_log("Registration successful.");
    }
        
    /* We do only react upon PINGs, SQUERYs and &NOTICES */
    if (!strcmp(args[0], "PING"))
        service_pong();

    if ((*args[0] == ':') && (!strcmp(args[1], "SQUERY")))
        service_squery(args);
    
    if (!strcmp(args[0], "&NOTICES"))
        service_notice(args);
} 

/* reformats the server output */
void parse_server_output(char *buffer)
{
    char *ch, buf[TKS_MAXBUFFER];

    /* server sent an empty line, so just return */
    if (!buffer && !*buffer)
        return;

    while ((ch = (char *) strchr(buffer, '\n')))
    {
        *ch = '\0';

        if (*(ch - 1) == '\r')
            *(ch - 1) = '\0';

        snprintf(buf, TKS_MAXBUFFER - 1, "%s%s", tks_inbuf, buffer);

        *tks_inbuf = '\0';

        process_server_output(buf);

        buffer = ch + 1;
    }

    if (*buffer)
        strcpy(tks_inbuf, buffer);
}

/* reads and returns output from the server */
int server_output(int fd, char *buffer)
{
    int n;
    
    n = read(fd, buffer, TKS_MAXBUFFER);
    if (n > 0)
        buffer[n] = '\0';

    
#ifdef TKSERV_DEBUG
    printf("%s\n", buffer);
#endif

    return(n);
}

/* is the origin of the /squery opered? */
int is_opered(void)
{
    char *nick, *ch, *token, *u_num, *userh;
    char buffer[TKS_MAXBUFFER];
    int retv = 0;

    nick = (char *) strdup(nuh);
    ch   = (char *) strchr(nick, '!');
    *ch  = '\0';

    sendto_server("USERHOST %s\n", nick);

    /* get the USERHOST reply (hopefully) */
    if (server_output(fd, buffer) < 0)
    {
        free(nick);
        return(0);  /* read() error */
    }

    token = (char *) strtok(buffer, " ");
    token = (char *) strtok(NULL,   " ");
    u_num = (char *) strdup(token);
    token = (char *) strtok(NULL,   " ");
    token = (char *) strtok(NULL,   " ");
    userh = (char *) strdup(token);

    /* if we got the USERHOST reply, perform the check */ 
    if (!strcmp(u_num, "302"))
    {
        char *ch;
        ch = (char *) strchr(userh, '=') - 1;

        /* is the origin opered? */
        if (*ch == '*')
        {
            char *old_uh, *new_uh, *ch;

            old_uh = (char *) (strchr(nuh, '!') + 1);
            new_uh = (char *) (strchr(userh, '=') + 2);

            if (ch = (char *) strchr(new_uh, '\r'))
                *ch = '\0';

            /* Does the u@h of the USERHOST reply correspond to the u@h of our origin? */
            if (!strcmp(old_uh, new_uh))
                retv = 1;
            else 
                /* 
                ** race condition == we sent a USERHOST request and got the 
                ** USERHHOST reply, but this reply doesn't correspond to our origin
                ** of the SQUERY -- this should never happen (but never say never ;)
                */
                sendto_user("A race condition has occured -- please try again.");
        }
    }
    else
    {
        /*
        ** race condition == we sent a USERHOST request but the next message from
        ** the server was not a USERHOST reply (usually due to lag)
        */
        sendto_user("A race condition has occured -- please try again (and ignore the following error message).");
    }
    free(nick);
    free(u_num);
    free(userh);
    return(retv);
}

/* 
** Look for an entry in the access file and
** see if the origin needs to be opered
*/
int must_be_opered()
{
    FILE *fp;
    int retv = 1;
    
    /* if the access file exists, check for auth */
    if ((fp = fopen(TKSERV_ACCESSFILE, "r")) != NULL)
    {
        char buffer[TKS_MAXBUFFER];
        char *access_uh, *token, *uh;

        while (fgets(buffer, TKS_MAXBUFFER, fp))
        {
            uh    = (char *) (strchr(nuh, '!') + 1);
            token = (char *) strtok(buffer, " ");

            if (token)
            {
                access_uh  = (char *) strdup(token);
                if (access_uh)
                {
                    /* check for access file corruption */
                    if (*access_uh == '\0')
                    {
                        tks_log("Corrupt access file. RTFM. :-)");
                        /* if access file is corrupted, better safe than sorry --B.*/
                        retv = 1;
			break;
                    }
                    /* do we need an oper? */
                    if (*access_uh == '!' &&
                       !fnmatch((char *) (strchr(access_uh, '!') + 1), uh, 0))
                        retv = 0;

                    free(access_uh);
                }
            }
        }
        fclose(fp);
    }
    else
        tks_log("%s not found.", TKSERV_ACCESSFILE);

    return(retv);
}

/* check whether origin is authorized to use the service */
int is_authorized(char *cmd, char *pwd, char *host)
{
    FILE *fp;
    int retv = 0; /* 0 not authorized (perhaps *yet*); negative: errors */

    /* if the access file exists, check for authorization */
    if ((fp = fopen(TKSERV_ACCESSFILE, "r")) != NULL)
    {
	char buffer[TKS_MAXBUFFER];
	char *access_uh = NULL, *access_pwd = NULL;
	char *token, *uh, *ch, *tlds = NULL;
#ifdef CRYPTDES
	char *pwdtmp;
	char salt[3];
#endif
        while ((retv == 0) && fgets(buffer, TKS_MAXBUFFER, fp))
        {
            uh    = (char *) (strchr(nuh, '!') + 1);
            token = (char *) strtok(buffer, " ");

            if (token)
            {
	        if (*token == '!')
		    token++;
                access_uh  = (char *) strdup(token);
                if (access_uh == NULL)
                    retv = -2;
            }

            token = (char *) strtok(NULL, " ");

            if ((retv == 0) && token)
            {
                access_pwd = (char *) strdup(token);
                if (access_pwd == NULL)
                    retv = -2;
#ifdef CRYPTDES
                salt[0] = access_pwd[0];
                salt[1] = access_pwd[1];
                salt[2] = '\0';
#endif
            }

            token = (char *) strtok(NULL, " ");

            if ((retv == 0) && token)
            {
                tlds   = (char *) strdup(token);

		if (!*tlds)	/* protect from empty set */
		    free (tlds);
                if (tlds == NULL)
                    retv = -2;
            }
            else if (ch = (char *) strchr(access_pwd, '\n'))
                *ch = '\0';

            /* check for access file corruption */
            if (!*access_uh || !*access_pwd)
                retv = -3;

            /* check uh, pass and TLD */
            if (!fnmatch(access_uh, uh, 0))
            {
#ifdef CRYPTDES
                pwdtmp = crypt(pwd, salt);
                if (pwdtmp && !strcmp(pwdtmp, access_pwd))
#else
                if (!strcmp(pwd, access_pwd))
#endif
                {
		    if (!tlds)
                        retv = 1; /* no tlds, user has no limits */
                    else
                    {
                        char *token, *ch;
                        int negate = 0;

                        if (ch = (char *) strchr(tlds, '\n'))
                            *ch = '\0';

			for( token = (char *) strtok(tlds, ",");
			     token;			     
			     token = (char *) strtok(NULL, ",")
			    )
			{
			    negate = 0;
                    	    /* '!' negates the given host/domain -> not allowed to tkline */
                    	    if (*token == '!')
                    	    {
                        	token++;
                        	negate = 1;
                    	    }
			    
                    	    if (!fnmatch(token, host, 0))
			    {
                        	retv = negate ? -1 : 1;
				break;
			    }
			}

                    } /* !tlds */
                }
                else
		    tks_log("Wrong password. Ignored.");
            }
            else
		tks_log("Wrong uh. Ignored.");
        } /* EOF fp */

	if (access_uh)
	    free(access_uh);

	if (access_pwd)
	    free(access_pwd);

	if (tlds)
	    free(tlds);

    }
    else
        retv = -4; /* could not open file */

    if (fp)
        fclose(fp);
    switch (retv)
    {
        case -4:
            tks_log("%s not found.", TKSERV_ACCESSFILE); break;
        case -3:
            tks_log("Corrupted access file. RTFM. :-)"); break;
        case -2:
	        tks_log("Out of memory."); break;
        case -1:
            sendto_user("You are not allowed to %s \"%s\".", cmd, host); break;
    }

    retv = retv < 0 ? 0 : retv;    /* errors do not allow authorization */
    return(retv);
}

/*************** ircd.conf section ****************/

/* Appends new tklines to the ircd.conf file */
int add_line(char type, char *host, char *user, char *reason, int lifetime)
{
    FILE *iconf;

    if (iconf = fopen(CPATH, "a"))
    {
        time_t now;

        now = time(NULL);

	if (lifetime > 0)
	    fprintf(iconf, "%c"SEP"%s"SEP"%s"SEP"%s"SEP"0 # %d %u tkserv\n",
				type, host, reason, user, lifetime, now);
	else
	    fprintf(iconf, "%c"SEP"%s"SEP"%s"SEP"%s"SEP"0 # permanent\n",
						type, host, reason, user);
        fclose(iconf);
        rehash(1);
        tks_log("%c"SEP"%s"SEP"%s"SEP"%s"SEP"0 added for %d hour(s) by %s.",
				type, host, reason, user, lifetime, nuh);

        return(1);
    }
    else
        tks_log("Couldn't write to "CPATH);

    return(0);
}

/* Check for expired klines/exempts in the ircd.conf file */
int check_lines(char *host, char *user, int lifetime)
{
    FILE *iconf, *iconf_tmp;
    
    if ((iconf = fopen(CPATH, "r")) && (iconf_tmp = fopen(TKSERV_IRCD_CONFIG_TMP, "w")))
    {
        int count = 0, found = 0;
        time_t now;
        char buffer[TKS_MAXBUFFER];
        char buf_tmp[TKS_MAXBUFFER];
        
        /* just in case... */
        chmod(TKSERV_IRCD_CONFIG_TMP, S_IRUSR | S_IWRITE);

        now = time(NULL);

        while (fgets(buffer, TKS_MAXBUFFER, iconf))
        {
            if (!(*buffer == 'K' || *buffer == 'E' || *buffer == 'R'))
            {
                /* buffer could contain %s,%d etc..., expecially with IPv6
                ** style config - mro
                */
                fputs(buffer, iconf_tmp);
            }
            else
            {
                /*
                ** If lifetime has a value of -1, look for matching
                ** tklines and remove them. Otherwise, perform
                ** the expiration check.
		** New: Let tkserv remove K/E:lines even if they were
		** added by somebody else. --erra
                */
                if (lifetime == -1)
                {
                    char *token;
                    char buf[TKS_MAXBUFFER];

                    strcpy(buf, buffer);

                    token = (char *) strtok(buf, SEP);
                    token = (char *) strtok(NULL, SEP);

                    if (token && !strcasecmp(token, host))
                    {
                        token = (char *) strtok(NULL, SEP);
                        token = (char *) strtok(NULL, SEP);

                        if (token && !strcasecmp(token, user))
                        {
                            count++;
                            found = 1;
                        }
                        else
                            fputs(buffer, iconf_tmp);
                    }
                    else
                        fputs(buffer, iconf_tmp);
                }
		/*
		** don't check K/E:lines w/o timestamps and tkserv sign
		*/
                else if (!strstr(buffer, "tkserv"))
		    fputs(buffer, iconf_tmp);
		else
                {
                    char *ch, *token;
                    char buf[TKS_MAXBUFFER];
                    unsigned long int lifetime, then;
                
                    strcpy(buf, buffer);

                    ch       = (char *) strrchr(buf, '#');
                    token    = (char *) strtok(ch, " ");
                    token    = (char *) strtok(NULL, " ");
                    lifetime = strtoul(token, NULL, 0);
                    token    = (char *) strtok(NULL, " ");
                    then     = strtoul(token, NULL, 0);
            
                    if (!(((now - then) / (60 * 60)) >= lifetime))
                        fputs(buffer, iconf_tmp);
                    else
                        found = 1;
                }
            }
        }
        
        fclose(iconf);
        fclose(iconf_tmp);
	/*
        system("cp "TKSERV_IRCD_CONFIG_TMP" "CPATH);
        unlink(TKSERV_IRCD_CONFIG_TMP);
	*/
        rename(CPATH, TKSERV_IRCD_CONFIG_BAK);
	rename(TKSERV_IRCD_CONFIG_TMP, CPATH);
        
        if (found)
            rehash(-1);

        return(count);
    }
    else
        tks_log("Error while checking for expired klines/exempts...");
}

/* reloads the ircd.conf file  -- the easy way */
void rehash(int what)
{
    FILE *f = fopen(PPATH, "r");

    if (f)
    {
	int pid = 0;

	fscanf(f, "%d", &pid);
	if (pid)
	    kill(pid, SIGHUP);
	else
	    tks_log("Cannot parse pid from " PPATH);

	fclose(f);
    }
    else
        tks_log("Cannot open " PPATH " for reading");

    if (what != -1)
        tklined = what;
}

/*************** end of ircd.conf section **************/
    
/*************** The service command section *************/

/* On PING, send PONG and check for expired tklines */
void service_pong(void)
{
    sendto_server("PONG %s\n", TKSERV_NAME);
    check_lines(NULL, NULL, 0);
}

/* 
** If we get a rehash, tell the origin that the tklines are active/removed 
** and check for expired tklines... 
*/
void service_notice(char **args)
{
    if ((!strcmp(args[4], "reloading") && (!strcmp(args[5], TKSERV_IRCD_CONF))) ||
        (!strcmp(args[3], "rehashing") && (!strcmp(args[4], "Server"))))
    {
        if (tklined)
        {
            sendto_user("TK-line%s.", (tklined > 1) ? "(s) removed" : " active");
            tklined = 0;
        }
    }
}

/* parse the received SQUERY */
void service_squery(char **args)
{
    char *cmd, *ch;

    nuh = (char *) strdup(args[0] + 1);
    cmd = (char *) strdup(args[3] + 1);
 
    if (ch = (char *) strchr(cmd, '\r'))
	*ch = '\0';

    if (!strcasecmp(cmd, "admin"))
    {
	sendto_user(TKSERV_ADMIN_NAME);
	sendto_user(TKSERV_ADMIN_CONTACT);
	sendto_user(TKSERV_ADMIN_OTHER);
    }
    else if (!strcasecmp(cmd, "help"))
	squery_help(args);

    else if (!strcasecmp(cmd, "info"))
	sendto_user("This service is featuring temporary conf lines (kills, exempts and rlines)."
		"It's available at IRC software package in contrib/tkserv directory");
    else if (!strcasecmp(cmd, "quit"))
	squery_quit(args);

    else if (!strcasecmp(cmd, "tkline") || !strcasecmp(cmd, "teline") || !strcasecmp(cmd, "trline"))
	squery_line(args);

    else if (!strcasecmp(cmd, "version"))
	sendto_user(TKS_VERSION);

    else
	sendto_user("Unknown command. Try HELP.");

    free(cmd);
}

/* SQUERY HELP */
void squery_help(char **args)
{
    char *ch, *help_about;

    help_about = args[4];

    if (help_about && *help_about)
    {
	if (ch = (char *) strchr(help_about, '\r'))
            *ch = '\0';

	if (!strcasecmp(help_about, "admin"))
            sendto_user("ADMIN shows you the administrative info for this service.");

	if (!strcasecmp(help_about, "help"))
            sendto_user("HELP <command> shows you the help text for <command>.");

	if (!strcasecmp(help_about, "info"))
            sendto_user("INFO shows you a short description about this service.");

	if (!strcasecmp(help_about, "tkline"))
	{
            sendto_user("TKLINE adds a temporary entry to the server's k-line list.");
            sendto_user("TKLINE is a privileged command.");
	}

	if (!strcasecmp(help_about, "trline"))
	{
            sendto_user("TRLINE adds a temporary entry to the server's r-line list.");
            sendto_user("TRLINE is a privileged command.");
	}

	if (!strcasecmp(help_about, "teline"))
	{
            sendto_user("TELINE adds a temporary entry to the server's e-line list.");
            sendto_user("TELINE is a privileged command.");
	}

	if (!strcasecmp(help_about, "version"))
            sendto_user("VERSION shows you the version information of this service.");
    }
    else
    {
	sendto_user("Available commands:");
	sendto_user("HELP, INFO, VERSION, ADMIN, TKLINE, TRLINE, TELINE.");
	sendto_user("Send HELP <command> for further help.");
    }
}

/* SQUERY TKLINE or EXEMPT */
void squery_line(char **args)
{
    int lifetime, i;
    char type, *passwd, *pattern, *host, *ch, *user = "*";
    char reason[TKS_MAXKILLREASON];

    /* Before we go thru all this, make sure we don't waste our time... */
    if (must_be_opered())
    {
    
#ifdef TKSERV_DEBUG
	printf("User must be opered: %s\n", nuh);
#endif
        if (!is_opered())
	{
#ifdef TKSERV_DEBUG
	    printf("User is not opered: %s\n", nuh);
#endif
	    
            sendto_user("Only IRC-Operators may use this command.");
            return;
	}
    }
    
    /* Make sure we have at least the minimum of necessary arguments */
    if (!(args[6] && *args[6]))
    {
	usage(args[3]);
	return;
    }

    /* 
    ** args[5] is the first of the relevant arguments (after the password).
    ** (The password is always the first argument after TKLINE.)
    */
    i = 5;

    while (args[i] && *args[i])
    {
	if (strchr(args[i], SEP_C))
	{
            sendto_user("Config separator (\"" SEP
			"\") is only allowed in the password.");
            return;
	}

	i++;
    }

    if (args[5] && *args[5])
    {
	if (*args[5] == 'E')	/* special syntax for EXEMPT */
	{
	    lifetime = 0;
//	    strcpy(args[3], "EXEMPT");	/* is shorter than TKLINE. */
	    strcpy(args[3], ":TELINE");
	}
	else if (isdigit(*args[5]) || (*args[5] == '-'))
            lifetime = atoi(args[5]);
	else
	{
            usage(args[3]);
            return;
	}
    }

    /* TKLINE <pass> <lifetime> <u@h> <reason> */
    if ((lifetime >= 0) && !(args[7] && *args[7]))
    {
	usage(args[3]);
	return;
    }

    /* TKLINE <pass> -1 <u@h> (removal of tklines) */
    if ((lifetime == -1) && !(args[6] && *args[6]))
    {
	usage(args[3]);
	return;
    }

    /*
    ** A lifetime of -1 means that user wants to remove a tkline.
    ** A lifetime between 1 and TK_MAXTIME means the user wants to add a tkline.
    ** A lifetime 0 means the user wants to add K:line w/o further removals.
    ** Any other value for the lifetime means the user didnt RTFM and is
    ** rejected.
    */
    if ((lifetime > TK_MAXTIME) || (lifetime < -1) /* || (lifetime == 0) */ )
    {
	sendto_user("<lifetime> must be greater than 0 and less than %d.",
								TK_MAXTIME);
	return;
    }

//    type = (args[3][1] == 'E' || args[3][1] == 'e') ? 'E' : 'K';
    type = toupper(args[3][2]);
    
    if (lifetime >= 0)
    {
#ifdef TKSERV_DEBUG
	printf("Adding %c-line: lifetime >= 0: %s %s %s\n",
				type, args[5], args[6], args[7]);
#endif
	passwd  = args[4];
	pattern = args[6];
	strncpy(reason, args[7], TKS_MAXKILLREASON - 1);
	reason[TKS_MAXKILLREASON - 1] = '\0';
	i = 8;

	/* I know... */
	while(args[i] && *args[i])
	{
            strncat(reason, " ", TKS_MAXKILLREASON - strlen(reason) - 1);
            strncat(reason, args[i], TKS_MAXKILLREASON - strlen(reason) - 1);
            i++;
	}
        
	if (ch = (char *) strchr(reason, '\r'))
            *ch = '\0';
    }
    
    if (lifetime == -1)
    {
#ifdef TKSERV_DEBUG
	printf("Removing %c-line: %s %s %s\n", type, args[5], args[6], args[7]);
#endif
	passwd  = args[4];
	pattern = args[6];

	if (ch = (char *) strchr(pattern, '\r'))
            *ch = '\0';
    }

    /* Don't allow "*@*" and "*" in the pattern */
    if (!strcmp(pattern, "*@*") || !strcmp(pattern, "*"))
    {
	sendto_user("The pattern \"%s\" is not allowed.", pattern);
	tks_log("%s tried to %s %c-line \"%s\".", nuh,
		(lifetime == -1) ? "remove" : "set", type, pattern);
	return;
    }

    /* Split the pattern up into username and hostname */
    if (ch = (char *) strchr(pattern, '@'))
    {
	host = (char *) (strchr(pattern, '@') + 1);
	user = pattern;
	*ch  = '\0';
    }
    else /* user defaults to "*" */
	host = pattern;

    /*
    ** Make sure there's a dot in the hostname.
    ** The reason for this being that i "need" a dot
    ** later on and that i don't want to perform
    ** extra checks whether it's there or not...
    ** Call this lazyness, but it also makes the service faster. ;-)
    */
    if (!strchr(host, '.'))
    {
	sendto_user("The hostname must contain at least one dot.");
	return;
    }

    if (!is_authorized(args[3], passwd, host))
    {
	sendto_user("Authorization failed.");
	return;
    }

    if (lifetime == -1)
    {
	int i;

	i = check_lines(host, user, lifetime);
#ifdef TKSERV_VERBOSE
	sendto_user("%d %c-line%s for \"%s@%s\" found.", i, type,
					(i > 1) ? "s" : "", user, host);
#endif
	if (i > 0)
            rehash(2);
    }
    else if (!add_line(type, host, user, reason, lifetime))
        sendto_user("Error while trying to edit the "CPATH" file.");
}

/* 
** SQUERY QUIT 
** Each time we receive a QUIT via SQUERY we check whether
** the supplied password matches the one in the conf file or not.
** If not, an error is sent out. If yes, we close the connection to
** the server.
*/
void squery_quit(char **args)
{
    char *ch;

    if (ch = (char *) strchr(args[4], '\r'))
	*ch = '\0';

    if (!strcmp(args[4], TKSERV_PASSWORD))
    {
	tks_log("Got QUIT from %s. Terminating.", nuh);
	sendto_server("QUIT :TkServ services has been stopped\n");
    }
    else
    {
	sendto_user("I refuse to QUIT. Have a look at the log to see why.");
	tks_log("Got QUIT from %s with wrong password. Continuing.", nuh);
    }
}

/**************** End of service command section ***************/

int main(int argc, char *argv[])
{

    char *host, *port, buffer[TKS_MAXBUFFER], last_buf[TKS_MAXBUFFER]; 
    char tmp[TKS_MAXPATH];

    int is_unix    = (argv[1] && *argv[1] == '/');
    int sock_type  = (is_unix) ? AF_UNIX : AF_INET;
    int proto_type = SOCK_STREAM;
    int eof        = 0;

    struct in_addr     LocalHostAddr;
    struct sockaddr_in server;
    struct sockaddr_in localaddr;
    struct hostent     *hp;
    struct timeval     timeout;

    fd_set read_set;
    fd_set write_set;

    if ((is_unix) && (argc != 2))
    {
        fprintf(stderr, "Usage: %s <server> <port>\n", argv[0]);
        fprintf(stderr, "       %s <Unix domain socket>\n", argv[0]);

        exit(1);
    } 
    else if (!is_unix && argc != 3)
    {
        fprintf(stderr, "Usage: %s <server> <port>\n", argv[0]);
        fprintf(stderr, "       %s <Unix domain socket>\n", argv[0]);

        exit(1);
    }

    tks_log("TkServ started.");

    if ((fd = socket(sock_type, proto_type, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    /* copy the args into something more documentable */
    host = argv[1];

    if (!is_unix)
        port = argv[2];

    /* Unix domain socket */
    if (is_unix)
    {
        struct sockaddr_un name;
        memset(&name, 0, sizeof(struct sockaddr_un));
        name.sun_family = AF_UNIX;
        strcpy(name.sun_path, host);

        if (connect(fd, (struct sockaddr *) &name, strlen(name.sun_path) + 2) == -1)
        {
            perror("connect");
            close(fd);
            exit(1);
        }
    } else {
        memset(&localaddr, 0, sizeof(struct sockaddr_in));
        localaddr.sin_family = AF_INET;
        localaddr.sin_addr   = LocalHostAddr;
        localaddr.sin_port   = 0;
        if (bind(fd, (struct sockaddr *) &localaddr, sizeof(localaddr)))
        {
            perror("bind");
            close(fd);
            exit(1);
        }

        memset(&server, 0, sizeof(struct sockaddr_in));
        memset(&LocalHostAddr, 0, sizeof(LocalHostAddr));

        if (!(hp = gethostbyname(host)))
        {
            perror("resolv");
            close(fd);
            exit(1);
        }

        memmove(&(server.sin_addr), hp->h_addr, hp->h_length);
        memmove((void *) &LocalHostAddr, hp->h_addr, sizeof(LocalHostAddr));
        server.sin_family = AF_INET;
        server.sin_port   = htons(atoi(port));

        if (connect(fd, (struct sockaddr *) &server, sizeof(server)) == -1)
        {
            perror("connect");
            exit(1);
        }
    }
    /* register the service with SERVICE_WANT_NOTICE */
    sendto_server("PASS %s\n", TKSERV_PASSWORD);
    sendto_server("SERVICE %s localhost %s 33554432 0 :%s\n",
			TKSERV_NAME, TKSERV_DIST, TKSERV_DESC);
    sendto_server("SERVSET 33619968\n");

    timeout.tv_usec = 1000;
    timeout.tv_sec  = 10;

    /* daemonization... i'm sure it's not complete */
#ifndef TKSERV_DEBUG
    /*
    ** To make printf() working in DEBUG section we need fork() to be enclosed with DEBUG ;) --skold
    **/

    switch (fork())
    {
    case -1:
        perror("fork()");
        exit(3);
    case 0:
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (setsid() == -1)
            exit(4);

        break;
    default:
        return 0;
    }
#endif

    /* listen for server output and parse it */
    while (!eof)
    {
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_SET(fd, &read_set);

        if (select(FD_SETSIZE, &read_set, &write_set, NULL, &timeout) == -1)
            perror("select");
        
        if (!server_output(fd, buffer))
        {
            tks_log("Connection closed.");
            tks_log("Last server output was: %s", last_buf);

            eof = 1;
        }

        strcpy(last_buf, buffer);
        parse_server_output(buffer);
    }

    if (nuh)
        free(nuh);
    close(fd);
    exit(0);
}
/* eof */
