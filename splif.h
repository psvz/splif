/* daemonize() minimum */
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
/* daemonize() minimum */
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <string.h>

#define PINGTIME 10		// seconds
#define SIGNATUR "dp5/a],d"	// 8 bytes
#define EPOLLCNK 10		// max events
#define BUFPAGES 16		// mill buffer in pages
#define LISQUEUE 128		// accept() listening queue
#define XTIMEOUT 5		// seconds, on cond_wait & splifguard read

	char	*s_host, *s_port, *p_host, *p_port;
	int	efd;		// epoll file descriptor

	int	fdb;
extern	int     spuf;		// splif@public end fd
extern	const   char    *signa;
extern	pthread_mutex_t pmut;
extern	pthread_cond_t  answ;
	uint64_t        cbuf;

void daemonize(), conres(int), *mill(void*), *splifguard(void*);
int tcp_connect(const char*, const char*), tcp_listen(const char*, const char*);
