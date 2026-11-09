/* gserverlist.c: GameSpyClientMgr.lib, written from the release's prebuilt build of it.
 *
 * The GameSpy C Engine SDK's server lists: fetching the list from the master
 * server (or the LAN) and querying each server on it. See goaceng.h for the
 * public functions' descriptions.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nonport.h"
#include "goaceng.h"
#include "gserver.h"
#include "darray.h"
#include "gutil.h"

#define MSHOST	"master.gamespy.com"
#define MSPORT	28900
#define SERVER_GROWBY 64
#define LAN_SEARCH_TIME 3000 /* 3 sec */

/* one query in progress */
typedef struct
{
	SOCKET s;
	GServer currentserver;
	unsigned long starttime;
	struct sockaddr_in saddr;
} UpdateInfo;

struct GServerListImplementation
{
	GServerListState state;
	DArray servers;
	UpdateInfo *updatelist; /* dynamic array of updateinfos */
	char gamename[32];
	char seckey[32];
	char enginename[32];
	int maxupdates;
	int nextupdate;
	int abortupdate;
	ListCallBackFn CallBackFn;
	void *instance;
	char *sortkey;
	gbool sortascending;
	SOCKET slsocket;
	unsigned long lanstarttime;
	GQueryType querytype;
	HashTable keylist;
};

GServerList g_sortserverlist; /* global serverlist for sorting info!! */

const int querylengths[] = {7, 6, 7, 9, 13, 8};
char *querystrings[] = {"\\basic\\", "\\info\\", "\\rules\\",
						"\\players\\", "\\info\\\\rules\\", "\\status\\"};

GServerList ServerListNew(char *gamename, char *enginename, char *seckey, int maxconcupdates, void *CallBackFn, int CallBackFnType, void *instance)
{
	GServerList list;

	list = (GServerList)malloc(sizeof(struct GServerListImplementation));
	list->state = sl_idle;
	list->servers = ArrayNew(sizeof(GServer), SERVER_GROWBY, ServerFree);
	list->keylist = TableNew2(sizeof(char *), 500, 4, GStringHash, GCaseInsensitiveCompare, GStringFree);
	list->maxupdates = maxconcupdates;
	list->updatelist = (UpdateInfo *)malloc(maxconcupdates * sizeof(UpdateInfo));
	memset(list->updatelist, 0, maxconcupdates * sizeof(UpdateInfo));
	strcpy(list->gamename, gamename);
	strcpy(list->seckey, seckey);
	strcpy(list->enginename, enginename);
	list->CallBackFn = (ListCallBackFn)CallBackFn;
	list->instance = instance;
	list->sortkey = "";
	SocketStartUp();
	return list;
}

void ServerListFree(GServerList serverlist)
{
	ArrayFree(serverlist->servers);
	TableFree(serverlist->keylist);
	free(serverlist->updatelist);
	free(serverlist);
	SocketShutDown();
}

/* sets up the update list, creating its sockets */
GError InitUpdateList(GServerList serverlist)
{
	int i;

	for (i = 0; i < serverlist->maxupdates; i++)
	{
		serverlist->updatelist[i].s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (serverlist->updatelist[i].s == INVALID_SOCKET)
		{ /* ran out of sockets, just cap maxupdates here, unless we don't have any */
			if (i == 0)
				return GE_NOSOCKET;
			serverlist->maxupdates = i;
			break;
		}
		serverlist->updatelist[i].currentserver = NULL;
		serverlist->updatelist[i].starttime = 0;
	}
	return 0;
}

/* connects to the master server */
GError CreateServerListSocket(GServerList serverlist)
{
	struct sockaddr_in saddr;
	struct hostent *hent;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(MSPORT);
	saddr.sin_addr.s_addr = inet_addr(MSHOST);
	if (saddr.sin_addr.s_addr == INADDR_NONE)
	{
		hent = gethostbyname(MSHOST);
		if (!hent)
			return GE_NODNS;
		saddr.sin_addr.s_addr = *(u_long *)hent->h_addr_list[0];
	}
	serverlist->slsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverlist->slsocket == INVALID_SOCKET)
		return GE_NOSOCKET;
	if (connect(serverlist->slsocket, (struct sockaddr *)&saddr, sizeof saddr) != 0)
	{
		closesocket(serverlist->slsocket);
		return GE_NOCONNECT;
	}
	return 0;
}

