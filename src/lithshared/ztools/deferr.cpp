// zTools: zMinErr::catcher — the default error function (reconstructed
// from deferr.obj). Builds a diagnostic string in a stack buffer (only
// the first copy is bounds-checked) and exits; the buffer is never
// actually printed in the shipped build.
// Byte-identical. Measured source facts: buffers are msg[84] and num[10]
// (that frame puts num at 0x15(%esp) as shipped); the digit copies walk
// d itself, not a copy; the hex digit is an if/else (a ternary gives
// setle/dec/and); `--d` comes first in the loop body; the loop is
// `do { ... if (i == 4) break; } while (i--);`; and the tail is
// `*o++ = '\n'; *o = 0;` (o[1] = 0 schedules the push before the stores).
#include "ztools.h"

void zMinErr::catcher(const char *s, int e)
{
	char msg[84];
	char num[10];
	char *d = &num[9];
	const char *t;

	*d = 0;

	while (e)
	{
		*--d = e % 10 + '0';
		e /= 10;
	}

	char *o = msg;

	while (*s && o < &msg[64])
		*o++ = *s++;

	t = " - error #";
	while (*t)
		*o++ = *t++;

	while (*d)
		*o++ = *d++;

	t = " Caller IP = ";
	while (*t)
		*o++ = *t++;

	unsigned long ip = caller_ip & 0xffff;
	d = &num[9];
	*d = 0;

	int i = 7;
	do
	{
		--d;
		int c = ip & 15;
		if (c > 9)
			c += 'A' - 10;
		else
			c += '0';
		ip >>= 4;
		*d = c;
		if (i == 4)
			break;
	} while (i--);

	caller_ip = ip;

	while (*d)
		*o++ = *d++;

	*o++ = '\n';
	*o = 0;

	exit(1);
}
