
#include "Client.h"
#include "SendReceive.h"

// Global Socket
SOCKET m_socket;

void MainClient(char* ip_address, char* port, char* username)
{
	SOCKADDR_IN clientService;
	//HANDLE hThread[2];

	int user_exit = 0;
	// updating the client struct
	client_thread_param thread_param;

	strcpy_s(thread_param.username, MAX_USERNAME_LENGTH, username);
	strcpy_s(thread_param.ip_address, MAX_IP_LENGTH, ip_address);
	strcpy_s(thread_param.port, MAX_PORT_LENGTH, port);
	thread_param.clientService = &clientService;
	// Initialize Winsock.
	WSADATA wsaData;

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//Call the socket function and return its value to the m_socket variable. 
	// Create a socket.

	CreateAndCheckSocket();
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

	int wait = 0;
	int server_resp = 0;
	int connect_res = 0;

	while (1) {
		//try connection
		connect_res = connect(m_socket, (SOCKADDR*)& clientService, sizeof(clientService));
		if (connect_res == SOCKET_ERROR) {
			printf("Failed connection, error %ld\n", WSAGetLastError());
			if (ReconnectMenu(0, ip_address, port) == 1) //try reconnecting
				continue;
			else {
				WSACleanup();
				break;
			} // user wants to exit
		}
		else { 	//connection successfull:
			//send client request
			if (SendClientRequest(username) != 0) {
				printf("couldnt write client request to socket\n");
				exit(1);
			}
			//else - successful write to socket, get server response.

			//Approve/Deny Client

			char *AcceptedStr = NULL;
			wait = WaitForMessage(&AcceptedStr);
			
			if (wait == 1) { // TIMEOUT - show menu
				closesocket(m_socket); //disconnect from server
				if (ReconnectMenu(1, ip_address, port) == 1) {
					m_socket = CreateAndCheckSocket();
					continue;
				}
				//else - got 2 exit from user. 
				printf("Bye\n");
				return;
			}

			//else - wait = 0 so we received a valid message

			server_resp = CheckServerResponse(AcceptedStr);
			if (server_resp == 1) { //server denied - show menu
				closesocket(m_socket); //disconnect from server
				if (ReconnectMenu(2, ip_address, port) == 1) {
					m_socket = CreateAndCheckSocket();
					continue;
				}
				//else - got 2 exit from user
				printf("Bye\n");
				return;
			}

			else { //server resp = 0 - server approved
				if (GameFlow() == 1) { // TIMEOUT during game
					closesocket(m_socket); //disconnect from server
					if (ReconnectMenu(1, ip_address, port) == 1) {
						m_socket = CreateAndCheckSocket();
						continue;
					}
					//else - got 2 exit from user. 
					printf("Bye\n");
					return;
				}
				
			}
									
		}			
	}
	//user_exit = ReconnectMenu(0, &clientService, ip_address, port);
}

