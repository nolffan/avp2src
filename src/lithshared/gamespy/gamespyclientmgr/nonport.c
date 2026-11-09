/* nonport.c: GameSpyClientMgr.lib, written from the release's prebuilt build of it.
 *
 * The GameSpy C Engine SDK's portability layer (as the release built it,
 * for Win32).
 */
#include "nonport.h"

unsigned long current_time() /* returns current time in msec */
{
	return (GetTickCount());
}

void msleep(unsigned long msec)
{
	Sleep(msec);
}

void SocketStartUp()
{
	WSADATA data;
	WSAStartup(0x0101, &data);
}

void SocketShutDown()
{
	WSACleanup();
}

int SetSockBlocking(SOCKET sock, int isblocking)
{
	unsigned long argp;
	int rcode;

	if (isblocking)
		argp = 0;
	else
		argp = 1;
	rcode = ioctlsocket(sock, FIONBIO, &argp);
	if (rcode == 0)
		return 1;
	else
		return 0;
}
