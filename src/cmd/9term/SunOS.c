#include <u.h>
#include <termios.h>
#include <stropts.h>
#include <libc.h>
#include "term.h"

#define debug 0

int
getpts(int fd[], char *slave)
{
	fd[1] = open("/dev/ptmx", ORDWR);
	if ((grantpt(fd[1]) < 0) || (unlockpt(fd[1]) < 0))
		return -1;
	fchmod(fd[1], 0622);

	strcpy(slave, ptsname(fd[1]));

	fd[0] = open(slave, ORDWR);
	if(fd[0] < 0)
		sysfatal("open %s: %r\n", slave);

	/* set up the right streams modules for a tty */
	ioctl(fd[0], I_PUSH, "ptem");        /* push ptem */
	ioctl(fd[0], I_PUSH, "ldterm");      /* push ldterm */

	return 0;
}

int
childpty(int fd[], char *slave)
{
	int sfd;

	close(fd[1]);
	setsid();
	sfd = open(slave, ORDWR);
	if(sfd < 0)
		sysfatal("open %s: %r\n", slave);
	return sfd;
}

struct winsize ows;

void
updatewinsize(int row, int col, int dx, int dy)
{
	struct winsize ws;

	ws.ws_row = row;
	ws.ws_col = col;
	ws.ws_xpixel = dx;
	ws.ws_ypixel = dy;
	if(ws.ws_row != ows.ws_row || ws.ws_col != ows.ws_col)
	if(ioctl(rcfd, TIOCSWINSZ, &ws) < 0)
		fprint(2, "ioctl TIOCSWINSZ: %r\n");
	ows = ws;
}

static struct termios ttmode;

int
isecho(int fd)
{
	if(tcgetattr(fd, &ttmode) < 0)
		fprint(2, "tcgetattr: %r\n");
	if(debug) fprint(2, "israw %c%c\n",
		ttmode.c_lflag&ICANON ? 'c' : '-',
		ttmode.c_lflag&ECHO ? 'e' : '-');
	return (ttmode.c_lflag&(ICANON|ECHO));
}

int
setecho(int fd, int newe)
{
	int old;

	if(tcgetattr(fd, &ttmode) < 0)
		fprint(2, "tcgetattr: %r\n");
	old = (ttmode.c_lflag&ECHO)==ECHO;
	if(old != newe){
		if(newe)
			ttmode.c_lflag |= ECHO;
		else
			ttmode.c_lflag &= ~ECHO;
		if(tcsetattr(fd, TCSANOW, &ttmode) < 0)
			fprint(2, "tcsetattr: %r\n");
	}
	return old;
}
