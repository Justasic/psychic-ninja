#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
#include "vector/vec.h"

// Include our socket types and function declarations.
#include "socket/socket.h"

// A global vector to store our socket structures.
vec_t(socket_t*) sockets;

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
				fprintf(stderr, "Failed to convert ip address from binary to text: %s (%d)\n", strerror(errno), errno);
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
		socket_t *socket; // Socket variable used for iteration of our type
		int i;           // 'i' for 'iterator' used to determine how many items we've iterated.
		vec_foreach(&sockets, socket, i)
		{
				DestroySocket(socket);
		}

		// Deallocate our global vector
		vec_deinit(&sockets);
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
socket_t *CreateSocket(const char *host, const char *port)
{
		// Allocate the socket structure.
		socket_t *sock = malloc(sizeof(socket_t));

		// Tell it what kind of socket(s) we want.
		struct addrinfo hints;
		struct addrinfo *servinfo;
		servinfo = malloc(sizeof(struct addrinfo));
		hints.ai_family = AF_INET;       // IPv4 socket
		hints.ai_socktype = SOCK_STREAM; // Streaming socket.
		// Resolve the addresses
		int rv = 1;
		rv = getaddrinfo(host, port, &hints, &servinfo);
		
		// Check if there was an error and return a failure state.
		if (rv != 0)
				return NULL;

		// Include the address information struct into our socket struct.
		sock->adr = servinfo;
		sock->sa->sa = *servinfo->ai_addr;

		// call the UNIX socket() syscall to acquire a file descriptor.
		// here, we create a IPv4 socket (AF_INET), tell it that we want
		// a streaming socket with dynamic-length packets, and tell it
		// that we're using TCP/IP
		sock->fd  = socket(sock->adr->ai_family, sock->adr->ai_socktype, sock->adr->ai_protocol);

		// Check if the socket failed to be created.
		if (sock->fd == -1)
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
int ConnectSocket(socket_t *sock)
{
		// Make sure someone didn't biff.
		assert(sock);

		// Check to make sure we connected to something.
		int ConnectionSuccessful = 0;

		// Since some hostnames can resolve to multiple addresses (eg, Round-Robin DNS)
		// this for-loop is required to iterate to one which works.
		for (struct addrinfo *adr = sock->adr; adr; adr = adr->ai_next)
		{
				// Here we check if connect failed, if it did, print an error and continue
				// otherwise we set the ConnectionSuccessful variable to indicate we can leave the loop.
				if (connect(sock->fd, adr->ai_addr, adr->ai_addrlen) == -1)
				{
						// Print to stderr instead of stdout for shell-routing reasons.
						fprintf(stderr, "Connection to %s:%hd was unsuccessful: %s (%d)\n", GetIPAddress(adr), sock->port, strerror(errno), errno);
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
				fprintf(stderr, "Failed to find an address to connect to successfully from host %s:%hd\n", sock->host, sock->port);
				return 0;
		}

		return 1;
}

/*******************************************************************
 * Function: DestroySocket                                         *
 *                                                                 *
 * Arguments: socket_t*                                            *
 *                                                                 *
 * Returns: (void) No return.                                      *
 *                                                                 *
 * Description: Destroys the socket structure safely and           *
 * deallocates any resources used by the structure.                *
 *                                                                 *
 *******************************************************************/
void DestroySocket(socket_t *sock)
{
		assert(sock);
		
		// Close the socket so we don't have an untracked file descriptors
		close(sock->fd);

		// Deallocate anything we allocated.
		if (sock->adr)
				freeaddrinfo(sock->adr);

		if (sock->host)
				free(sock->host);

		// Finally, deallocate the socket structure itself.
		free(sock);

		// Remove it out of the vector.
		vec_remove(&sockets, sock);
}

/*******************************************************************
 * Function: ReadSocket                                            *
 *                                                                 *
 * Arguments: socket_t*, void*, size_t                             *
 *                                                                 *
 * Returns: (size_t) Returns the number of bytes read or -1 for an *
 * error status from errno.                                        *
 *                                                                 *
 * Description: Reads data from a socket and fills the buffer,     *
 * once the buffer is filled, it returns the number of bytes read  *
 * from the socket or it returns -1 upon an error condition.       *
 *                                                                 *
 *******************************************************************/
size_t ReadSocket(socket_t *sock, void *buffer, size_t bufferlen)
{
		assert(buffer && sock);

		// Fill the buffer with bytes from the socket
		size_t bytes = read(sock->fd, buffer, bufferlen);
		// Check for errors
		if (bytes == -1UL)
				fprintf(stderr, "Failed to read bytes from socket %d: %s (%d)\n", sock->fd, strerror(errno), errno);

		// Return the number of bytes, if bytes == -1 then we had an error and should
		// handle accordingly in the functions which call this function.
		return bytes;
}

/*******************************************************************
 * Function: WriteSocket                                           *
 *                                                                 *
 * Arguments: socket_t*, const void*, size_t                       *
 *                                                                 *
 * Returns: (size_t) Returns the number of bytes written to the    *
 * socket or -1 for an error condition.                            *
 *                                                                 *
 * Description: The inverse of the `ReadSocket` function in that   *
 * it writes data to the socket. It returns -1 upon failure and    *
 * returns the number of bytes written upon success.               *
 *                                                                 *
 *******************************************************************/
size_t WriteSocket(socket_t *sock, const void *buffer, size_t bufferlen)
{
		assert(sock && buffer);

		// Write the buffer out the socket and return how many bytes we
		// wrote or any error codes if we had an error.
		size_t bytes = send(sock->fd, buffer, bufferlen, 0);
		// error check again
		if (bytes == -1UL)
				fprintf(stderr, "Failed to send bytes to socket %d: %s (%d)\n", sock->fd, strerror(errno), errno);

		return bytes;
}
