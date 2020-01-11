#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAX_CLIENT_REQUEST_LEN 36 //CLIENT_REQUEST:20\n 36
#define MAX_USERNAME_LENGTH 21
#define MAX_IP_LENGTH 16 // 255.255.255.255, 15 + 1
#define MAX_PORT_LENGTH 5 // 8888, + 1
#define CLIENT_REQUEST "CLIENT_REQUEST:"
#define SERVER_APPROVED "SERVER_APPROVED\n"
#define SERVER_DENIED "SERVER_DENIED\n"
#define SERVER_MAIN_MENU "SERVER_MAIN_MENU\n"


#define SERVER_WAIT_TIMEOUT 15
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "SharedFuncs.h"
#pragma comment(lib, "Ws2_32.lib")

typedef struct {
	char username[MAX_USERNAME_LENGTH];
	char ip_address[MAX_IP_LENGTH];
	char port[MAX_PORT_LENGTH];

	SOCKADDR_IN *clientService;
} client_thread_param;

void MainClient(char* ip_address, char* port, char* username);

static DWORD RecvDataThread(LPVOID lpParam);

static DWORD SendDataThread(LPVOID lpParam);

/*	Description:	Show The reconnection menu to the user, and display the correct error according to the reason for reconnection
	Parameters:		reason_for_connect - 0: initial connection
										 1: server denied -> reconnection
										 2: timeout connection lost -> reconnection
					ip_address - the IP to which we want to connect
					port - the port to which we want to connet
	Returns:		user decision - 2 if user asked to exit, 1 to reconnect  */
int ReconnectMenu(int reason_for_connect, char* ip_address, char* port);

/*	Description: Wait for the server to send a message. TIMEOUT if no response within 15 seconds
	Parameters:  AcceptedString - pointer to a char pointer, that will eventually point to the received string
	Returns:	 error code	*/
int WaitForMessage(char **AcceptedString);


/*	Description: Send the client request to the server, with the name of the user
	Parameters:	username - name of the user, as received from the command line
	Returns: 0 upon successfull send, error otherwise */
int SendClientRequest(char *username);

int CreateAndCheckSocket();

/*	Description: After receiving a response from the server, check which type of message it is.
	Parameters:	 reponse - the string received from the the server
	Returns:	 integer representing the message	*/
int CheckServerResponse(char* response);
//	//if (response == NULL) {
//	//	return 1;
//	//}
//	if (CompareProtocolMessages(response, SERVER_APPROVED) == 0) {
//		return 0;
//	}
//	else if (CompareProtocolMessages(response, SERVER_DENIED) == 0) {
//		return 1;
//	}
//	else if (CompareProtocolMessages(response, SERVER_MAIN_MENU) == 0) {
//		return 2;
//	}
//
//	free(response);
//	return 0;
//}