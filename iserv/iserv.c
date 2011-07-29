/* 
** Copyright (c) 2005 skold [RusNet IRC Network]
**
** File     : iserv.c v1.2
** Desc.    : Configuration lines module for ircd
**            For further info see the rusnet-doc/RUSNET_DOC file.
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
** $Id: iserv.c,v 1.8 2010-11-10 13:38:39 gvs Exp $
 */

#include "os.h"
#include "i_defines.h"
#define ISERV_C
#include "i_externs.h"
#undef ISERV_C

static int	do_log = 0, do_stop = 0;
isConfLine	*isinbuf[ISINBUF];
char		isoutbuf[ISOUTBUF];
int		inbuf_count = 0, outbuf_count = 0, write_error = 0;
#if defined(ISERV_DEBUG)
int		mallocs = 0;
#endif
time_t		nextflush;
FILE		*f;
int		isffd_tmp;

RETSIGTYPE dummy(s)
int s;
{
	/* from common/bsd.c */
#ifndef HAVE_RELIABLE_SIGNALS
	(void)signal(SIGALRM, dummy);
	(void)signal(SIGPIPE, dummy);
# ifndef HPUX	/* Only 9k/800 series require this, but don't know how to.. */
#  ifdef SIGWINCH
	(void)signal(SIGWINCH, dummy);
#  endif
# endif
#else
# if POSIX_SIGNALS
	struct  sigaction       act;

	act.sa_handler = dummy;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGALRM);
	(void)sigaddset(&act.sa_mask, SIGPIPE);
#  ifdef SIGWINCH
	(void)sigaddset(&act.sa_mask, SIGWINCH);
#  endif
	(void)sigaction(SIGALRM, &act, (struct sigaction *)NULL);
	(void)sigaction(SIGPIPE, &act, (struct sigaction *)NULL);
#  ifdef SIGWINCH
	(void)sigaction(SIGWINCH, &act, (struct sigaction *)NULL);
#  endif
# endif
#endif
}

RETSIGTYPE s_rehash(s)
int s;
{
# if POSIX_SIGNALS
        struct  sigaction act;

        act.sa_handler = s_rehash;
        act.sa_flags = 0;
        (void)sigemptyset(&act.sa_mask);
        (void)sigaddset(&act.sa_mask, SIGHUP);
        (void)sigaction(SIGHUP, &act, NULL);
# else  
        (void)signal(SIGHUP, s_rehash);
# endif
        do_log = 1;
}

RETSIGTYPE s_die(s)
int s;
{
# if POSIX_SIGNALS
        struct  sigaction act;

        act.sa_handler = s_die;
        act.sa_flags = 0;
        (void)sigemptyset(&act.sa_mask);
        (void)sigaddset(&act.sa_mask, SIGINT);
        (void)sigaddset(&act.sa_mask, SIGTERM);
        (void)sigaction(SIGINT, &act, NULL);
        (void)sigaction(SIGTERM, &act, NULL);
# else  
        (void)signal(SIGINT, s_die);
        (void)signal(SIGTERM, s_die);
# endif
        do_stop = 1;
}

void
init_signals()
{
	/* from ircd/ircd.c setup_signals() */
#if POSIX_SIGNALS
	struct	sigaction act;

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGPIPE);
	(void)sigaddset(&act.sa_mask, SIGALRM);
	(void)sigaddset(&act.sa_mask, SIGUSR2);
# ifdef	SIGWINCH
	(void)sigaddset(&act.sa_mask, SIGWINCH);
	(void)sigaction(SIGWINCH, &act, NULL);
# endif
	(void)sigaction(SIGPIPE, &act, NULL);
	(void)sigaction(SIGUSR2, &act, NULL);

	act.sa_handler = dummy;
	(void)sigaction(SIGALRM, &act, NULL);

	act.sa_handler = s_rehash;
	(void)sigaddset(&act.sa_mask, SIGHUP);
	(void)sigaction(SIGHUP, &act, NULL);

	act.sa_handler = s_die;
	(void)sigaddset(&act.sa_mask, SIGINT);
	(void)sigaction(SIGINT, &act, NULL);

	act.sa_handler = s_die;
	(void)sigaddset(&act.sa_mask, SIGTERM);
	(void)sigaction(SIGTERM, &act, NULL);

//	act.sa_handler = dummy;
//	(void)sigaddset(&act.sa_mask, SIGUSR2);
//	(void)sigaction(SIGUSR2, &act, NULL);
#else
# ifndef	HAVE_RELIABLE_SIGNALS
	(void)signal(SIGPIPE, dummy);
	(void)signal(SIGUSR2, dummy);
	(void)signal(SIGALRM, dummy);   
