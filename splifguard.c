#include "splif.h"

void*
splifguard(void *arg)
{
	int	sfd, fdx, n;
const	int	on = 1;
const	struct	timeval tv = {.tv_sec = XTIMEOUT, .tv_usec = 0};
	char	buffer[sizeof cbuf];

	sfd = tcp_listen(NULL, s_port);

	for (;;) {
		while ( (fdx = accept(sfd, NULL, NULL)) < 0 && errno == EINTR);
		if (fdx < 0) {
			syslog(LOG_CRIT, "splifguard accept: %m");
                        _exit(EXIT_FAILURE);
		}
                if (setsockopt(fdx, IPPROTO_TCP, TCP_NODELAY, &on, sizeof on) < 0)
			syslog(LOG_ERR, "splifguard - can't disable Nagle: %m");

		//tv.tv_sec = XTIMEOUT;
		if (setsockopt(fdx, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0)
			syslog(LOG_ERR, "splifguard - can't set TO: %m");

		while ( (n = recv(fdx, buffer, sizeof buffer, 0)) < 0 && errno == EINTR);
		if (n < sizeof buffer) {
			syslog(LOG_ERR, "splifguard receipt incomplete, %m");
			conres(fdx);
			continue;
		}
/* no need in resetting since we only use epoll with such fd:
		tv.tv_sec = 0;
		if (setsockopt(fdx, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0)
			syslog(LOG_ERR, "splifguard - can't reset TO: %m");
*/
		pthread_mutex_lock(&pmut);

		if (memcmp(buffer, signa, sizeof buffer) == 0) {
			conres(spuf);
			spuf = fdx;
			goto over;
		}

		if (memcmp(buffer, &cbuf, sizeof buffer) == 0) {
			fdb = fdx;
			pthread_cond_signal(&answ);
			goto over;
		}

		conres(fdx);

	over:	pthread_mutex_unlock(&pmut);
	}
}
