#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAX_CLIENT_REQUEST_LEN 36 //CLIENT_REQUEST:20\n 36

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "SharedFuncs.h"
#pragma comment(lib, "Ws2_32.lib")

void MainClient(char* ip_address, char* port, char* username);