/* sets the list's state and tells the app */
void ServerListModeChange(GServerList serverlist, GServerListState newstate)
{
	serverlist->state = newstate;
	serverlist->CallBackFn(serverlist, LIST_STATECHANGED, serverlist->instance, NULL, NULL);
}

/* for a synchronous update: think until the update is done */
void DoSyncLoop(GServerList serverlist)
{
	while (serverlist->state != sl_idle)
	{
		ServerListThink(serverlist);
		msleep(10);
	}
}

GError ServerListUpdate(GServerList serverlist, gbool async)
{
	return ServerListUpdate2(serverlist, async, NULL, qt_status);
}

static GError SendListRequest(GServerList serverlist, char *filter);

GError ServerListUpdate2(GServerList serverlist, gbool async, char *filter, GQueryType querytype)
{
	GError error;

	if (serverlist->state != sl_idle)
		return GE_BUSY;
	serverlist->querytype = querytype;
	error = CreateServerListSocket(serverlist);
	if (error) return error;
	error = SendListRequest(serverlist, filter);
	if (error) return error;
	error = InitUpdateList(serverlist);
	if (error) return error;
	serverlist->nextupdate = 0;
	serverlist->abortupdate = 0;
	if (!async)
		DoSyncLoop(serverlist);
	return 0;
}

/* answers the master's challenge and asks it for the list */
static GError SendListRequest(GServerList serverlist, char *filter)
{
	char data[256], *ptr, result[64];
	int len;

	len = recv(serverlist->slsocket, data, sizeof(data) - 1, 0);
	if (len == SOCKET_ERROR)
		return GE_NOCONNECT;
	data[len] = '\0'; /* null terminate it */
	ptr = strstr(data, "\\secure\\");
	if (ptr == NULL)
		return GE_DATAERROR;
	ptr += 8;
	gs_encrypt((uchar *)serverlist->seckey, 6, (uchar *)ptr, 6);
	gs_encode((uchar *)ptr, 6, (uchar *)result);
	/* validate to the master */
	sprintf(data, "\\gamename\\%s\\gamever\\%s\\location\\0\\validate\\%s\\final\\\\queryid\\1.1\\",
			serverlist->enginename, ENGINE_VERSION, result);
	len = send(serverlist->slsocket, data, strlen(data), 0);
	if (len == SOCKET_ERROR || len == 0)
		return GE_NOCONNECT;
	/* send the list request */
	if (filter)
		sprintf(data, "\\list\\cmp\\gamename\\%s\\where\\%s\\final\\", serverlist->gamename, filter);
	else
		sprintf(data, "\\list\\cmp\\gamename\\%s\\final\\", serverlist->gamename);
	len = send(serverlist->slsocket, data, strlen(data), 0);
	if (len == SOCKET_ERROR || len == 0)
		return GE_NOCONNECT;
	ServerListModeChange(serverlist, sl_listxfer);
	return 0;
}

void ServerListSetQueryType(GServerList serverlist, GQueryType querytype)
{
	serverlist->querytype = querytype;
	serverlist->nextupdate = 0;
	serverlist->abortupdate = 0;
}

static GError CreateServerListLANSocket(GServerList serverlist);
static GError SendBroadcastRequest(GServerList serverlist, int startport, int endport, int delta);

GError ServerListLANUpdate(GServerList serverlist, gbool async, int startsearchport, int endsearchport, int searchdelta)
{
	GError error;

	if (serverlist->state != sl_idle)
		return GE_BUSY;
	error = InitUpdateList(serverlist);
	if (error) return error;
	error = CreateServerListLANSocket(serverlist);
	if (error) return error;
	error = SendBroadcastRequest(serverlist, startsearchport, endsearchport, searchdelta);
	if (error) return error;
	serverlist->nextupdate = 0;
	serverlist->abortupdate = 0;
	if (!async)
		DoSyncLoop(serverlist);
	return 0;
}

static GError CreateServerListLANSocket(GServerList serverlist)
{
	int optval = 1;

	serverlist->slsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverlist->slsocket == INVALID_SOCKET)
		return GE_NOSOCKET;
	if (setsockopt(serverlist->slsocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval)) != 0)
		return GE_NOSOCKET;
	return 0;
}

