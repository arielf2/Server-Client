#include "Header.h"
#include "Client.h"


int main(int argc, char* argv[]) {

	char username[MAX_USERNAME_LENGTH], port[MAX_PORT_LENGTH], ip_address[MAX_IP_LENGTH];
	int client_ret_val = 0;
	if (argc != 4) {
		printf("The client program requires 3 user arguments: ip_address, port, username");
		return 1;
	}
	if (ParseCommand(ip_address, port, username, argv)) {
		return 1;
	}
	return MainClient(ip_address, port, username);
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