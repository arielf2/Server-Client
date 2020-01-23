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


int PrepareMessage(char **Dest, char *message, char* parameter_1, char* parameter_2, char* parameter_3, int num_of_valid_params) {

	if (*Dest != NULL) {
		printf("Debug Print:\n Dest has to be pointer to null, so it can be malloced\n");
		return 1;
	}
	int len = 0;
	//len(message) + ':' = len(message) + 1
	//number of ; = num of valid params - 1
	//len of params:
	// + 1 for \n 
	len = GetTotalLen(parameter_1, parameter_2, parameter_3, num_of_valid_params) + strlen(message) + 1 + (num_of_valid_params - 1) + 2;

	*Dest = (char*)malloc(len * sizeof(char));
	if (*Dest == NULL) {
		printf("Error in allocating memory\n");
		return 1;
	}

	strcpy_s(*Dest, len, message); //THIS_IS_MESSAGE
	//strcat_s(*Dest, len, ":");
	if (num_of_valid_params != 0) {
		strcat_s(*Dest, len, ":");
		// at least one valid parameter
		strcat_s(*Dest, len, (const char*)parameter_1);
		if (num_of_valid_params >= 2) {
			strcat_s(*Dest, len, ";");
			strcat_s(*Dest, len, (const char*)parameter_2);
			if (num_of_valid_params == 3) {
				strcat_s(*Dest, len, ";");
				strcat_s(*Dest, len, (const char*)parameter_3);
			}
		}
	}
	//printf("\nMessage is: %s\n", *Dest);
	*(*Dest + strlen(*Dest)) = '\n'; //Messages need to end with '\n'

	return 0;
}


//int SendMessageToDest(char *message, SOCKET *local_socket) {
//	int return_val = 0;
//	TransferResult_t SendRes;
//
//	SendRes = SendString(message, *local_socket);
//	if (SendRes == TRNS_FAILED)
//	{
//		printf("Socket error while trying to write data to socket\n");
//		return_val = 0x555;
//	}
//	free(message);
//	return return_val;
//}