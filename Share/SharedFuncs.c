// This module defines useful functions for the communication protocol
#include "SharedFuncs.h"

int GetLen(char *str) {
	
	int i = 0;
	if (NULL == str) {
		printf("GetLen Received a Null pointer");
		return 0;
	}
	while (*str != '\n' && i < MAX_MESSAGE_LEN) {
		i++;
		str++;
	}
	return (i == MAX_MESSAGE_LEN) ? 0: i; //return 0 value to indicate error, if the length of the message is too long

}


int CompareProtocolMessages(char *str_a, char *str_b) {
	
	int i = 0;
	int len_a = GetLen(str_a);
	int len_b = GetLen(str_b);
	if (len_a == 0 || len_b == 0) {
		//one of the strings is either empty or too long
		printf("Error in one of the compared strings\n");
		return 100; //some error code
	}

	if (len_a != len_b) {
		return 1; //strings are different
	}

	while (*str_a != '\n' && *str_b != '\n') {
		if (*str_a != *str_b) {
			//strings are different
			return 1;
			break;
		}
		str_a++;
		str_b++;
	}

	return 0;

}


