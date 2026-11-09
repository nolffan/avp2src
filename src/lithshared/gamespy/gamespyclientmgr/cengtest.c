/* cengtest.c: GameSpyClientMgr.lib, written from the release's prebuilt build of it.
 *
 * The GameSpy C Engine SDK's sample program, built into the library as it
 * shipped: it fetches and prints a server list.
 */
#include <stdio.h>
#include "goaceng.h"

void ListCallBack1(GServerList serverlist, int msg, void *instance, void *param1, void *param2)
{
	GServer server;

	if (msg == LIST_PROGRESS)
	{
		server = (GServer)param1;
		printf("%s:%d [%d] %s %d/%d %s\n", ServerGetAddress(server), ServerGetQueryPort(server),
			   ServerGetPing(server), ServerGetStringValue(server, "hostname", "(NONE)"),
			   ServerGetIntValue(server, "numplayers", 0), ServerGetIntValue(server, "maxplayers", 0),
			   ServerGetStringValue(server, "mapname", "(NO MAP)"));
	}
	else
		printf("%d %d\n", msg, ServerListState(serverlist));
}

void sampleKeyEnumFn(char *key, char *value, void *instance)
{
	printf("\t%s: %s\n", key, value);
}

void ListCallBack2(GServerList serverlist, int msg, void *instance, void *param1, void *param2)
{
	GServer server;

	if (msg == LIST_PROGRESS)
	{
		server = (GServer)param1;
		printf("%s:%d| \n", ServerGetAddress(server), ServerGetQueryPort(server));
		ServerEnumKeys(server, sampleKeyEnumFn, NULL);
		printf("\n");
	}
}

void PrintServerList(GServerList serverlist)
{
	GServer server;
	int i;

	for (i = 0; i < ServerListCount(serverlist); i++)
	{
		server = ServerListGetServer(serverlist, i);
		printf("%s:%d [%d] %s %d/%d %s\n", ServerGetAddress(server), ServerGetQueryPort(server),
			   ServerGetPing(server), ServerGetStringValue(server, "hostname", "(NONE)"),
			   ServerGetIntValue(server, "numplayers", 0), ServerGetIntValue(server, "maxplayers", 0),
			   ServerGetStringValue(server, "mapname", "(NO MAP)"));
	}
	printf("\n\n\n");
}

int main(int argc, char **argv)
{
	GServerList serverlist;

	serverlist = ServerListNew("ut", "gmtest", "HA6zkS", 10, ListCallBack1, GCALLBACK_FUNCTION, NULL);
	ServerListUpdate2(serverlist, 0, NULL, qt_status);
	ServerListFree(serverlist);
	return 0;
}
