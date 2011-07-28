/* 
** Copyright (c) 1998 [RusNet IRC Network]
**
** File     : rusnet_codepages.c
** Desc.    : rusnet specific commands
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
 */

#ifndef RUSNET_CODEPAGES_C
#define RUSNET_CODEPAGES_C

#include "os.h"
#include "s_defines.h"
#define S_SERV_C
#include "s_externs.h"
#undef S_SERV_C


struct Codepage *translator_list = NULL;
unsigned handle = 1;


unsigned char rusnet_codepage_register(char *port,  char *name, char *intable, 
         char *outtable)
{
    struct Codepage *translator_new;
    extern char config_prefix[256];
    FILE *fp;

    char   intablefile[512], outtablefile[512];
   
    translator_new = (struct Codepage *)MyMalloc(sizeof (struct Codepage)); 

    translator_new->port = atoi(port);

    if (strlen(name) >= MAX_CODEPAGE_NAME_LENGTH)
	name[MAX_CODEPAGE_NAME_LENGTH-1] = 0;
    strncpy(translator_new->id, name, MAX_CODEPAGE_NAME_LENGTH);
    
    if (intable != NULL && intable[0] != 0)
    {
        strcpy(intablefile,config_prefix);
        strcat(intablefile,"/");
        strcat(intablefile,intable);

        handle = open(intablefile, O_RDONLY);
        if (handle <= 0) translator_new->incoming = NULL;
           else
           {
                 translator_new->incoming = MyMalloc(256+1);
                 read(handle, translator_new->incoming, 256);
                 close(handle);
           }
    }
    else translator_new->incoming = NULL;

    if (outtable != NULL && outtable[0] != 0)
    {
        strcpy(outtablefile,config_prefix);
        strcat(outtablefile,"/");
        strcat(outtablefile,outtable);

        handle = open(outtablefile, O_RDONLY);
        if (handle <= 0) translator_new->outgoing = NULL;
           else
           {
                 translator_new->outgoing = MyMalloc(256+1);
                 read(handle, translator_new->outgoing, 256);
                 close(handle);
           }
    }
    else translator_new->outgoing = NULL;

    translator_new->nextptr = translator_list;

    translator_list = translator_new;

    return 1;
}

unsigned char rusnet_codepage_free(void)
{
    struct Codepage *temp2 = translator_list, *temp = NULL;

    while (temp2 != NULL)
    {
          temp = temp2;
          temp2 = temp->nextptr;	  
	  
          MyFree(temp->incoming);
          MyFree(temp->outgoing);
          MyFree(temp);
    }

    translator_list = NULL;
    
    return 1;
}

struct Codepage *rusnet_getptrbyport(unsigned port)
{
    struct Codepage *temp = translator_list;

    if (port == 0) return NULL;

    while (temp != NULL)
    {
          if (temp->port == port) 
          {
              return temp;
          }
          temp = temp->nextptr;
    }

    return NULL;
}

struct Codepage *rusnet_getptrbyname(char *id)
{
    struct Codepage *temp = translator_list;

    if( (id == NULL) || (*id == 0 ) )
    {
	return NULL;
    }
    
    while (temp != NULL)
    {
          if (strcmp(temp->id , id) == 0)
             return temp;
          temp = temp->nextptr;
    }

    return NULL;
}

void rusnet_translate(struct Codepage *work, unsigned char dir, unsigned char *source, int len)
{
    FILE *fp;

    unsigned char *dest;
    unsigned char *table=NULL;
    unsigned i;

    if (work == NULL) 
          return;

    if (len <= 0) 
    {
          return;
    }

    if (work != NULL)
    {
      switch (dir)
      {
          case RUSNET_DIR_INCOMING : table = (unsigned char *) work->incoming;
                                     break;
          case RUSNET_DIR_OUTGOING : table = (unsigned char *) work->outgoing;
				     break;
          default:
 		return; 
      }

      if (table != NULL)
      {
        for (i = 0; i < len; i++)
        {
            source[i] = table[(unsigned char)source[i]];
        }
      }

    }

    return;
}
#endif