//static DWORD RecvDataThread(LPVOID lpParam)
//{
//
//	if (NULL == lpParam) {
//		printf("Received bad parameters in Client Recv Data Thread");
//		exit(1);
//	}
//
//	client_thread_param *client_params = (client_thread_param *)lpParam;
//	TransferResult_t RecvRes;
//	int wait = 0;
//
//	//Approve/Deny Client
//	char *AcceptedStr = NULL;
//	wait = WaitForMessage(&AcceptedStr);
//
//	if (wait == 1) // TIMEOUT - try to reconnect
//	{
//		ReconnectMenu(2, client_params->clientService, client_params->ip_address, client_params->port);
//	}
//	//RecvRes = ReceiveString(&AcceptedStr, m_socket);
//	if (CompareProtocolMessages(AcceptedStr, SERVER_APPROVED) == 0) {
//		// wait for main menu message from server
//	}
//	else if (CompareProtocolMessages(AcceptedStr, SERVER_DENIED) == 0) {
//		ReconnectMenu(1, client_params->clientService, client_params->ip_address, client_params->port);
//	}
//
//	free(AcceptedStr);
//
//
//
//	while (1)
//	{
//		char *AcceptedStr = NULL;
//		RecvRes = ReceiveString(&AcceptedStr, m_socket);
//
//		if (RecvRes == TRNS_FAILED)
//		{
//			printf("Socket error while trying to write data to socket\n");
//			return 0x555;
//		}
//		else if (RecvRes == TRNS_DISCONNECTED)
//		{
//			printf("Server closed connection. Bye!\n");
//			return 0x555;
//		}
//		else
//		{
//			printf("%s\n", AcceptedStr);
//		}
//
//		free(AcceptedStr);
//	}
//
//	return 0;
//}
//
///*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
//
////Sending data to the server
//static DWORD SendDataThread(LPVOID lpParam)
//{
//
//	if (NULL == lpParam) {
//		printf("Received bad parameters in Client Send Data Thread");
//		exit(1);
//	}
//
//	client_thread_param *client_params = (client_thread_param *)lpParam;
//	
//	//Client Request
//	char ClientRequest[MAX_CLIENT_REQUEST_LEN + 1] = CLIENT_REQUEST;  // CLIENT_REQUEST:
//	strcat_s(ClientRequest, MAX_CLIENT_REQUEST_LEN + 1, client_params->username); //CLIENT_REQUEST:Ariel
//	ClientRequest[strlen(ClientRequest)] = '\n'; // Turn the string to the correct protocol message, that ends with '\n' instead of '\0'
//	
//	//printf("length of str is %d", GetLen(ClientRequest));
//	//printf("\nCompare %d", CompareProtocolMessages(ClientRequest, "IAMBCD\n"));
//
//	char SendStr[256];
//	TransferResult_t SendRes;
//
//	while (0)
//	{
//		//gets_s(SendStr, sizeof(SendStr)); //Reading a string from the keyboard
//
//		//if (STRINGS_ARE_EQUAL(SendStr, "quit"))
//			//return 0x555; //"quit" signals an exit from the client side
//
//		SendRes = SendString(SendStr, m_socket);
//
//		if (SendRes == TRNS_FAILED)
//		{
//			printf("Socket error while trying to write data to socket\n");
//			return 0x555;
//		}
//	}
//}
//
int ReconnectMenu(int reason_for_connect, char* ip_address, char* port) {
	int user_decision = 0;
	long int user_dec = 0;
	char user_resp[2];
start:

	if (reason_for_connect == 0) // this is a first-time connection {
		printf("Failed connecting to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
	else if (reason_for_connect == 1) // reason_for_connect == 1 - server denied
		printf("Server on %s:%s denied the connection request.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
	else //reason_for_connect = 2 - timeout or connection lost
		printf("Connection to server on %s:%s has been lost.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
	gets_s(user_resp, 2);

	user_dec = strtol(user_resp, NULL, 10);
	if (user_dec != 1 && user_dec != 2)
		goto start;
	//if (strcmp(user_resp, "2") == 0) {
	//	WSACleanup();			//return; 
	//	user_decision = 2;
	//}
	//else if (strcmp(user_resp, "1") == 0) //user decision = 1 - try to reconnect
	//	user_decision = 1;
	else {
		//goto start;
		return user_dec;
	}

	//return user_decision;
}

int WaitForMessage(char **AcceptedString) {
	int error = 0;

	fd_set set;
	struct timeval time;
	time.tv_sec = SERVER_WAIT_TIMEOUT;
	time.tv_usec = 0;

	FD_ZERO(&set);
	FD_SET(m_socket, &set);

	//char *AcceptedStr = NULL;
	TransferResult_t RecvRes;

	error = select(0, &set, NULL, NULL, &time);

	if (error == 0) {
		printf("TIMEOUT: 15 seconds passed and no response from server, in WaitForMessage, ReceiveData client\n");
		return 1;
	}
	RecvRes = ReceiveString(AcceptedString, m_socket);

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
		printf("accepted string is: %s\n", *AcceptedString);
	}

	//free(AcceptedStr);

}

int SendClientRequest(char *username) {
	char ClientRequest[MAX_CLIENT_REQUEST_LEN + 1] = CLIENT_REQUEST;  // CLIENT_REQUEST:
	TransferResult_t SendRes;

	strcat_s(ClientRequest, MAX_CLIENT_REQUEST_LEN + 1, username); //CLIENT_REQUEST:Ariel
	ClientRequest[strlen(ClientRequest)] = '\n'; // Turn the string to the correct protocol message, that ends with '\n' instead of '\0'

	SendRes = SendString(ClientRequest, m_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}

	//else - success, return 0
	return 0;
}

int CreateAndCheckSocket() {
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	return 0;
}

int CheckServerResponse(char* response) {
	//if (response == NULL) {
	//	return 1;
	//}

	int resp = -1;
	if (CompareProtocolMessages(response, SERVER_APPROVED) == 0) {
		resp = 0;
	}
	else if (CompareProtocolMessages(response, SERVER_DENIED) == 0) {
		resp = 1;
	}
	else if (CompareProtocolMessages(response, SERVER_MAIN_MENU) == 0) {
		resp = 2;
	}

	free(response);
	return resp;
}


int GameFlow() {
	int wait = 0;
	int server_response = 0;
	int user_response = 0;
	// wait for server main menu message
	// get user response from:
	//1 Play against another client 
	//2 Play against the server
	//3 View the leaderboard
	//4 Quit
	char *AcceptedStr = NULL;
	wait = WaitForMessage(&AcceptedStr);
	if (wait == 1) {
		return 1; //go back to main while loop, disconnect and show initial menu to user
	}
	server_response = CheckServerResponse(AcceptedStr);
	if (server_response != 2) {
		printf("Debug Print:\nServer Didn't send SERVER_MAIN_MENU\n");
		//error?
	}
	//else - server response is 2 - main m enu, show main menu to user
	user_response = ShowMainMenu();
	if (user_response == 1) {
		//play against another client
	}
	else if (user_response == 2) {
		//play against server
	}
	else if (user_response == 3) {
		// view leader board
	}
	else { //user response == 4 {

	}

}

int ShowMainMenu() {
	long int user_decision = 0;
	char user_resp[2];
start:
	printf("Choose what to do next:\n1. Play against another client\n2. Play against the server\n3. View the leaderboard\n4.Quit");
	gets_s(user_resp, 2);
	user_decision = strtol(user_resp, NULL, 10);
	if (user_decision != 1 && user_decision != 2 && user_decision != 3 && user_decision != 4)
		goto start;
	return user_decision;
}