#  ifdef	SIGWINCH
	(void)signal(SIGWINCH, dummy);
#  endif
# else
	(void)signal(SIGPIPE, SIG_IGN);
	(void)signal(SIGUSR2, SIG_IGN);
	(void)signal(SIGALRM, SIG_IGN);   
#  ifdef	SIGWINCH
	(void)signal(SIGWINCH, SIG_IGN);
#  endif
# endif
	(void)signal(SIGHUP, s_rehash);
	(void)signal(SIGTERM, s_die); 
	(void)signal(SIGINT, s_die);
#endif
}

/* Main loop */
int	main(argc, argv)
int	argc;
char	*argv[];
{
	
	char	*host, *port, buffer[ISREADBUF], last_buf[ISREADBUF]; 
	nextflush = time(NULL) + ISCHECK;


	if (argc == 2 && !strcmp(argv[1], "-X"))
		exit(0);

	if (isatty(0))
	{
	    (void)printf("iserv %s\n", make_version());
#if defined(INET6)
	    (void)printf("\t+INET6\n");
#endif
#if defined(USE_POLL)
	    (void)printf("\t+USE_POLL\n");
#endif
	    exit(0);
	}
	init_signals();
	init_syslog();
	init_filelogs();
	sendto_log(LOG_NOTICE, "Daemon starting (%s%s).",
		   make_version(),
#if defined(ISERV_DEBUG)
                   "+debug"
#else
		    ""
#endif				      
		   );
	while (1)
	    {
		loop_io();
		DebugLog((DEBUG_MISC, "Incoming buffer counter %d, outgoing buffer counter: %d, %d allocated",
			inbuf_count, outbuf_count, isoutbuf ? strlen(isoutbuf) : 0));
		if (do_log)
		    {
			sendto_log(LOG_INFO,
				   "Got SIGHUP, reinitializing log file(s).");
			init_filelogs();
			init_syslog();
			do_log = 0;
		    }
		if (do_stop)
		{
			sendto_log(LOG_INFO, "Got SIGINT/SIGTERM, exiting.");
			DebugLog((DEBUG_IO, "Got SIGINT/SIGTERM, exiting."));
			exit(0);
		}
		if ((time(NULL) > nextflush)) {
		    DebugLog((DEBUG_RAW, "Lost memory allocations: %d", mallocs));
		    if (flushPendingLines() > 0)
		    {
			sendto_log(LOG_CRIT, "Fatal write errors, daemon exiting...");
			exit(0);
		    }
		    nextflush = time(NULL) + ISCHECK;
		}
	    }
}

/*
 * field breakup for ircd.conf file (from ircd/parse.c)
 */
