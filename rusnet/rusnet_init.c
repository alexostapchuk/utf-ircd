/* 
** Copyright (c) 1998 [RusNet IRC Network]
**
** File     : rusnet_init.c
** Desc.    : rusnet specific commands
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
 */

#ifndef RUSNET_INIT_C
#define RUSNET_INIT_C

#include "os.h"
#include "s_defines.h"
#include "match_ext.h"

#define	BUFLEN	255
#define CFG_COMMENT	'#'
#define CFG_CMD_END	';'

unsigned char logging_enabled = 0;
unsigned char logfile_name[BUFLEN + 1];
	 char config_prefix[BUFLEN + 1];
unsigned char* servicesname = NULL;

void update_matchtables()
{
     int    i;
     for (i=0xc0; i<=0xdf; i++)
     {
	  touppertab[i] = i+0x20;
          tolowertab[i+0x20] = i;
	  char_atribs[i] = char_atribs[i+0x20] = PRINT_CHAR|ALPHA_CHAR; /* deprecate */
	  CharAttrs[i] = CharAttrs[i+0x20] = PRINT_CHAR|ALPHA_CHAR;
     }

     touppertab[0xa3] = 0xb3; 
     tolowertab[0xb3] = 0xa3;

     char_atribs[0xa3] = char_atribs[0xb3] = PRINT_CHAR|ALPHA_CHAR; /* deprecate */
     CharAttrs[0xa3] = CharAttrs[0xb3] = PRINT_CHAR|ALPHA_CHAR;
}


void initialize_rusnet(char *path)
{
/*
     extern unsigned long rusnet_vaddr;
*/
     extern struct rusnet_route *hdr_rusnet_route;

     FILE *finit;
     char s[BUFLEN + 1];
     int  i,j;
     char *parm[16], *p, *r;
     char *ptr;
     char *codepage_name, *intable, *outtable, *portnum;

     hdr_rusnet_route = NULL; 

     update_matchtables();

     finit = fopen(path, "rt");
     if (finit == NULL)
        return;

     s[BUFLEN] = '\0';	/* terminate buffer string for occasion */
     while (fgets(s,BUFLEN,finit) != NULL)
     {
	r = s;
	/* пропуск пробелов в начале */	
	while ( isspace(*r) )
	{
	    r++;
	}
	if (*r == '#') continue; /* коментарий - на новую строку */
	
	while ( (p = strchr(r, CFG_CMD_END)) != NULL)
	{
	   *p++ = '\0';	/* terminate the token */
	}
	
	for (i = 0, ptr = strtok(r, " \t\n\r"); 
	     ptr != NULL && *ptr != CFG_COMMENT && i < 16; 
	     ptr = strtok(NULL, " \t\n\r"))
	{
	    parm[i++] = ptr;	/* split parameters	*/
	}


    	if (i == 0) continue;	/* Umka: handle empty command	*/
	while (i < 16) parm[i++] = NULL; /* finalize parameters	*/

      /*
       *    Configure rusnet extensions 
       */
           if (strcasecmp(parm[0], "logfile") == 0)
           {
                  logging_enabled = 1;
                  strcpy((char *)logfile_name, parm[1]);
           }
           else
           if (strcasecmp(parm[0], "prefix") == 0)
           {
               strcpy(config_prefix, parm[1]);
           }
	   else
           if (strcasecmp(parm[0], "codepage") == 0)
           {
               codepage_name = parm[1];

               for (j = 0; codepage_name[j] != '\0'; j++)
                    codepage_name[j] = toupper(codepage_name[j]);

               intable = outtable = portnum = NULL;

               for (j = 2; parm[j] != NULL; j++)
               {
                   if (strcasecmp(parm[j], "incoming") == 0)
                      intable = parm[++j];
                   else
                   if (strcasecmp(parm[j], "outgoing") == 0)
                      outtable = parm[++j];
                   else
                   if (strcasecmp(parm[j], "port") == 0)
                      portnum = parm[++j];
               }
             
	       if (portnum != NULL) 
		       rusnet_codepage_register(portnum, codepage_name, 
						intable, outtable);
           }
           else
		   r = p;		/* prepare for next clause	*/
      }

     fclose(finit);
}  

void uninitialize_rusnet(void)
{
     rusnet_codepage_free();
}
#endif
