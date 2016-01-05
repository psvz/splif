#include "splif.h"

/* Global variables initialization */

	int	spuf = -1;
        pthread_mutex_t	pmut = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t	answ = PTHREAD_COND_INITIALIZER;
const	char	*signa = SIGNATUR;


int
main(int argc, char **argv)
{
	char	*ptr;
	int	sfd, n, fda;
const	int	on = 1;
	sigset_t	set;
	pthread_t	tid;
struct	epoll_event	ev = {.events = EPOLLIN};

struct	timespec	tp;

	if (argc < 3) {
		fprintf(stderr, "\nUsage in private VM: %s <Splif host:port> <Private host:port>\
			         \nUsage in public  VM: %s <Splif port> <Public port>\n\n", argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	daemonize();

	if (sigemptyset(&set) < 0) {
		syslog(LOG_ERR, "sigemtyset: %m");
	} else {
		if (sigaddset(&set, SIGPIPE) < 0) {
			syslog(LOG_ERR, "sigaddset: %m");
		} else {
			if ( (errno = pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0)
				syslog(LOG_ERR, "sigmask: %m");
		}
	}

	if ( (efd = epoll_create(EPOLLCNK)) < 0) {
		syslog(LOG_CRIT, "epoll create: %m");
		_exit(EXIT_FAILURE);
        }

	if ( (errno = pthread_create(&tid, NULL, mill, NULL)) != 0) {
		syslog(LOG_CRIT, "mill create: %m");
		_exit(EXIT_FAILURE);
	}

	if ( (ptr = rindex(argv[1], ':')) != NULL) {
		s_host = argv[1];
		s_port = ptr + 1;
		*ptr = '\0';
		if ( (ptr = rindex(argv[2], ':')) == NULL) {
			syslog(LOG_CRIT, "Bad argument: %s", argv[2]);
			_exit(EXIT_FAILURE);
		}
		p_host = argv[2];
		p_port = ptr + 1;
		*ptr = '\0';
		/* Code runs in private VM */
		for (;;) {
			while ( (sfd = tcp_connect(s_host, s_port)) < 0) sleep(PINGTIME);
			if (setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on) < 0)
				syslog(LOG_ERR, "can't set keepalive: %m");

			if (send(sfd, signa, sizeof cbuf, 0) < 0) {
				syslog(LOG_ERR, "sigsend: %m");
				conres(sfd);
				continue;
			}
		armed:	while ( (n = recv(sfd, &cbuf, sizeof cbuf, 0)) < 0 && errno == EINTR);
			if (n < 0) {
				syslog(LOG_ERR, "prirecv from splif: %m");
				conres(sfd);
				continue;
			}
			if (n == 0) {
				close(sfd);
				continue;
			}
			if (n != sizeof cbuf) {
				syslog(LOG_ERR, "prisplif inadequate input");
				goto armed;
			}
			if ( (fda = tcp_connect(p_host, p_port)) < 0) {
				// no logging as it's done inside tcp_tonnect()
				goto armed;
			}
			if ( (fdb = tcp_connect(s_host, s_port)) < 0) {
				// no logging as it's done inside tcp_tonnect()
				conres(fda);
				goto armed;
			}

			ev.data.u64 = fda;
			ev.data.u64 <<= 32;
			ev.data.u64 |= fdb;
			if (epoll_ctl(efd, EPOLL_CTL_ADD, fdb, &ev) < 0) {
				syslog(LOG_ERR, "prifdb epoll add: %m");
				conres(fda);
				conres(fdb);
				goto armed;
			}

			ev.data.u64 = fdb;
			ev.data.u64 <<= 32;
			ev.data.u64 |= fda;
			if (epoll_ctl(efd, EPOLL_CTL_ADD, fda, &ev) < 0) {
				syslog(LOG_ERR, "prifda epoll add: %m");
				conres(fda);
				conres(fdb);
                                goto armed;
			}
			if (send(fdb, &cbuf, sizeof cbuf, 0) < 0) {
				syslog(LOG_ERR, "prifinal send: %m");
				conres(fda);
				conres(fdb);
			}
			//syslog(LOG_ERR, "test error output");		// debug mode
			//syslog(LOG_CRIT, "test critical output");	// debug mode
			goto armed;
		}
	} else {
		s_port = argv[1];
		p_port = argv[2];
		/* Code runs in public VM */
		if ( (errno = pthread_create(&tid, NULL, splifguard, NULL)) != 0) {
                	syslog(LOG_CRIT, "splifguard create: %m");
                	_exit(EXIT_FAILURE);
		}

		sfd = tcp_listen(NULL, p_port);	// listening socket

		for (;;) {
			while ( (fda = accept(sfd, NULL, NULL)) < 0 && errno == EINTR);
			if (fda < 0) {
				syslog(LOG_CRIT, "pubaccept: %m");
				_exit(EXIT_FAILURE);
			}
			if (setsockopt(fda, IPPROTO_TCP, TCP_NODELAY, &on, sizeof on) < 0)
                        	syslog(LOG_ERR, "pubaccept - can't disable Nagle: %m");

			pthread_mutex_lock(&pmut);
			cbuf = fda;
			if (send(spuf, &cbuf, sizeof cbuf, 0) != sizeof cbuf) {
				syslog(LOG_ERR, "fda send incomplete, %m");
				pthread_mutex_unlock(&pmut);
				conres(fda);
				continue;
			}

			clock_gettime(CLOCK_REALTIME_COARSE, &tp);
			tp.tv_sec += XTIMEOUT;

			while ( (n = pthread_cond_timedwait(&answ, &pmut, &tp)) != 0 && n == EINTR);
			if (n != 0) { // TIMEDOUT only option
				syslog(LOG_ERR, "private end timed out");
				pthread_mutex_unlock(&pmut);
				conres(fda);
				continue;
			}

			ev.data.u64 = fda;
                        ev.data.u64 <<= 32;
                        ev.data.u64 |= fdb;
                        if (epoll_ctl(efd, EPOLL_CTL_ADD, fdb, &ev) < 0) goto puberr;

			ev.data.u64 = fdb;
                        ev.data.u64 <<= 32;
                        ev.data.u64 |= fda;
                        if (epoll_ctl(efd, EPOLL_CTL_ADD, fda, &ev) < 0) goto puberr;

			pthread_mutex_unlock(&pmut);

			continue;

		puberr:	syslog(LOG_ERR, "epoll add public duo: %m");
			conres(fda);
			conres(fdb);
			pthread_mutex_unlock(&pmut);
		}
	}
	return 0;
}
