#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include "vector/vec.h"

// Include our socket types and function declarations.
#include "socket/socket.h"

// A global vector to store our socket structures.
vec_t(socket_t) sockets;

// A union to make switching between these types easier.
typedef union
{
		// IPv4 version of the sockaddr struct
		struct sockaddr_in  sin;
		// IPv6 version of the sockaddr struct
		struct sockaddr_in6 sin6;
		// Type-size compatible version
		struct sockaddr     sa;
} sockaddr_t;

/*******************************************************************
 * Function: InitializeSockets                                     *
 *                                                                 *
 * Arguments: (None)                                               *
 *                                                                 *
 * Returns: (int) a true or false value where true = 1, false = 0  *
 *                                                                 *
 * Description: This initializes any internal data structures      *
 * used to manage and organize the sockets and their structures.   *
 *                                                                 *
 *******************************************************************/
int InitializeSockets(void)
{
		// Initialize our global sockets variable so we can start
		// adding data (eg, socket_t structures) to it.
		vec_init(&sockets);
		// Return that we succeeded the above operation.
		return 1;
}

/*******************************************************************
 * Function: DestroySockets                                        *
 *                                                                 *
 * Arguments: (None)                                               *
 *                                                                 *
 * Returns: (int) a true or false value where true = 1, false = 0  *
 *                                                                 *
 * Description: This closes any sockets and deallocates their data *
 * structures and any associated metadata as well as deallocates   *
 * any other data structures associated with the management of the *
 * sockets themselves (eg, our global `sockets' variable.          *
 *                                                                 *
 *******************************************************************/
int DestroySockets(void)
{
		// Iterate over all our sockets and make sure they're closed
		// so we don't leak file descriptors or memory.
		socket_t socket; // Socket variable used for iteration of our type
		int i;           // 'i' for 'iterator' used to determine how many items we've iterated.
		vec_foreach(&sockets, socket, i)
		{
				// Close the file descriptor
				close(socket.fd);

				// Free any memory used.
				free(socket.host);
				free(socket.ip);
		}

		// Deallocate our global vector
		vec_deinit(&sockets);
}

/*******************************************************************
 * Function: ResolveAddress                                        *
 *                                                                 *
 * Arguments:                                                      *
 * - (const char*) Strings containing the hostname and port to     *
 *   resolve                                                       *
 * - (struct addrinfo*) servinfo output                            *
 *                                                                 *
 * Returns: hints and servinfo filled structs via args. Returns    *
 * 1 (true) if operation was successful or 0 (false) if not.       *
 *                                                                 *
 * Description: Resolves the hostname and port to an address usable*
 * to the operating system and this application allowing a data    *
 * connection to establish.                                        *
 *                                                                 *
 *******************************************************************/
void ResolveAddress(const char *host, const char *port, struct addrinfo *servinfo)
{
		// Make sure someone didn't biff and cause a crash.
		assert(host && port && servinfo);

		// Tell it what kind of socket(s) we want.
		struct addrinfo hints;
		hints.ai_family = AF_INET;       // IPv4 socket
		hints.ai_socktype = SOCK_STREAM; // Streaming socket.
		// Resolve the addresses
		int rv = 1;
		rv = getaddrinfo(host, port, &hints, servinfo);
		
		// Check if there was an error and return a failure state.
		if (rv != 0)
				return 0;

		// Everything worked!
		return 1;
}

/*******************************************************************
 * Function: CreateSocket                                          *
 *                                                                 *
 * Arguments: sockaddr_t union                                     *
 *                                                                 *
 * Returns: (socket_t) A pointer to the socket_t structure with a  *
 * newly created socket ready to start a connection or NULL if     *
 * there was an error creating the socket.                         *
 *                                                                 *
 * Description: Creates a socket with the operating system, ready  *
 * for a connection to be established over it. No data can be sent *
 * over this socket quite yet and must be called by ConnectSocket  *
 * before data may be sent or received.                            *
 *                                                                 *
 *******************************************************************/
socket_t *CreateSocket(const struct addrinfo *servinfo)
{
		socket_t sock;
		// call the UNIX socket() syscall to acquire a file descriptor.
		// here, we create a IPv4 socket (AF_INET), tell it that we want
		// a streaming socket with dynamic-length packets, and tell it
		// that we're using TCP/IP
		sock.fd  = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

		// Check if the socket failed to be created.
		if (sock.fd == -1)
				return NULL;

		// Add the socket to the vector.
		vec_push(&sockets, sock);

		// Normally we cannot do this because it's a function scope-local
		// variable but because it's inserted into the vector, we're okay.
		return &sock;
}