static GError SendBroadcastRequest(GServerList serverlist, int startport, int endport, int delta)
{
	struct sockaddr_in saddr;
	short i;

	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = 0xFFFFFFFF; /* broadcast */
	for (i = startport; i <= endport; i += delta)
	{
		saddr.sin_port = htons(i);
		sendto(serverlist->slsocket, "\\status\\", 8, 0, (struct sockaddr *)&saddr, sizeof(saddr));
	}
	ServerListModeChange(serverlist, sl_lanlist);
	serverlist->lanstarttime = current_time();
	return 0;
}

/* adds a server to the end of the list */
void ServerListAddServer(GServerList serverlist, unsigned long ip, unsigned short port, GQueryType qtype)
{
	GServer server;

	server = ServerNew(ip, port, qtype, serverlist->keylist);
	ArrayAppend(serverlist->servers, &server);
}

static int ServerListFindServer(GServerList serverlist, unsigned long ip, int port);
static int ServerListFindServerInUpdateList(GServerList serverlist, GServer server);

void ServerListRemoveServer(GServerList serverlist, char *ip, int port)
{
	int currentserver;
	int i;

	currentserver = ServerListFindServer(serverlist, inet_addr(ip), port);
	if (currentserver == -1)
		return;
	if (serverlist->state != sl_idle && serverlist->nextupdate > currentserver)
	{ /* it's already been sent out: take it off the update list */
		i = ServerListFindServerInUpdateList(serverlist, *(GServer *)ArrayNth(serverlist->servers, currentserver));
		if (i != -1)
			serverlist->updatelist[i].currentserver = NULL;
		serverlist->nextupdate--;
	}
	ArrayDeleteAt(serverlist->servers, currentserver);
}

/* returns the index of the server in the list, or -1 */
static int ServerListFindServer(GServerList serverlist, unsigned long ip, int port)
{
	int i;
	GServer server;

	for (i = 0; i < ArrayLength(serverlist->servers); i++)
	{
		server = *(GServer *)ArrayNth(serverlist->servers, i);
		if (port == ServerGetQueryPort(server) && ServerGetInetAddress(server) == ip)
			return i;
	}
	return -1;
}

/* returns the index of the server in the update list, or -1 */
static int ServerListFindServerInUpdateList(GServerList serverlist, GServer server)
{
	int i;

	for (i = 0; i < serverlist->maxupdates; i++)
		if (serverlist->updatelist[i].currentserver == server)
			return i;
	return -1;
}

static void ServerListInsertServer(GServerList serverlist, unsigned long ip, unsigned short port, int pos, GQueryType qtype);

GError ServerListAuxUpdate(GServerList serverlist, char *ip, int port, gbool async, GQueryType querytype)
{
	GServer server;
	int currentserver;
	unsigned long real_ip = inet_addr(ip);
	GError error;
	int i;

	currentserver = ServerListFindServer(serverlist, real_ip, port);
	if (serverlist->state == sl_idle)
	{ /* start an update with just this server */
		error = InitUpdateList(serverlist);
		if (error) return error;
		if (currentserver != -1)
		{ /* move it to the end of the list */
			server = *(GServer *)ArrayNth(serverlist->servers, currentserver);
			server->ping = 9999;
			ArrayRemoveAt(serverlist->servers, currentserver);
			ArrayAppend(serverlist->servers, &server);
		}
		else
			ServerListAddServer(serverlist, real_ip, port, querytype);
		serverlist->nextupdate = ArrayLength(serverlist->servers) - 1;
		serverlist->abortupdate = 0;
		ServerListModeChange(serverlist, sl_querying);
		if (!async)
			DoSyncLoop(serverlist);
		return 0;
	}
	/* an update is running: put the server next in line */
	if (currentserver == -1)
	{
		ServerListInsertServer(serverlist, real_ip, port, serverlist->nextupdate, querytype);
		return 0;
	}
	server = *(GServer *)ArrayNth(serverlist->servers, currentserver);
	if (currentserver < serverlist->nextupdate)
	{ /* it's been updated (or is being updated): requeue it */
		server->ping = 9999;
		i = ServerListFindServerInUpdateList(serverlist, server);
		if (i != -1)
			serverlist->updatelist[i].currentserver = NULL;
		ArrayInsertAt(serverlist->servers, &server, serverlist->nextupdate);
		ArrayRemoveAt(serverlist->servers, currentserver);
		serverlist->nextupdate--;
	}
	return 0;
}

