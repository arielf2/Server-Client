#pragma once
// This module defines useful functions for the communication protocol
#include <stdlib.h>	

#define MAX_MESSAGE_LEN 40


/*	Description: Gets the number of characters in the message (without the '\n'), where the message is in the specified protocol (ends with '\n')
	Parameters:  char *str: pointer to array of characters
	Returns:	 'length' of the message (number of characters), or error if message is invalid */
int GetLen(char *str);


/*	Description: Compares two messages in the specified protocol
	Parameters:  char *str_a, *str_b: pointers to two arrays of characters that we want to compare 
	Returns:	 0 if the messages are identical */ 
int CompareProtocolMessages(char *str_a, char *str_b);




/* Functions that can be probably used by both the server and the client */

/*	Description: Get the message type and parameters and create a valid message to send to server/client
	Parameters: Dest - pointer to a char pointer, this will hold the final message to send
				Dest is malloced - need to free after using
				message - the message type
				parameter_1/2/3 - given parameters, up to 3. Set unused parameters to NULL.
				num_of_valid_params - number of valid parameters to send. Up to 3
	Returns:	0 if function succeeded, 1 if failed
	Usage example: 

	char *PlayerMove = NULL;
	PrepareMessage(&PlayerMove, "CLIENT_PLAYER_MOVE", "PAPER", NULL, NULL, 1);
	*/


int PrepareMessage(char **Dest, char *message, char* parameter_1, char* parameter_2, char* parameter_3, int num_of_valid_params);


int GetTotalLen(char* parameter_1, char* parameter_2, char* parameter_3, int num_of_valid_params);