#include "headers/libunixsocket.h"
#include "taskmasterctl.h"
#include <unistd.h>

int initialize_socket(const char *socket_path)
{
	int sfd = create_unix_stream_socket(socket_path, LIBSOCKET_STREAM);
	write(sfd, "Hello", 5);
	return sfd;
}
