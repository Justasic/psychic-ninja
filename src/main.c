// Include some files to interact with the operating system
// and access things like printing to terminal.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Include our socket code to handle TCP/IP data packets.
#include "socket/socket.h"


// The entry point to the application.
int main(int argc, char **argv)
{
	// Initialize the sockets
	InitializeSockets();

	// Create a socket with our host and port we need to connect to
	socket_t *sock = CreateSocket("irc.chatspike.net", "6667");

	// Make sure we got a valid socket, otherwise exit.
	if (!sock)
	{
			fprintf(stderr, "Failed to create to the socket.\n");
			return EXIT_FAILURE;
	}
	
	// Attempt to connect to the socket
	if (!ConnectSocket(sock))
	{
			fprintf(stderr, "Failed to connect to the socket.\n");
			return EXIT_FAILURE;
	}

	// Now we can enter our event-loop and process data
	while (1)
	{

	}

	// Close out any sockets before we exit.
	DestroySockets();
	return EXIT_SUCCESS;
}
