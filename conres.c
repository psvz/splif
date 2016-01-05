#include "splif.h"

void
conres(int fd)
{
struct	linger	lin = {.l_onoff = 1};

	setsockopt(fd, SOL_SOCKET, SO_LINGER, &lin, sizeof lin);
	close(fd);
}
