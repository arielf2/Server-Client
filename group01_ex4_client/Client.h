#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAX_CLIENT_REQUEST_LEN 36 //CLIENT_REQUEST:20\n 36
#define MAX_USERNAME_LENGTH 21
#define CLIENT_REQUEST "CLIENT_REQUEST:"

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "SharedFuncs.h"
#pragma comment(lib, "Ws2_32.lib")

typedef struct {
	char username[MAX_USERNAME_LENGTH];
} client_thread_param;

void MainClient(char* ip_address, char* port, char* username);

static DWORD RecvDataThread(LPVOID lpParam);
static DWORD SendDataThread(LPVOID lpParam);