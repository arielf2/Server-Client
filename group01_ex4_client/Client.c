
#include "Client.h"

// Global Socket
SOCKET m_socket;

void MainClient(char* ip_address, char* port, char* username)
{
	SOCKADDR_IN clientService;
	HANDLE hThread[2];

	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//Call the socket function and return its value to the m_socket variable. 
	// For this application, use the Internet address family, streaming sockets, and the TCP/IP protocol.

	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}
	/*
	 The parameters passed to the socket function can be changed for different implementations.
	 Error detection is a key part of successful networking code.
	 If the socket call fails, it returns INVALID_SOCKET.
	 The if statement in the previous code is used to catch any errors that may have occurred while creating
	 the socket. WSAGetLastError returns an error number associated with the last error that occurred.
	 */


	 //For a client to communicate on a network, it must connect to a server.
	 // Connect to a server.

	 //Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip_address); //Setting the IP address to connect to
	clientService.sin_port = htons((u_short)port); //Setting the port to connect to.

	/*
		AF_INET is the Internet address family.
	*/


	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	while (connect(m_socket, (SOCKADDR*)& clientService, sizeof(clientService)) == SOCKET_ERROR) {
		//(connect(m_socket, (SOCKADDR*)& clientService, sizeof(clientService)) == SOCKET_ERROR) {
		printf("Failed connecting to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
		char user_resp[2];
		gets_s(user_resp, 2);

		if (strcmp(user_resp, "2") == 0) {
			WSACleanup();
			return;
		}
	}

	
}

