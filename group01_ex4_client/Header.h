#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERNAME_LENGTH 21  // 20 + \0
#define MAX_IP_LENGTH 16 // 255.255.255.255, 15 + 1
#define MAX_PORT_LENGTH 5 // 8888, + 1

/*	Description: Initial parser for the command line arguments
	Parameters:  ip_address: string representing the ip address, to be updated in the function
				 port: string representing the port, to be updated in the function
				 username: string representing the ip username, to be updated in the function
				 arguments: array of strings: the actual command line arguments
	Returns:	 error code: 1 upon succces, 0 upon failure */

int ParseCommand(char* ip_address, char* port, char* username, char* arguments[]);
