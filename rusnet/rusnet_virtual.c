/* 
** Copyright (c) 1998 [RusNet IRC Network]
**
** File     : rusnet_virtual.c
** Desc.    : rusnet specific commands
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
 */

#ifndef RUSNET_VIRTUAL_C
#define RUSNET_VIRTUAL_C

#include "os.h"
#include "s_defines.h"
#include "s_externs.h"

#ifdef	INET6
	/* A macro to fill an address structure */
#define	SET_ADDRESS(res, addr, source, target, port)	do {		\
	memset((addr), 0, 10);						\
	(addr)[10] = (addr)[11] = 0xff;	/* mapping */			\
	(res) = inet_pton(AF_INET, (source), (addr) + 12);		\
	if ((res) == 0)							\
		(res) = inet_pton(AF_INET6, (source), (addr));		\
	if ((res) == 1)							\
		memcpy(&(target).sin6_addr, (addr), sizeof(addr));	\
	else								\
		memcpy(&(target).sin6_addr, &in6addr_any,		\
			sizeof(in6addr_any));				\
	(target).sin6_family = AF_INET6;				\
	(target).sin6_port = (port);					\
} while (0)
#endif	/* INET6 */

#define	IS_WORK_ADDRESS(iface, raddr)					\
	(((iface)->SIN_FAMILY == (raddr)->SIN_FAMILY) &&		\
	(memcmp(&((iface)->SIN_ADDR), &((raddr)->SIN_ADDR),		\
		sizeof((iface)->SIN_ADDR)) == 0))

/*
unsigned long rusnet_vaddr = 0L;
*/

struct rusnet_route
{
	struct SOCKADDR_IN	routing_interface;
	struct SOCKADDR_IN	server_interface;
	char			*description;

	struct rusnet_route	*next;
};

struct rusnet_route *hdr_rusnet_route = NULL;


void rusnet_add_route(char *server_ip, char *interface_ip, char *desc)
{
        struct rusnet_route *work;
#ifdef	INET6
	int	res;
	char	addr[16];
#endif	/* INET6 */

	work = (struct rusnet_route *)MyMalloc(sizeof(struct rusnet_route));

	bzero(work, sizeof(struct rusnet_route));

#ifdef	INET6
	SET_ADDRESS(res, addr, interface_ip, work->routing_interface, 0);
	SET_ADDRESS(res, addr, server_ip, work->server_interface, 0);
#else	/* INET6 */
	work->routing_interface.SIN_FAMILY = AF_INET;
	work->server_interface.SIN_FAMILY = AF_INET;
	work->routing_interface.SIN_PORT = 0;
	work->server_interface.SIN_PORT = 0;

	work->routing_interface.SIN_ADDR.S_ADDR = inet_addr(interface_ip);
	work->server_interface.SIN_ADDR.S_ADDR = inet_addr(server_ip);
#endif	/* INET6 */
	DupString(work->description, desc);

	work->next = hdr_rusnet_route;
	hdr_rusnet_route = work;
}

void rusnet_free_routes(void)
{
	struct rusnet_route *work, *work2;

	for (work = hdr_rusnet_route; work != NULL; work = work2)
	{
		work2 = work->next;
		MyFree(work->description);
		MyFree(work);
	}

	hdr_rusnet_route = NULL;
}

int rusnet_bind_interface_address(int fd, struct SOCKADDR_IN *raddr, 
			           char *desc, size_t len)
{
	struct rusnet_route *work;
	int rc, rc2;
	struct SOCKADDR_IN test;
#ifdef DEBUGMODE
	char	addr[48];
#endif

	work = hdr_rusnet_route;

	while (work != NULL) {
		Debug((DEBUG_ERROR,"Server interface: %s",
			inet_ntop(work->server_interface.SIN_FAMILY,
				&(work->server_interface.SIN_ADDR),
				addr, sizeof(addr))));

		Debug((DEBUG_ERROR,"Routing interface: %s",
			inet_ntop(work->routing_interface.SIN_FAMILY,
				&(work->routing_interface.SIN_ADDR),
				addr, sizeof(addr))));

		if (IS_WORK_ADDRESS(&work->server_interface, raddr)) {
			Debug((DEBUG_ERROR,"Binding"));
			Debug((DEBUG_ERROR,"Socket = %d", fd));
			Debug((DEBUG_ERROR,"Local port = %d",
		  		ntohl(work->routing_interface.SIN_PORT)));

			bzero(&test, sizeof(test));
			test.SIN_FAMILY = work->routing_interface.SIN_FAMILY;
			memcpy(&test.SIN_ADDR,
				&(work->routing_interface.SIN_ADDR),
				sizeof(test.SIN_ADDR));
			test.SIN_PORT = 0;

			rc = bind(fd, (struct SOCKADDR *)&test, sizeof(test));
			rc2 = errno;
			if (rc)
				perror("bind"); 
			if (rc2 == EINPROGRESS || rc2 == EWOULDBLOCK)
				Debug((DEBUG_ERROR,
					"EINPROGRESS or EWOULDBLOCK"));

			Debug((DEBUG_ERROR,"bind() retcode = %d", rc));
			strncpyzt(desc, work->description, len);
			return rc;
		}
		work = work->next;
	}

	// Otherwise do nothing
	return -1;
}
#endif
