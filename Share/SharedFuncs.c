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


int GetTotalLen(char* parameter_1, char* parameter_2, char* parameter_3, int num_of_valid_params) {
	int len = 0;

	if (num_of_valid_params == 0)
		return 0;
	else if (num_of_valid_params == 1)
		len = strlen(parameter_1);
	else if (num_of_valid_params == 2)
		len = strlen(parameter_1) + strlen(parameter_2);
	else // 3 valid params
		len = strlen(parameter_1) + strlen(parameter_2) + strlen(parameter_3);

	return len;
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



int parse_command(char *command, char* message_type, char* parameters) {
	char s[2] = ":";
	char delim[2] = ";";
	int counter = 0;
	int i = 0;
	int j = 0;
	char* parameters_string;

	/*count how many parameters*/
	while (command[i] != '\n') {
		if (command[i] == ';')
			counter++;
		i++;
	}
	counter++;

	parameters = (char*)malloc(counter * sizeof(char*));
	
	message_type = strtok(command, s);

	parameters_string = strtok(NULL, s);

	parameters = strtok(parameters_string, delim);
	
	for (j = 1; j < counter; j++) {
		parameters = strtok(NULL, delim);
	}
	return counter;


}


//int WaitForMessage(char **AcceptedString, int wait_period, SOCKET *local_socket) {
//	int error = 0;
//
//	fd_set set;
//	struct timeval time;
//	time.tv_sec = wait_period;
//	time.tv_usec = 0;
//
//	FD_ZERO(&set);
//	FD_SET(local_socket, &set);
//
//	//char *AcceptedStr = NULL;
//	TransferResult_t RecvRes;
//
//	error = select(0, &set, NULL, NULL, &time);
//
//	if (error == 0) {
//		printf("TIMEOUT: %d seconds passed and no response from server, in WaitForMessage, ReceiveData client\n", wait_period);
//		return TIMEOUT_ERROR;
//	}
//	RecvRes = ReceiveString(AcceptedString, m_socket);
//
//	if (RecvRes == TRNS_FAILED)
//	{
//		printf("Socket error while trying to write data to socket\n");
//		return 0x555;
//	}
//	else if (RecvRes == TRNS_DISCONNECTED)
//	{
//		printf("Server closed connection. Bye!\n");
//		return 0x555;
//	}
//	else
//	{
//		//printf("accepted string is: %s\n", *AcceptedString);
//		//return error;
//	}
//
//	//free(AcceptedStr);
//	//return error;
//
//}