static void ServerListInsertServer(GServerList serverlist, unsigned long ip, unsigned short port, int pos, GQueryType qtype)
{
	GServer server;

	server = ServerNew(ip, port, qtype, serverlist->keylist);
	ArrayInsertAt(serverlist->servers, &server, pos);
}

static GError ServerListLANList(GServerList serverlist);
static GError ServerListReadList(GServerList serverlist);
static GError ServerListQueryLoop(GServerList serverlist);

GError ServerListThink(GServerList serverlist)
{
	switch (serverlist->state)
	{
	case sl_idle:
		return 0;
	case sl_listxfer:
		/* read the data */
		return ServerListReadList(serverlist);
	case sl_lanlist:
		return ServerListLANList(serverlist);
	case sl_querying:
		/* do some queries */
		return ServerListQueryLoop(serverlist);
	}
	return 0;
}

/* reads LAN servers' replies to the broadcast */
static GError ServerListLANList(GServerList serverlist)
{
	struct timeval timeout = {0, 0};
	fd_set set;
	char indata[1500];
	struct sockaddr_in saddr;
	int saddrlen = sizeof(saddr);
	int error;

	while (1) /* we break if the select fails */
	{
		FD_ZERO(&set);
		FD_SET(serverlist->slsocket, &set);
		error = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
		if (SOCKET_ERROR == error || 0 == error) /* no data */
			break;
		error = recvfrom(serverlist->slsocket, indata, sizeof(indata) - 1, 0, (struct sockaddr *)&saddr, &saddrlen);
		if (SOCKET_ERROR == error)
			continue;
		/* RETAIL BUG: indata is not null terminated before it is searched
		   (ServerListQueryLoop does terminate its replies) */
		if (strstr(indata, "\\final\\") != NULL) /* we got a reply: add the server */
			ServerListAddServer(serverlist, saddr.sin_addr.s_addr, ntohs(saddr.sin_port), qt_status);
	}
	if (current_time() - serverlist->lanstarttime > LAN_SEARCH_TIME) /* done waiting for replies */
	{
		closesocket(serverlist->slsocket);
		serverlist->slsocket = INVALID_SOCKET;
		ServerListModeChange(serverlist, sl_querying);
	}
	return 0;
}

/* reads the (compressed) server list from the master: 6 bytes per server */
static GError ServerListReadList(GServerList serverlist)
{
	static char data[2048]; /* static input buffer */
	static int oldlen = 0;
	fd_set set;
	struct timeval timeout = {0, 0};
	int len;
	char *p;
	unsigned long ip;
	unsigned short port;

	FD_ZERO(&set);
	FD_SET(serverlist->slsocket, &set);
	if (select(FD_SETSIZE, &set, NULL, NULL, &timeout) <= 0)
		return 0;
	/* append to data */
	len = recv(serverlist->slsocket, data + oldlen, sizeof(data) - oldlen - 1, 0);
	if (len == SOCKET_ERROR || len == 0)
	{
		closesocket(serverlist->slsocket);
		serverlist->slsocket = INVALID_SOCKET;
		oldlen = 0; /* clear data so it can be used again */
		ServerListHalt(serverlist);
		ServerListModeChange(serverlist, sl_querying);
		return GE_NOCONNECT;
	}
	oldlen += len;
	p = data;
	while (p - data <= oldlen - 6)
	{
		if (strncmp(p, "\\final\\", 7) == 0 || serverlist->abortupdate)
		{ /* we're done with the list */
			closesocket(serverlist->slsocket);
			serverlist->slsocket = INVALID_SOCKET;
			oldlen = 0; /* clear data so it can be used again */
			ServerListModeChange(serverlist, sl_querying);
			return 0;
		}
		if (oldlen < 6) /* no way it could be a full IP, quit */
			break;
		memcpy(&ip, p, 4);
		p += 4;
		memcpy(&port, p, 2);
		p += 2;
		ServerListAddServer(serverlist, ip, ntohs(port), serverlist->querytype);
	}
	/* keep what's left for next time */
	oldlen = oldlen - (p - data);
	memmove(data, p, oldlen);
	return 0;
}

static GError FreeUpdateList(GServerList serverlist);

