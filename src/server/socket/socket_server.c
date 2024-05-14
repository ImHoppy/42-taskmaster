#include "socket_server.h"
#include "taskmasterd.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define debug_write(x) fprintf(stderr, "(%s:%d) %s: %s\n", __FILE__, __LINE__, __FUNCTION__, x)

static int set_unix_socket_path(struct sockaddr_un *saddr, const char *path_or_name)
{
	memset(&saddr->sun_path, 0, sizeof(saddr->sun_path));
	if (path_or_name[0] != 0)
	{
		strncpy(saddr->sun_path, path_or_name, sizeof(saddr->sun_path) - 1);
	}
	else
	{
		// Abstract socket address.
		// Make sure this is not a nulled memory region.
		if (path_or_name[1] == 0)
		{
			errno = EINVAL;
			debug_write("Socket address is too many 0s");
			return FAILURE;
		}
		size_t max_len = 1 + strlen(path_or_name + 1);
		if (max_len > sizeof(saddr->sun_path) - 1)
		{
			debug_write("Abstract socket address is too long");
			errno = ENAMETOOLONG;
			return FAILURE;
		}
		memcpy(saddr->sun_path, path_or_name, max_len);
	}
	return 0;
}

/**
 * @brief Create and connect a new UNIX STREAM socket.
 *
 * Creates and connects a new STREAM socket with the socket given in `path`.
 *
 * If the first byte in path is a null byte, this function assumes that it is an
 * abstract socket address (see `unix(7)`; this is a Linux-specific feature).
 * Following that byte, there must be a normal 0-terminated string.
 *
 * @retval >0 Success; return value is a socket file descriptor
 * @retval <0 Error.
 */
int create_socket(const char *socket_path, int flags)
{
	struct sockaddr_un saddr = {0};
	int sfd;

	if (socket_path == NULL)
		return FAILURE;

	if ((sfd = socket(AF_UNIX, SOCK_STREAM | flags, 0)) < 0)
	{
		debug_write(strerror(errno));
		return FAILURE;
	}

	memset(&saddr, 0, sizeof(struct sockaddr_un));

	if (strlen(socket_path) > (sizeof(saddr.sun_path) - 1))
	{
		debug_write("UNIX socket destination path too long");
		return FAILURE;
	}

	saddr.sun_family = AF_UNIX;
	if ((set_unix_socket_path(&saddr, socket_path)) < 0)
	{
		debug_write("Failed to set UNIX socket path");
		return FAILURE;
	}
	size_t pathlen = strlen(saddr.sun_path);
	if (pathlen == 0) // likely abstract socket address
		pathlen = sizeof(saddr.sun_path);

	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr.sun_family) + pathlen) < 0)
	{
		debug_write(strerror(errno));
		close(sfd);
		return FAILURE;
	}

	return sfd;
}

/**
 * @brief Close a socket
 *
 * This function uses the `close(2)` system call to close a socket.
 *
 * @param sfd The socket file descriptor
 *
 * @retval 0 Success
 * @retval -1 Socket was already closed
 */
int destroy_socket(int sfd)
{
	if (sfd < 0)
		return FAILURE;

	if (close(sfd) < 0)
	{
		debug_write(strerror(errno));
		return FAILURE;
	}

	return SUCCESS;
}

int accept_stream_socket(int fds, int flags)
{
}

int create_server_socket(const char *path, int socktype, int flags)
{
	return SUCCESS;
}
