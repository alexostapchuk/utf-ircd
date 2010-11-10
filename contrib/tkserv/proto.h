/*
 *   $Id: proto.h,v 1.8 2005/08/23 22:28:22 skold Exp $
 */

void sendto_server(char *buf, ...); 
void sendto_user(char *text, ...);
void process_server_output(char *line);
void parse_server_output(char *buffer);
int  server_output(int fd, char *buffer);

void service_pong(void);
void service_notice(char **args);
void service_squery(char **args);
int  service_userhost(char *args);
void squery_help(char **args);
void squery_line(char **args);
void squery_quit(char **args);

void sendlog(char *text, ...);
char *ts(void);

int is_opered(void);

void rehash(int what);
