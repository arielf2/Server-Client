
#include "Client.h"
#include "SendReceive.h"

// Global Socket
SOCKET m_socket;

void MainClient(char* ip_address, char* port, char* username)
{
	
	SOCKADDR_IN clientService;
	HANDLE hThread[2];
	client_thread_param thread_param;
	strcpy_s(thread_param.username, MAX_USERNAME_LENGTH, username);

	// Initialize Winsock.
	WSADATA wsaData;

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//Call the socket function and return its value to the m_socket variable. 
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
	printf("Trying to connect to port %d\n", clientService.sin_port);
	while (connect(m_socket, (SOCKADDR*)& clientService, sizeof(clientService)) == SOCKET_ERROR) {
		//(connect(m_socket, (SOCKADDR*)& clientService, sizeof(clientService)) == SOCKET_ERROR) {
		printf("Failed connection, error %ld\n", WSAGetLastError());
		printf("Failed connecting to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
		char user_resp[2];
		gets_s(user_resp, 2);

		if (strcmp(user_resp, "2") == 0) {
			WSACleanup();
			//return; 
			break; //break just so we can finish the program. Currently can't connect to server. Change back to return
		}
	}

	/* If we are here - we managed to connect to the server */

	hThread[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendDataThread, &thread_param, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvDataThread, NULL, 0, NULL);

	//concat client request to username
	

	// Send client request
	
	//strcpy_s(cl_req, MAX_CLIENT_REQUEST_LEN + 1, "CLIENT_REQUEST:")
}

static DWORD RecvDataThread(LPVOID lpParam)
{

	
	TransferResult_t RecvRes;


	while (1)
	{
		char *AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (RecvRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			printf("Server closed connection. Bye!\n");
			return 0x555;
		}
		else
		{
			printf("%s\n", AcceptedStr);
		}

		free(AcceptedStr);
	}

	return 0;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

//Sending data to the server
static DWORD SendDataThread(LPVOID lpParam)
{

	if (NULL == lpParam) {
		printf("Received bad parameters in Client Send Data Thread");
		exit(1);
	}

	client_thread_param *client_params = (client_thread_param *)lpParam;
	

	char ClientRequest[MAX_CLIENT_REQUEST_LEN + 1] = CLIENT_REQUEST;  // CLIENT_REQUEST:

	strcat_s(ClientRequest, MAX_CLIENT_REQUEST_LEN + 1, client_params->username); //CLIENT_REQUEST:Ariel

	ClientRequest[strlen(ClientRequest)] = '\n'; // Turn the string to the correct protocol message, that ends with '\n' instead of '\0'
	
	printf("length of str is %d", GetLen(ClientRequest));
	printf("\nCompare %d", CompareProtocolMessages(ClientRequest, "IAMBCD\n"));

	char SendStr[256];
	TransferResult_t SendRes;

	while (0)
	{
		//gets_s(SendStr, sizeof(SendStr)); //Reading a string from the keyboard

		//if (STRINGS_ARE_EQUAL(SendStr, "quit"))
			//return 0x555; //"quit" signals an exit from the client side

		SendRes = SendString(SendStr, m_socket);

		if (SendRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
	}
}