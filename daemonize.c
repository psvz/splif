#include "splif.h"

void
daemonize()
{
	int	fdn;
	pid_t	pid;

	if ( (pid = fork()) < 0) {
		syslog(LOG_ERR, "fork fails: %m\nrunning in shell");
		return;
	}

	if (pid != 0) _exit(0);	//parent exits

	if (setsid() < 0) {	//child becomes session leader
		syslog(LOG_ERR, "setsid fails: %m\nrunning non-daemon");
		return;
	}

	if ( (pid = fork()) < 0) syslog(LOG_ERR, "fork fails: %m\nrunning as leader");
	else if (pid != 0) _exit(0);

	umask(0);
	fdn = chdir("/");

	if ( (fdn = sysconf(_SC_OPEN_MAX)) < 0) fdn = 8192;
	do close(fdn); while (fdn-- > 0);

	if ( (fdn = open("/dev/null", O_RDWR)) != STDIN_FILENO)
		syslog(LOG_ERR, "STDIN is %d, instead of 0", fdn);

	dup2(STDIN_FILENO, STDOUT_FILENO);
	dup2(STDIN_FILENO, STDERR_FILENO);
}
