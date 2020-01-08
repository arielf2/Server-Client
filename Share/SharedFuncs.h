#pragma once
// This module defines useful functions for the communication protocol

#define MAX_MESSAGE_LEN 40

/*	Description: Gets the number of characters in the message (without the '\n'), where the message is in the specified protocol (ends with '\n')
	Parameters:  char *str: pointer to array of characters
	Returns:	 'length' of the message (number of characters), or error if message is invalid */
int GetLen(char *str);


/*	Description: Compares two messages in the specified protocol
	Parameters:  char *str_a, *str_b: pointers to two arrays of characters that we want to compare 
	Returns:	 0 if the messages are identical */ 
int CompareProtocolMessages(char *str_a, char *str_b);