/* sends out queries and reads their replies */
static GError ServerListQueryLoop(GServerList serverlist)
{
	int i, scount = 0, error, final;
	fd_set set;
	struct timeval timeout = {0, 0};
	char indata[1500];
	struct sockaddr_in saddr;
	int saddrlen = sizeof(saddr);
	GServer server;

	/* first, check for available data */
	FD_ZERO(&set);
	for (i = 0; i < serverlist->maxupdates; i++)
		if (serverlist->updatelist[i].currentserver != NULL) /* there is a server waiting */
		{
			scount++;
			FD_SET(serverlist->updatelist[i].s, &set);
		}
	if (scount > 0) /* there are sockets to check for data */
	{
		error = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
		if (SOCKET_ERROR != error && 0 != error)
			for (i = 0; i < serverlist->maxupdates; i++)
				if (serverlist->updatelist[i].currentserver != NULL && FD_ISSET(serverlist->updatelist[i].s, &set))
				{ /* we can read data! */
					saddrlen = sizeof(saddr);
					error = recvfrom(serverlist->updatelist[i].s, indata, sizeof(indata) - 1, 0, (struct sockaddr *)&saddr, &saddrlen);
					if (saddr.sin_addr.s_addr != serverlist->updatelist[i].saddr.sin_addr.s_addr ||
						saddr.sin_port != serverlist->updatelist[i].saddr.sin_port)
						continue; /* it wasn't from this server */
					if (SOCKET_ERROR != error) /* we got data */
					{
						indata[error] = 0; /* truncate and parse it */
						final = (strstr(indata, "\\final\\") != NULL);
						server = serverlist->updatelist[i].currentserver;
						if (server->ping == 9999) /* set the ping */
							server->ping = (short)(current_time() - serverlist->updatelist[i].starttime);
						ServerParseKeyVals(server, indata);
						if (final) /* it's all done */
						{
							serverlist->CallBackFn(serverlist, LIST_PROGRESS, serverlist->instance, server,
												   (void *)((serverlist->nextupdate * 100) / ArrayLength(serverlist->servers))); /* percent done */
							serverlist->updatelist[i].currentserver = NULL; /* reuse the updatelist */
						}
					}
					else
						serverlist->updatelist[i].currentserver = NULL;
				}
	}
	/* kill expired ones */
	for (i = 0; i < serverlist->maxupdates; i++)
		if (serverlist->updatelist[i].currentserver != NULL && current_time() - serverlist->updatelist[i].starttime > SERVER_TIMEOUT)
		{
			serverlist->updatelist[i].currentserver = NULL; /* reuse the updatelist */
		}
	if (serverlist->abortupdate || (serverlist->nextupdate >= ArrayLength(serverlist->servers) && scount == 0))
	{ /* we are done!! */
		FreeUpdateList(serverlist);
		ServerListModeChange(serverlist, sl_idle);
		return 0;
	}
	/* now, send out queries on available sockets */
	for (i = 0; i < serverlist->maxupdates && serverlist->nextupdate < ArrayLength(serverlist->servers); i++)
		if (serverlist->updatelist[i].currentserver == NULL) /* it's available */
		{
			server = *(GServer *)ArrayNth(serverlist->servers, serverlist->nextupdate);
			serverlist->nextupdate++;
			serverlist->updatelist[i].currentserver = server;
			serverlist->updatelist[i].saddr.sin_family = AF_INET;
			serverlist->updatelist[i].saddr.sin_addr.s_addr = inet_addr(ServerGetAddress(server));
			serverlist->updatelist[i].saddr.sin_port = htons((short)ServerGetQueryPort(server));
			sendto(serverlist->updatelist[i].s, querystrings[server->querytype], querylengths[server->querytype], 0,
				   (struct sockaddr *)&serverlist->updatelist[i].saddr, sizeof(struct sockaddr_in));
			serverlist->updatelist[i].starttime = current_time();
		}
	return 0;
}

/* closes the update list's sockets */
static GError FreeUpdateList(GServerList serverlist)
{
	int i;

	for (i = 0; i < serverlist->maxupdates; i++)
		closesocket(serverlist->updatelist[i].s);
	return 0;
}

GError ServerListHalt(GServerList serverlist)
{
	if (serverlist->state != sl_idle)
		serverlist->abortupdate = 1;
	return 0;
}

