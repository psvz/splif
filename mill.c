#include "splif.h"

void*
mill(void *arg)
{
struct	epoll_event	evi[EPOLLCNK];
	char	*buffer;
	int	nfds, i, m, n, k, fdr, fdw;
const	int	len = BUFPAGES * getpagesize();


	if ( (buffer = malloc(len)) == NULL) {
		syslog(LOG_CRIT, "calloc failure");
		_exit(EXIT_FAILURE);
	}

	for (;;) {
		if ( (nfds = epoll_wait(efd, evi, EPOLLCNK, -1)) < 0) {
			syslog(LOG_CRIT, "epoll_wait: %m");
			_exit(EXIT_FAILURE);
		}
		for (i = 0; i < nfds; i++) {
			fdr = evi[i].data.u64 & 0xFFFFFFFF;
			fdw = evi[i].data.u64 >> 32;
			if ( (n = recv(fdr, buffer, len, 0)) <= 0) {
				conres(fdr);
				conres(fdw);
			} else {
				k = 0;
				while (k < n) if ( (m = send(fdw, buffer + k, n - k, 0)) < 0) {
					conres(fdr);
					conres(fdw);
					break;
				} else k+=m;
			}
		}
	}
}			
