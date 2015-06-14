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

static const char *GetIPAddress(struct addrinfo *adr)
{
		// Buffer big enough to read a human-readable IPv6 address (more than enough room for IPv4)
		static char txt[INET6_ADDRSTRLEN];

		// Clear the buffer to make sure it is completely empty from any previous IP address resolutions.
		memset(txt, 0, sizeof(txt));

		// call inet_ntop to convert the binary form of the address to the human-readable form.
		if (!inet_ntop(adr->ai_family, adr->ai_addr, txt, sizeof(txt)))
		{
				// We had an issue, tell the user about it for debug-reasons.
				fprintf(stderr, "Failed to convert ip address from binary to text: %s (%s)\n", strerror(errno), errno);
				return NULL;
		}

		// We now have our text address
		return txt;
}


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
 * Returns: Creates a socket_t struct via args. Returns 1 (true)   *
 * if operation was successful or 0 (false) if not.                *
 *                                                                 *
 * Description: Resolves the hostname and port to an address usable*
 * to the operating system and this application allowing a data    *
 * connection to establish.                                        *
 *                                                                 *
 *******************************************************************/
int ResolveAddress(const char *host, const char *port, socket_t *sock)
{
		// Make sure someone didn't biff and cause a crash.
		assert(host && port && sock);

		// Tell it what kind of socket(s) we want.
		struct addrinfo hints;
		struct addrinfo *servinfo;
		servinfo = malloc(sizeof(struct addrinfo));
		hints.ai_family = AF_INET;       // IPv4 socket
		hints.ai_socktype = SOCK_STREAM; // Streaming socket.
		// Resolve the addresses
		int rv = 1;
		rv = getaddrinfo(host, port, &hints, servinfo);
		
		// Check if there was an error and return a failure state.
		if (rv != 0)
				return 0;

		// Include the address information struct into our socket struct.
		sock->adr = servinfo;
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
socket_t *CreateSocket(socket_t *sock)
{
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

		return sock;
}

/*******************************************************************
 * Function: ConnectSocket                                         *
 *                                                                 *
 * Arguments: socket_t*                                            *
 *                                                                 *
 * Returns: (boolean) Attempts to connect to the resolved host for *
 * the addresses provided and returns a boolean on whether it was  *
 * successful or not.                                              *
 *                                                                 *
 * Description: Creates a connection to a socket so data can be    *
 * transmitted over a connection. Once this function is successfully
 * called then the read and write functions can be used.           *
 *                                                                 *
 *******************************************************************/
int ConnectSocket(socket_t *sock, struct addrinfo *adrinfo)
{
		// Make sure someone didn't biff.
		assert(sock);

		// Check to make sure we connected to something.
		int ConnectionSuccessful = 0;

		// Since some hostnames can resolve to multiple addresses (eg, Round-Robin DNS)
		// this for-loop is required to iterate to one which works.
		for (struct addrinfo *adr = adrinfo; adr; adr = adr->ai_next)
		{
				// Here we check if connect failed, if it did, print an error and continue
				// otherwise we set the ConnectionSuccessful variable to indicate we can leave the loop.
				if (connect(sock->fd, adr->ai_addr, adr->ai_addrlen) == -1)
				{
						// Print to stderr instead of stdout for shell-routing reasons.
						fprintf(stderr, "Connection to %s:%s was unsuccessful: %s (%s)\n", GetIPAddress(adr), sock->port, strerror(errno), errno);
				}
				else
				{
						// Our connection was a success, set the variable and break from the loop.
						ConnectionSuccessful = 1;
						break;
				}
		}

		// Did we find a successful address to connect to?
		if (!ConnectionSuccessful)
		{
				fprintf(stderr, "Failed to find an address to connect to successfully from host %s:%s\n", sock->host, sock->port);
				return 0;
		}

		return 1;
}

void DestroySocket(socket_t *sock)
{
		assert(sock);
		
		if ()
}