char    *getfield(irc_newline)
char    *irc_newline;
{
        static  char *line = NULL;
        char    *end, *field;

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

/* Accept new string and return structure */

isConfLine *makePendingLine(char *line)
{
/*
 ** Expected syntax:
 ** new:
 ** LINE:<user@hostmask|user@ipmask>:<reason>:<nick>:<port>:<expiration_timestamp>
 ** old: not supported
 ** LINE:<host|ip>:<reason>:<user>:<port>:<expiration_timestamp>
 ** Last field if empty or zero makes line permanent
 ** If negative - line is to be removed
 */
    Reg	char	*tmp;
    isConfLine	*tmp_line;

    tmp_line = (isConfLine *) MyMalloc(sizeof(isConfLine));
#ifdef ISERV_DEBUG
    mallocs++;
#endif
    if (!line)
	return;

	DebugLog((DEBUG_RAW, "> makePendingLine(%s)", line));
	DebugLog((DEBUG_MISC, "Making new line: %s", line));

	if ((tmp = getfield(line)) == NULL) {
	    sendto_log(LOG_ERR, "BROKEN: no line type"); 
	    goto ILLEGAL_LINE;
	}
	tmp_line->type = *tmp;
	
	if ((tmp = getfield(NULL)) == NULL) {
	    sendto_log(LOG_ERR, "BROKEN: no host address"); 
	    goto ILLEGAL_LINE;
	}
	DupString(tmp_line->userhost, tmp);
#ifdef ISERV_DEBUG
	mallocs++;
#endif
	if ((tmp = getfield(NULL)) == NULL) {
	    sendto_log(LOG_ERR, "BROKEN: no reason field"); 
	    goto ILLEGAL_LINE;
	}
	DupString(tmp_line->passwd, tmp);
#ifdef ISERV_DEBUG
	mallocs++;
#endif
	if ((tmp = getfield(NULL)) == NULL) {
	    sendto_log(LOG_ERR, "BROKEN: no nick"); 
	    goto ILLEGAL_LINE;
	}
	DupString(tmp_line->name, tmp);
#ifdef ISERV_DEBUG
	mallocs++;
#endif
	if ((tmp = getfield(NULL)) == NULL) {
	    sendto_log(LOG_ERR, "BROKEN: no port");
	    goto ILLEGAL_LINE;
	}
	DupString(tmp_line->port, tmp);
#ifdef ISERV_DEBUG
	mallocs++;
#endif
	if ((tmp = getfield(NULL)) == NULL) {
	    tmp_line->expire = 0;
	    tmp_line->status = LINE_ADD;
	}
	else {
	    tmp_line->expire = (time_t)atoi(tmp);
	    if ((tmp_line->expire <= time(NULL)) && (tmp_line->expire > 0))
		goto ILLEGAL_LINE;
	    tmp_line->status = (tmp_line->expire >= 0) ? LINE_ADD : LINE_REMOVE;
	}
	if (!index(tmp_line->userhost, '@') && *tmp_line->userhost != '/')
	{
	    char	*newhost;
	    int	len = 3;	/* *@\0 = 3 */

	    len += strlen(tmp_line->userhost);
	    newhost = (char *)MyMalloc(len);
#ifdef ISERV_DEBUG
	    mallocs++;
#endif
	    sprintf(newhost, "*@%s", tmp_line->userhost);
	    MyFree(tmp_line->userhost);
#ifdef ISERV_DEBUG
	    mallocs--;
#endif
	    tmp_line->userhost = newhost;
	}
	return tmp_line;
    
ILLEGAL_LINE:
	tmp_line->status = LINE_ILLEGAL;
	return(tmp_line);
}

/* Free ConfLine item */
void delPendingLine(isConfLine *isline)
{
    if (!isline)
	return;
    DebugLog((DEBUG_RAW, "> delPendingLine(%s)", (isline && isline->userhost) ? isline->userhost : "NULL"));
    if (isline->userhost) {
	bzero(isline->userhost, strlen(isline->userhost));
	MyFree(isline->userhost);
#ifdef ISERV_DEBUG
	mallocs--;
#endif
    }
    if (isline->name) {
	bzero(isline->name, strlen(isline->name));
	MyFree(isline->name);
#ifdef ISERV_DEBUG
	mallocs--;
#endif
    }
    if (isline->passwd) {
	bzero(isline->passwd, strlen(isline->passwd));
	MyFree(isline->passwd);
#ifdef ISERV_DEBUG
	mallocs--;
#endif
    }
    if (isline->port) {
	bzero(isline->port, strlen(isline->port));
	MyFree(isline->port);
#ifdef ISERV_DEBUG
	mallocs--;
#endif    
    }
    bzero(isline, sizeof(isline));
    MyFree(isline);
#ifdef ISERV_DEBUG
    mallocs--;
#endif
}

void addPendingLine(line)
char *line;
{
    int 	i = 0;
    isConfLine	*tmp = NULL;
    if (!line)
	return;
    sendto_log(LOG_INFO, "CHECK: %s", line);
    DebugLog((DEBUG_RAW, "> addPendingLine(%s)", line));

    tmp = makePendingLine(line); /* Don't try to use line later, it is now broken */

    for (i = inbuf_count - 1; i >= 0; i--)
    {
	if (match_tline(tmp, isinbuf[i]) == 0)
	{
	    delPendingLine(isinbuf[i]);
	    isinbuf[i] = tmp;
	    return;
	}
    }
    if (inbuf_count < ISINBUF - 1)
	isinbuf[inbuf_count++] = tmp;
    else {
    	isinbuf[inbuf_count++] = tmp;
	flushPendingLines();
	nextflush = time(NULL) + ISCHECK;
    }
}
/* If configuration line passed all checks, add it to the delayed output buffer 
** which has to be dumped to the file.
*/
int appendLine(line)
char *line;
{
    int	bytes = 0;
    int	flush_error = 1;
    
    if (!line)
	return;
    DebugLog((DEBUG_RAW, "> appendLine(%s)", line));

    bytes = strlen(line);
    if ((ISOUTBUF - outbuf_count - 2) < bytes)
    {
	flush_error = flushOutBuffer();
    }
    if (!flush_error)
    {
	strncpy(isoutbuf + outbuf_count, line, bytes);
	outbuf_count += bytes;
	*(isoutbuf + outbuf_count) = '\n';
	outbuf_count++;
	*(isoutbuf + outbuf_count) = '\0';
	sendto_log(LOG_INFO, "OLD: %s", line);
	return(bytes + 1);
    } else {
	return 0;
    }
}

int appendConfLine(isConfLine *isline)
{
    /* Max line size is 2 + 64 +  32 + 6 + 11 +    256   */
    /*                 type:host:nick:port:expire:reason */
    /* You are allowed to supply the reason of any length, 
    ** but do we really need this fun? Let's truncate it 
    ** downto 255 chars, that's enough to express yourself -skold
    */
#define ISMAXLINE 400
    int		bytes = 0;
    int		flush_error = 0;

    if (!isline || !isline->userhost || !isline->name || !isline->passwd)
    {
	DebugLog((DEBUG_MISC, "Invalid line argument to appendConfLine()"));
	return 0;
    }
    DebugLog((DEBUG_RAW, "appendConfLine(%s)", isline->userhost ? isline->userhost : "<unknown>"));
    
    if (isline->status & (LINE_ILLEGAL|LINE_REMOVE)) 
	return 0;	
    if ((ISOUTBUF - outbuf_count - 1) < ISMAXLINE)
    {
	flush_error = flushOutBuffer();
    }
    if (!flush_error) {
	bytes = snprintf(isoutbuf + outbuf_count, ISMAXLINE,
		"%c%c%s%c%s%c%s%c%s%c%d\n",
		isline->type, IRCDCONF_DELIMITER,
		isline->userhost, IRCDCONF_DELIMITER,
		isline->passwd, IRCDCONF_DELIMITER,
		isline->name, IRCDCONF_DELIMITER,
		isline->port, IRCDCONF_DELIMITER,
		isline->expire);
	outbuf_count += bytes; /* Next available position */
	if (bytes >= ISMAXLINE) /* Should not happen, write anyway, so don't be stupid */
	    DebugLog((DEBUG_IO, "Written line was truncated. Check yourself."));
	*(isoutbuf + outbuf_count) = '\0';
	DebugLog((DEBUG_IO, "%d bytes added to outgoing buffer", bytes));
	DebugLog((DEBUG_RAW, "< appendConfLine()"));
	return bytes;
    } else {
	return 0;
    }    
}

int flushOutBuffer()
{
	DebugLog((DEBUG_RAW, "flushOutBuffer()"));
	isoutbuf[outbuf_count] = '\0';
    	if (write(isffd_tmp, isoutbuf, strlen(isoutbuf)) == -1) {
	    sendto_log(LOG_CRIT, "Failed (%s) to write iserv tmp file: %s",
		    strerror(errno), mybasename(KILLSCONFTMP));
	    return write_error++;
	}
	outbuf_count = 0;
	bzero(isoutbuf, strlen(isoutbuf));
	DebugLog((DEBUG_MISC, "< flushOutBuffer()"));
	return 0;
}
/* Analyze KILLSCONF file and write pending lines into file  */
/*
 ** Currently only K, R and E lines are supported and stored in KILLSCONF file
 ** by default. Later it is going to be more complicated and more lines will
 ** be suppored. -skold
 ** K, R and E lines always have to go into separate file since they are dynamic.
 **/

int flushPendingLines()
{
    char	*head, *tail, *tmp;
    int		count = 0, i = 0;
    char	buf[ISREADBUF];
    isConfLine	*isline = NULL;
    int		skip_config = 0;
    
    DebugLog((DEBUG_RAW, "> flushPendingLines(): %d lines", inbuf_count));

    if (inbuf_count == 0)
	return 0;
    if ((f = fopen(KILLSCONF, "r")) == NULL) 
	skip_config = 1;
    if (((isffd_tmp = creat(KILLSCONFTMP, 0660))) > -1)
    {
	if (!skip_config) {
	    while (NULL != (fgets(buf, sizeof(buf), f)))
    	    {
                if ((tmp = (char *)index(buf, '\n')))
                        *tmp = 0;
		DebugLog((DEBUG_MISC, "Got from file: %s",
			    buf ? buf : "NULL"));
		if (buf[1] != IRCDCONF_DELIMITER)
		{
		    sendto_log(LOG_ALERT, "Illegal line in config file: %s, skipped", buf ? buf : "NULL");
		    continue;
		}

        	if (!(*buf == 'K' || *buf == 'E' || *buf == 'R')) /* This should be changed 
						into something that does not hurt -skold */
        	{
		    appendLine(buf);
        	}
        	else 
        	{
		    isline = makePendingLine(buf);
		    if (checkPendingLines(isline)) {
			if (isline)
			    DebugLog((DEBUG_MISC, "Config line is valid: %s!%s", isline->name ? isline->name : "NULL", 
				    isline->userhost ? isline->userhost : "NULL"));
			appendConfLine(isline);
		    }
		    delPendingLine(isline);
		}
	    }
	}
	for (i = 0; i < inbuf_count; i++)
	{
	    if (isinbuf[i]->status & (LINE_ILLEGAL | LINE_REMOVE)) {
		delPendingLine(isinbuf[i]);
		continue;
	    }
	    appendConfLine(isinbuf[i]);
	    sendto_log(LOG_INFO, "ADD: %c for %s!%s (%s) expire %s", isinbuf[i]->type, isinbuf[i]->name,
		isinbuf[i]->userhost, isinbuf[i]->passwd, myctime(isinbuf[i]->expire));
	    delPendingLine(isinbuf[i]);
	}
	inbuf_count = 0;
	flushOutBuffer();
    	if (!skip_config && fclose(f) == -1) {
	    sendto_log(LOG_CRIT, "Could not close (%s) file %s", strerror(errno),
		    KILLSCONF);
	    write_error++;
	}
	if (close(isffd_tmp) == -1) {
	    sendto_log(LOG_CRIT, "Could not save (%s) file %s", strerror(errno), 
		    KILLSCONFTMP);
	    return write_error++;
	};
	if (rename(KILLSCONF, KILLSCONFBAK) == -1 && errno != 2) {
		sendto_log(LOG_WARNING, "Failed (%s) to backup current config: %s -> %s.",
		        strerror(errno), mybasename(KILLSCONF), mybasename(KILLSCONFBAK));
		write_error++;
	}
	if (!write_error) {
	    if (rename(KILLSCONFTMP, KILLSCONF) == -1) {
		sendto_log(LOG_CRIT, "Failed (%s) to update current config: %s -> %s.",
		        strerror(errno), mybasename(KILLSCONFTMP), mybasename(KILLSCONF));
		write_error++;
	    }
	    sendto_log(LOG_INFO, "%s saved", mybasename(KILLSCONF));
	}
	DebugLog((DEBUG_MISC, "Exiting from flushOutBuffer() with %d write errors", write_error));
	DebugLog((DEBUG_RAW, "Buffer flushed to conf file %s", 
		mybasename(KILLSCONF)));
	DebugLog((DEBUG_RAW, "< flushPendingLines()"));
	return write_error;   

    } else {
	/* Report error*/
	sendto_log(LOG_CRIT, "Failed (%s) to open tmp config: %s",
		        strerror(errno), mybasename(KILLSCONFTMP));
	DebugLog((DEBUG_RAW, "< flushPendingLines()"));
	return write_error++;
    }
}
 /*
 ** checkPendingLines checks if the given line should be removed or replaced in
 ** the resulting set of lines. 
 ** If line is valid, add it to the output pending buffer and flush to the file later.
 ** Return value is 0 if line is to be removed from config, 1 - otherwise
 */

int checkPendingLines(isConfLine *isline)
{
    time_t	now;
    int		i = 0, tmp = 0;

    DebugLog((DEBUG_RAW, "> checkPendingLines()"));

    now = time(NULL);
    
    if (isline->status & LINE_ILLEGAL)
	return 0;
    if (isline->expire <= now && isline->expire > 0)
	    return 0;

    for (i = 0; i < inbuf_count; i++)
    {
	if (isinbuf[i]->expire <= now && isinbuf[i]->expire > 0)
	{	
	    isinbuf[i]->status = LINE_ILLEGAL;
	    continue;
	}
	if (isinbuf[i]->status & LINE_ILLEGAL)
	    continue;
    	tmp = match_tline(isinbuf[i], isline);
	if (tmp == 0)
	    return 0;
    }
    return 1;
}

 /*
 ** line1 is less than line2 if usermask of line1 is mached by usermask of line2
 ** and expire time of line1 is less than expire time of line2.
 ** They are equal if masks are equal.
 ** Returns -1 if line1 < line2,
 ** 0 if line1 == line2,
 ** 1 if line1 > line2
 ** -2 if line1 not comparable with line2
 ** NOTE: This routine has been simplified. 
 ** Main lines check is inside ircd now.  Iserv should blindly follow the changes
 ** sent from ircd. This was decided so to save us from being doing everything twice. -skold
 */
int match_tline(isConfLine *newline, isConfLine *oldline)
{

    if (newline->type != oldline->type)
    {
	return -2;
    }

    if (!strcmp(newline->userhost, oldline->userhost) && !strcmp(newline->name, oldline->name))
	return 0;
    else
	return -2;
}


/*end of story*/
