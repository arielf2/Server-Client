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


//int parse_command(char *command, parameters_struct* parameters_s) {
//	char s[2] = ":";
//	char delim[2] = ";";
//	int counter = 0;
//	int counter_p = 0;
//	int i = 0;
//	int j = 0;
//	char* parameters_string;
//	char l_string[250] = "";
//	/*count how many parameters*/
//	while (command[i] != '\n') {
//		l_string[i] = command[i];
//		if (command[i] == ';')
//			counter++;
//		if (command[i] == ':')
//			counter_p++;
//		i++;
//	}
//	l_string[i] = '\0';
//	//test
//	counter++;
//	//*parameters = (char*)malloc(counter * sizeof(char*));
//
//	parameters_s->message_type = NULL;
//	parameters_s->param1 = NULL;
//	parameters_s->param2 = NULL;
//	parameters_s->param3 = NULL;
//	parameters_s->param4 = NULL;
//
//	parameters_s->message_type = strtok(l_string, s);
//	if (counter == 1 & counter_p == 1)
//		parameters_s->param1 = strtok(NULL, s);
//	else {
//		parameters_string = strtok(NULL, s);
//		if (counter > 1) {
//			char* param_one = (char*)malloc(40 * sizeof(char*));
//			strcpy(param_one, strtok(parameters_string, delim));
//			//parameters[j] = strtok(parameters_string, delim);
//			//parameters_s->param1 = strtok(parameters_string, delim); //rotem
//			parameters_s->param1 = param_one;
//			if (counter >= 2) {
//
//				char* param_two = (char*)malloc(40 * sizeof(char*));
//				strcpy(param_two, strtok(NULL, delim));
//				parameters_s->param2 = param_two;
//				//parameters_s->param2 = strtok(NULL, delim); //rotem
//			}
//			if (counter >= 3)
//			{
//				char* param_three = (char*)malloc(40 * sizeof(char*));
//				strcpy(param_three, strtok(NULL, delim));
//				parameters_s->param3 = param_three;
//				// parameters_s->param3 = strtok(NULL, delim);// rotem
//			}
//			if (counter == 4) {
//				char* param_four = (char*)malloc(40 * sizeof(char*));
//				strcpy(param_four, strtok(NULL, delim));
//				parameters_s->param4 = param_four;
//				//parameters_s->param4 = strtok(NULL, delim); //rotem
//			}
//		}
//	}
//
//
//	return counter;
//
//}