GError ServerListClear(GServerList serverlist)
{
	if (serverlist->state != sl_idle) /* can't clear while updating */
		return GE_BUSY;
	ArrayFree(serverlist->servers);
	serverlist->servers = ArrayNew(sizeof(GServer), SERVER_GROWBY, ServerFree);
	return 0;
}

GServerListState ServerListState(GServerList serverlist)
{
	return serverlist->state;
}

char *ServerListErrorDesc(GServerList serverlist, GError error)
{
	switch (error)
	{
	case GE_NOERROR:
		return "";
	case GE_NOSOCKET:
		return "Unable to create socket";
	case GE_NODNS:
		return "Unable to resolve master";
	case GE_NOCONNECT:
		return "Connection to master reset";
	case GE_BUSY:
		return "Server List is busy";
	case GE_DATAERROR:
		return "Unexpected data in server list";
	}
	return "UNKNOWN ERROR CODE";
}

GServer ServerListGetServer(GServerList serverlist, int index)
{
	if (index < 0 || index >= ArrayLength(serverlist->servers))
		return NULL;
	return *(GServer *)ArrayNth(serverlist->servers, index);
}

int ServerListCount(GServerList serverlist)
{
	return ArrayLength(serverlist->servers);
}

static int IntKeyCompare(const void *entry1, const void *entry2);
static int FloatKeyCompare(const void *entry1, const void *entry2);
static int StrCaseKeyCompare(const void *entry1, const void *entry2);
static int StrNoCaseKeyCompare(const void *entry1, const void *entry2);

void ServerListSort(GServerList serverlist, gbool ascending, char *sortkey, GCompareMode comparemode)
{
	ArrayCompareFn comparator;

	switch (comparemode)
	{
	case cm_int:
		comparator = IntKeyCompare;
		break;
	case cm_float:
		comparator = FloatKeyCompare;
		break;
	case cm_strcase:
		comparator = StrCaseKeyCompare;
		break;
	case cm_stricase:
		comparator = StrNoCaseKeyCompare;
		break;
	/* RETAIL BUG: no default; an unknown comparemode sorts with an
	   uninitialised comparator */
	}
	serverlist->sortkey = sortkey;
	serverlist->sortascending = ascending;
	g_sortserverlist = serverlist;
	ArraySort(serverlist->servers, comparator);
}

static int IntKeyCompare(const void *entry1, const void *entry2)
{
	GServer server1 = *(GServer *)entry1;
	GServer server2 = *(GServer *)entry2;
	int diff;

	diff = ServerGetIntValue(server1, g_sortserverlist->sortkey, 0) -
		   ServerGetIntValue(server2, g_sortserverlist->sortkey, 0);
	if (!g_sortserverlist->sortascending)
		diff = -diff;
	return diff;
}

static int FloatKeyCompare(const void *entry1, const void *entry2)
{
	GServer server1 = *(GServer *)entry1;
	GServer server2 = *(GServer *)entry2;
	double f;

	f = ServerGetFloatValue(server1, g_sortserverlist->sortkey, 0) -
		ServerGetFloatValue(server2, g_sortserverlist->sortkey, 0);
	if (!g_sortserverlist->sortascending)
		f = -f;
	if ((float)f > (float)0.0)
		return 1;
	else if ((float)f < (float)0.0)
		return -1;
	else
		return 0;
}

static int StrCaseKeyCompare(const void *entry1, const void *entry2)
{
	GServer server1 = *(GServer *)entry1;
	GServer server2 = *(GServer *)entry2;
	int diff;

	diff = strcmp(ServerGetStringValue(server1, g_sortserverlist->sortkey, ""),
				  ServerGetStringValue(server2, g_sortserverlist->sortkey, ""));
	if (!g_sortserverlist->sortascending)
		diff = -diff;
	return diff;
}

static int StrNoCaseKeyCompare(const void *entry1, const void *entry2)
{
	GServer server1 = *(GServer *)entry1;
	GServer server2 = *(GServer *)entry2;
	int diff;

	diff = _stricmp(ServerGetStringValue(server1, g_sortserverlist->sortkey, ""),
					ServerGetStringValue(server2, g_sortserverlist->sortkey, ""));
	if (!g_sortserverlist->sortascending)
		diff = -diff;
	return diff;
}
