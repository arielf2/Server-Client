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
#define CLIENT_APPROVED "CLIENT_APPROVED\n"
#define CLIENT_DENIED "CLIENT_DENIED\n"

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

} client_thread_param;

void MainClient(char* ip_address, char* port, char* username);

static DWORD RecvDataThread(LPVOID lpParam);

static DWORD SendDataThread(LPVOID lpParam);

/*	Description:	Try connecting to the server, with the given ip address and port. If the connection fails,
					the user can decide to exit or try reconnecting.
	Parameters:		server_denied - flag that indicates whether this is the initial connection or another connection
					(like reconnection attempt after receiving server denied message)
					clientService - socket struct 
					ip_address - the IP to which we want to connect
					port - the port to which we want to connet
	Returns:		*/
int TryConnection(int server_denied, SOCKADDR_IN* clientService, char* ip_address, char* port);