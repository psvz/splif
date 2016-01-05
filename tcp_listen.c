#include "splif.h"

int
tcp_listen(const char *host, const char *serv)
{
	int	n;
const	int	on = 1;
struct	addrinfo	hints, *res, *dum;

	bzero(&hints, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		syslog(LOG_CRIT, "tcp_listen: getaddrinfo for %s:%s fails, %s", host, serv, gai_strerror(n));
                _exit(EXIT_FAILURE);
	}

	dum = res;

	do {
		if ( (n = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) continue;
		if (setsockopt(n, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on) < 0)
			syslog(LOG_ERR, "tcp_listen - can't reuse address: %m");
		if (bind(n, res->ai_addr, res->ai_addrlen) == 0) break;	/* success */

		close(n);

	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
		syslog(LOG_CRIT, "tcp_listen on %s:%s fails: %m", host, serv);
                _exit(EXIT_FAILURE);
	} else {
		if (listen(n, LISQUEUE) < 0) {
			syslog(LOG_CRIT, "tcp_listen - can't listen: %m");
			_exit(EXIT_FAILURE);
		}
	}

	freeaddrinfo(dum);

	return n;
}
