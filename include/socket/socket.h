#pragma once
#include <stdint.h> // For size_t
#include <sys/types.h> // for struct addrinfo

// Forward-declare our sockaddr_t union, the linker
// will resolve this reference later.
typedef union sockaddr_t;

typedef struct
{
		int fd;         // This is the file descriptor used to associate this socket with our applcation inside the kernel.
		short int port; // This is the port we're connecting on.
		char *host;     // the DNS hostname given to us from the user.
		char *ip;       // The IP address from the resolved hostname above.
		
		// Internal structures
		sockaddr_t *sa; // The internal socket address structures.
		struct addrinfo *adr; // The address info of the socket in binary form.
} socket_t;

// Forward declare our functions for use outside the file
extern int InitializeSockets(void);
extern int DestroySockets(void);
extern socket_t *CreateSocket(const char *host, const char *port);
extern int ConnectSocket(socket_t *sock);
extern void DestroySocket(socket_t *sock);
extern size_t ReadSocket(socket_t *sock, void *buffer, size_t bufferlen);
extern size_t WriteSocket(socket_t *sock, const void *buffer, size_t bufferlen);
