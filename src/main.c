// Include some files to interact with the operating system
// and access things like printing to terminal.
#include <stdio.h>
#include <unistd.h>

// Include our socket code to handle TCP/IP data packets.
#include "socket/socket.h"


// The entry point to the application.
int main(int argc, char **argv)
{
	// Initialize the sockets
	InitializeSockets();


	// Close out any sockets before we exit.
	DestroySockets();
	return EXIT_SUCCESS;
}
