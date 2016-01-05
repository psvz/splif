#include "splif.h"

int
tcp_connect(const char *host, const char *serv)
{
	int	n;
const	int	on = 1;
struct	addrinfo	hints, *res, *dum;

	bzero(&hints, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		syslog(LOG_CRIT, "tcp_connect: getaddrinfo for %s:%s fails, %s", host, serv, gai_strerror(n));
		_exit(EXIT_FAILURE);
	}

	dum = res;

	do {
		if ( (n = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) continue;
		if (connect(n, res->ai_addr, res->ai_addrlen) == 0) break;	/* success */

		close(n);

	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
		syslog(LOG_ERR, "tcp_connect to %s:%s fails: %m", host, serv);
		n = -1;
	} else {
		if (setsockopt(n, IPPROTO_TCP, TCP_NODELAY, &on, sizeof on) < 0)
			syslog(LOG_ERR, "tcp_connect - can't disable Nagle: %m");
	}

	freeaddrinfo(dum);

	return n;
}
