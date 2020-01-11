#include "Header.h"
#include "Client.h"


int main(int argc, char* argv[]) {

	char username[MAX_USERNAME_LENGTH], port[MAX_PORT_LENGTH], ip_address[MAX_IP_LENGTH];
	if (ParseCommand(ip_address, port, username, argv)) {
		return 1;
	}
	
	MainClient(ip_address, port, username);

}


int ParseCommand(char* ip_address, char* port, char* username, char* arguments[]) {

	int err = 0;

	if (strcpy_s(ip_address, MAX_IP_LENGTH, arguments[1])) {
		printf("Error while receiving ip address from command line\n");
		err = 1;
	}
	if (strcpy_s(port, MAX_PORT_LENGTH, arguments[2])) {
		printf("Error while receiving port from command line\n");
		err = 1;
	}
	if (strcpy_s(username, MAX_USERNAME_LENGTH, arguments[3])) {
		printf("Error while receiving username from command line\n");
		err = 1;
	}
	return err;

}