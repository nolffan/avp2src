/* gserver.c: GameSpyClientMgr.lib, written from the release's prebuilt build of it.
 *
 * The GameSpy C Engine SDK's servers: a GServer and its keys/values. Key and
 * value strings are kept once each in the server list's shared key list;
 * a server's key/value pairs point into it.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "nonport.h"
#include "gserver.h"

static int KeyValHashKeyP(const void *elem, int numbuckets);
static int KeyValCompareKeyP(const void *entry1, const void *entry2);

void ServerFree(void *elem)
{
	/* free a server! */
	GServer server = *(GServer *)elem;

	TableFree(server->keyvals);
	free(server);
}

GServer ServerNew(unsigned long ip, unsigned short port, GQueryType qtype, HashTable keylist)
{
	GServer server;
	int nBuckets, nChains;

	server = (GServer)malloc(sizeof(struct GServerImplementation));
	server->ip = ip;
	server->port = port;
	server->ping = 9999;
	server->querytype = qtype;
	/* use a smaller hash table for the smaller queries */
	switch (qtype)
	{
	case qt_basic:
		nBuckets = 4;
		nChains = 2;
		break;
	case qt_info:
		nBuckets = 6;
		nChains = 2;
		break;
	case qt_players:
	case qt_rules:
		nBuckets = 8;
		nChains = 2;
		break;
	case qt_info_rules:
	case qt_status:
	default:
		nBuckets = 8;
		nChains = 4;
		break;
	}
	server->keyvals = TableNew2(sizeof(GKeyValuePair), nBuckets, nChains,
								KeyValHashKeyP, KeyValCompareKeyP, NULL);
	server->keylist = keylist;
	return server;
}

static char *mytok(char *instr, char delim);
static int CheckValidKey(char *key);
static char *LookupKey(GServer server, char *k);

void ServerParseKeyVals(GServer server, char *keyvals)
{
	char *k, *v;
	GKeyValuePair kvpair;

	k = mytok(++keyvals, '\\'); /* skip over starting backslash */
	while (k != NULL)
	{
		v = mytok(NULL, '\\');
		if (v == NULL)
			v = "";
		if (CheckValidKey(k))
		{
			kvpair.key = LookupKey(server, k);
			kvpair.value = LookupKey(server, v);
			TableEnter(server->keyvals, &kvpair);
		}
		k = mytok(NULL, '\\');
	}
}

static char *mytok(char *instr, char delim)
{
	char *result;
	static char *thestr;

	if (instr)
		thestr = instr;
	result = thestr;
	while (*thestr && *thestr != delim)
	{
		thestr++;
	}
	if (thestr == result)
		result = NULL;
	if (*thestr) /* not the null term */
		*thestr++ = '\0';
	return result;
}

static int CheckValidKey(char *key)
{
	char *InvalidKeys[] = {"queryid", "final"};
	int i;

	for (i = 0; i < sizeof(InvalidKeys) / sizeof(InvalidKeys[0]); i++)
	{
		if (strcmp(key, InvalidKeys[i]) == 0)
			return 0;
	}
	return 1;
}

/* LookupKey
 * Returns the shared copy of k from the key list, adding it if needed
 */
static char *LookupKey(GServer server, char *k)
{
	char **keyindex;

	keyindex = (char **)TableLookup(server->keylist, &k);
	if (keyindex != NULL)
		return *keyindex;
	k = _strdup(k);
	TableEnter(server->keylist, &k);
	return k;
}

int ServerGetPing(GServer server)
{
	return server->ping;
}

char *ServerGetAddress(GServer server)
{
	return (char *)inet_ntoa(*(struct in_addr *)&server->ip);
}

unsigned int ServerGetInetAddress(GServer server)
{
	return server->ip;
}

int ServerGetQueryPort(GServer server)
{
	return server->port;
}

static GKeyValuePair *ServerRuleLookup(GServer server, char *key);

char *ServerGetStringValue(GServer server, char *key, char *sdefault)
{
	GKeyValuePair *kv;

	if (strcmp(key, "hostaddr") == 0) /* ooh! they want the hostaddr! */
		return ServerGetAddress(server);
	kv = ServerRuleLookup(server, key);
	if (!kv)
		return sdefault;
	return kv->value;
}

static GKeyValuePair *ServerRuleLookup(GServer server, char *key)
{
	GKeyValuePair kvp;
	char **keyindex;

	keyindex = (char **)TableLookup(server->keylist, &key);
	if (keyindex == NULL)
		return NULL; /* otherwise, the key isn't in the list */
	kvp.key = *keyindex;
	return (GKeyValuePair *)TableLookup(server->keyvals, &kvp);
}

int ServerGetIntValue(GServer server, char *key, int idefault)
{
	GKeyValuePair *kv;

	if (strcmp(key, "ping") == 0) /* ooh! they want the ping! */
		return ServerGetPing(server);
	kv = ServerRuleLookup(server, key);
	if (!kv)
		return idefault;
	return atoi(kv->value);
}

double ServerGetFloatValue(GServer server, char *key, double fdefault)
{
	GKeyValuePair *kv;

	kv = ServerRuleLookup(server, key);
	if (!kv)
		return fdefault;
	return atof(kv->value);
}

char *ServerGetPlayerStringValue(GServer server, int playernum, char *key, char *sdefault)
{
	char newkey[32];

	sprintf(newkey, "%s_%d", key, playernum);
	return ServerGetStringValue(server, newkey, sdefault);
}

int ServerGetPlayerIntValue(GServer server, int playernum, char *key, int idefault)
{
	char newkey[32];

	sprintf(newkey, "%s_%d", key, playernum);
	return ServerGetIntValue(server, newkey, idefault);
}

double ServerGetPlayerFloatValue(GServer server, int playernum, char *key, double fdefault)
{
	char newkey[32];

	sprintf(newkey, "%s_%d", key, playernum);
	return ServerGetFloatValue(server, newkey, fdefault);
}

static void KeyMapF(void *elem, void *clientData);

void ServerEnumKeys(GServer server, KeyEnumFn KeyFn, void *instance)
{
	GEnumData ed;

	ed.EnumFn = KeyFn;
	ed.instance = instance;
	ed.keylist = server->keylist;
	TableMap(server->keyvals, KeyMapF, &ed);
}

static void KeyMapF(void *elem, void *clientData)
{
	GKeyValuePair *kv = (GKeyValuePair *)elem;
	GEnumData *ped = (GEnumData *)clientData;

	ped->EnumFn(kv->key, kv->value, ped->instance);
}

/***********
 * UTILITY FUNCTIONS
 **********/
#define MULTIPLIER -1664117991
static int StringHash(char *s, int numbuckets);

int GStringHash(const void *elem, int numbuckets)
{
	return StringHash(*(char **)elem, numbuckets);
}

static int StringHash(char *s, int numbuckets)
{
	unsigned long hashcode = 0;

	while (*s != 0)
		hashcode = hashcode * MULTIPLIER + tolower(*s++);
	return (hashcode % numbuckets);
}

static int KeyValHashKeyP(const void *elem, int numbuckets)
{
	return StringHash(((GKeyValuePair *)elem)->key, numbuckets);
}

int GCaseInsensitiveCompare(const void *entry1, const void *entry2)
{
	return _stricmp(*(char **)entry1, *(char **)entry2);
}

/* the keys are shared, so the pointers themselves can be compared */
static int KeyValCompareKeyP(const void *entry1, const void *entry2)
{
	return ((GKeyValuePair *)entry1)->key - ((GKeyValuePair *)entry2)->key;
}

void GStringFree(void *elem)
{
	free(*(char **)elem);
}
