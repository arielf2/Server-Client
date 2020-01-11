#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAX_CLIENT_REQUEST_LEN 36 //CLIENT_REQUEST:20\n 36
#define MAX_USERNAME_LENGTH 21
#define MAX_IP_LENGTH 16 // 255.255.255.255, 15 + 1
#define MAX_PORT_LENGTH 5 // 8888, + 1
#define MAX_MOVE_LENGTH 9
#define CLIENT_REQUEST "CLIENT_REQUEST:"
#define SERVER_APPROVED "SERVER_APPROVED\n"
#define SERVER_DENIED "SERVER_DENIED\n"
#define SERVER_MAIN_MENU "SERVER_MAIN_MENU\n"
#define SERVER_PLAYER_MOVE_REQUEST "SERVER_PLAYER_MOVE_REQUEST\n"

#define SERVER_WAIT_TIMEOUT 15
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

/*	Description: Show the main menu to the user and accept the response
	Parameters:	 
	Returns:	integer representing the response */
int ShowMainMenu();


/*	Description: Get the message type and parameters and create a valid message to send to server/client
	Parameters: Dest - pointer to a char pointer, this will hold the final message to send
				Dest is malloced - need to free after using
				message - the message type
				parameter_1/2/3 - given parameters, up to 3. Set unused parameters to NULL.
				num_of_valid_params - number of valid parameters to send. Up to 3
	Returns:	0 if function succeeded, 1 if failed */
int PrepareMessage(char **Dest, char *message, char* parameter_1, char* parameter_2, char* parameter_3, int num_of_valid_params);


/*	Description: Gets the total length of the valid parameters for a specific message
	Parameters:	 parameter_1/2/3 - parameters for the message
				 num_of_valid_params - number of valid params (up to 3)
	Returns		 int - total length (in bytes) of the valid parameters	*/
int GetTotalLen(char* parameter_1, char* parameter_2, char* parameter_3, int num_of_valid_params);


/*	Description: 
	Parameters:
	Returns:	*/
int GameFlow();

/*	Description: Sends the message in the specified format to the destination, through the global socket
	Parameters: message - the message to send
	Returns: 0 if successfull, !0 if failed
*/
int SendMessageToDest(char *message);