
#include "Header.h"





HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];


int main(int argc, char *argv[]) {
	int Ind;
	int Loop;
	int return_val = -1;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	HANDLE game_session_file_mutex = NULL;
	HANDLE exit_thread = NULL;
	// Initialize Winsock.
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (StartupRes != NO_ERROR)
	{
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return 1;
	}

	/* The WinSock DLL is acceptable. Proceed. */

	// Create a socket.    
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (MainSocket == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
	}

	// Bind the socket.
	/*
		For a server to accept client connections, it must be bound to a network address within the system.
		The following code demonstrates how to bind a socket that has already been created to an IP address
		and port.
		Client applications use the IP address and port to connect to the host network.
		The sockaddr structure holds information regarding the address family, IP address, and port number.
		sockaddr_in is a subset of sockaddr and is used for IP version 4 applications.
   */
   // Create a sockaddr_in object and set its values.
   // Declare variables

	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		goto server_cleanup_2;
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(*argv[1]); //The htons function converts a u_short from host to TCP/IP network byte order 
									   //( which is big-endian ).
	/*
		The three lines following the declaration of sockaddr_in service are used to set up
		the sockaddr structure:
		AF_INET is the Internet address family.
		"127.0.0.1" is the local IP address to which the socket will be bound.
		2345 is the port number to which the socket will be bound.
	*/

	// Call the bind function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
	game_session_file_mutex = CreateMutex(NULL, FALSE, "game_session_file_mutex");
	if (game_session_file_mutex == NULL)
		printf("Error while creating game_session_file_mutex\n");

	/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
	return_val = create_leader_board();
	if (return_val != 0)
		printf("Error while creating leader_board_file\n");
	/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
	exit_thread = CreateThreadSimple()

	/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

	// Listen on the Socket.
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;

	printf("Waiting for a client to connect...\n");

	for (Loop = 0; Loop < MAX_LOOPS; Loop++)
	{
		SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			goto server_cleanup_3;
		}

		printf("Client Connected.\n");

		Ind = FindFirstUnusedThreadSlot();

		if (Ind == NUM_OF_WORKER_THREADS) //no slot is available
		{
			printf("No slots available for client, dropping the connection.\n");
			closesocket(AcceptSocket); //Closing the socket, dropping the connection.
		}
		else
		{
			ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
											  // AcceptSocket, instead close 
											  // ThreadInputs[Ind] when the
											  // time comes.
			ThreadHandles[Ind] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ServiceThread,
				&(ThreadInputs[Ind]),
				0,
				NULL
			);
		}
	} // for ( Loop = 0; Loop < MAX_LOOPS; Loop++ )

server_cleanup_3:

	CleanupWorkerThreads();

server_cleanup_2:
	if (closesocket(MainSocket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());

server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());

	return 0;
}


static int FindFirstUnusedThreadSlot()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}


static void CleanupWorkerThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], INFINITE);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(ThreadInputs[Ind]);
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
			else
			{
				printf("Waiting for thread failed. Ending program\n");
				return;
			}
		}
	}
}


//Service thread is the thread that opens for each successful client connection and "talks" to the client.
static DWORD ServiceThread(SOCKET *t_socket)
{
	char SendStr[SEND_STR_SIZE];
	int find_other_player = -1;
	BOOL Done = FALSE;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	char message_type[15] = "";
	char* parameters = NULL;
	char* user_name = NULL;
	step step = -1;
	char step_c[9] = "";
	status status = -1;

	strcpy(SendStr, "SERVER_APPROVED");

	SendRes = SendString(SendStr, *t_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}

	strcpy(SendStr, "SERVER_MAIN_MENU");

	SendRes = SendString(SendStr, *t_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}
	while (!Done)
	{
		char *AcceptedStr = NULL;

		RecvRes = ReceiveString(&AcceptedStr, *t_socket);

		if (RecvRes == TRNS_FAILED)
		{
			printf("Service socket error while reading, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			printf("Connection closed while reading, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
		else
		{
			printf("Got string : %s\n", AcceptedStr);
		}

		parse_command(AcceptedStr, message_type, parameters);

		if (STRINGS_ARE_EQUAL(message_type, "CLIENT_REQUEST")) {

			user_name = parameters[0];
			strcpy(SendStr, "SERVER_APPROVED");
			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_MAIN_MENU"))
		{
			continue;
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_CPU")) {
			status = CPU;
			step = rand_step();
			strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST");
			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
			
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_VERSUS")) {
			status = VERSUS;
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_LEADERBOARD")) {
			SendRes = send_leader_board(*t_socket);
			if (SendRes == TRNS_FAILED)
				{
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
				}
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_PLAYER_MOVE")) {
			if (status = CPU) {
				int parameter_0 = parameters[0] - '0';
				int winner = -1;
				winner = find_who_wins(step, parameter_0);
				if (winner == 0) {
					replace_enum_with_string(step, step_c);
					sprintf(SendStr, "SERVER_GAME_RESULTS:server;%s;%s,server\n", step_c, parameters[0]);
				}
				else if (winner == 1) {
					replace_enum_with_string(step, step_c);
					sprintf(SendStr, "SERVER_GAME_RESULTS:server;%s;%s,%s\n", step_c, parameters[0], user_name);
				}
				else 
					printf("Error in player move\n");

			}
			else if (status = VERSUS) {
				/*send results when playing against other player */
			}

			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_REPLAY")) {
			if (status == CPU) {
				step = rand_step();
				strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST");
				SendRes = SendString(SendStr, *t_socket);
			}
			else if (status == VERSUS) {

			}
			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_REFRESH")) {
			/*bonus*/
		}
		else if (STRINGS_ARE_EQUAL(message_type, "CLIENT_DISCONNECT"))
		{
			return 0;
		}
		else
		{
			strcpy(SendStr, "I don't understand");
		}

		SendRes = SendString(SendStr, *t_socket);

		if (SendRes == TRNS_FAILED)
		{
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}

		free(AcceptedStr);
	}

	printf("Conversation ended.\n");
	closesocket(*t_socket);
	return 0;
}

int find_other_player() {
	FILE* fp;
	int l_wait_code = -1;
	int l_ret_val = -1;
	HANDLE l_mutex_handle = NULL;
	l_mutex_handle = OpenMutex(SYNCHRONIZE, TRUE, "game_session_file_mutex");

	l_wait_code = WaitForSingleObject(l_mutex_handle, INFINITE);
	if (WAIT_OBJECT_0 != l_wait_code)
	{
		printf("Error when waiting for mutex\n");

	}

	fopen_s(&fp, "GameSession.txt", "r");
	l_ret_val = ReleaseMutex(l_mutex_handle);
	if (FALSE == l_ret_val)
	{
		printf("Error when releasing game_session_file_mutex\n");
	}

	if (fp) {
		fclose(fp);
		return 1;
	}
	else
		return 0;
}
void write_move_to_file(char *move) {
	FILE* fp;
	int l_wait_code = -1;
	int l_ret_val = -1;
	HANDLE l_mutex_handle = NULL;
	l_mutex_handle = OpenMutex(SYNCHRONIZE, TRUE, "game_session_file_mutex");

	l_wait_code = WaitForSingleObject(l_mutex_handle, INFINITE);
	if (WAIT_OBJECT_0 != l_wait_code)
	{
		printf("Error when waiting for mutex\n");

	}

	fopen_s(&fp, "GameSession.txt", "a");
	fputs(move, fp);
	l_ret_val = ReleaseMutex(l_mutex_handle);
	if (FALSE == l_ret_val)
	{
		printf("Error when releasing game_session_file_mutex\n");
	}
	fclose(fp);
}

int write_in_leader_board(char user_name[], int win) {
	FILE* fp;
	FILE* w_fp;
	char s[2] = ",";
	char start[5] = "Name";
	char line[255];//fix
	char* token;
	char* won;
	char* lost;
	char* ratio;
	int ratio_i = 0;
	fopen_s(&fp, "LeaderBoard.csv", "r");
	while (!feof(fp)) {
		fgets(line, 255, fp);
		token = strtok(line, s);
		if (strcmp(start, token) != 0) {//verfiy there cant be user with name "name" 

			if (strcmp(token, user_name) == 0) {
				won = strtok(NULL, s);
				lost = strtok(NULL, s);
				ratio = strtok(NULL, s);
				if (win == 1)
					*won = *won + 1;
				else
					*lost = *lost + 1;
				ratio_i = atoi(won) / atoi(lost);
				w_fp = fp;

				fprintf(w_fp, "%s,%s,%s, %d\n", user_name, won, lost, ratio_i);
				w_fp = NULL;

			}
		}
	}
	return 0;
}

int send_leader_board(SOCKET *t_socket) {
	FILE* fp;
	char SendStr[SEND_STR_SIZE];
	char line_for_send[255] = "";
	TransferResult_t SendRes;
	fopen_s(&fp, "LeaderBoard.csv", "r");

	while (feof(fp)) {

		fgets(line_for_send, 255, fp);
		replace_comma_with_tab(line_for_send, SendStr);
		SendRes = SendString(SendStr, *t_socket);

		if (SendRes == TRNS_FAILED)
		{
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*t_socket);
			return SendRes;
		}
	}
	return SendRes;
}

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
	}
	counter++;
	parameters = (char*)malloc(counter * sizeof(char*));
	

	message_type = strtok(command, s);
	parameters_string = strtok(NULL, s);
	parameters[j] = strtok(parameters_string, delim);
	for (j = 1; j < counter - 1; j++) {
		parameters[j] = strtok(NULL, delim);
	}
	return counter;
	

}
int create_leader_board() {
	FILE* fp;
	int return_val = -1;
	return_val = fopen_s(&fp, "LeaderBoard.csv", "w");
	if (return_val != 0)
		return 1;
	fprintf(fp, "Name,Won,Lost,W/L Ratio\n");
	return 0;
}

void replace_comma_with_tab(char* line, char* newline) {
	int i = 0;
	while (*(line + i) != '\n')
	{
		if (*(line + i) == ';')
			*(line + i) != '\t';//is it tab?
		i++;
	}
}
int rand_step() {
	int r = rand() % 5;
	return r;
}

int find_who_wins(step first_step, step second_step) {
	//if first eins returns 0 if second return 1
	if (first_step == SPOCK & second_step == ROCK)
		return 0;
	else if (first_step == ROCK & second_step == SPOCK)
		return 1;
	else if (first_step == ROCK & second_step == PAPER)
		return 1;
	else if (first_step == PAPER & second_step == ROCK)
		return 0;
	else if (first_step == SCISSORS & second_step == PAPER)
		return 0;
	else if (first_step == PAPER & second_step == SCISSORS)
		return 1;
	else if (first_step == LIZARD & second_step == SCISSORS)
		return 1;
	else if (first_step == SCISSORS & second_step == LIZARD)
		return 0;
	else if (first_step == LIZARD & second_step == SPOCK)
		return 0;
	else if (first_step == SPOCK & second_step == LIZARD)
		return 1;
	else if (first_step == ROCK & second_step == LIZARD)
		return 0;
	else if (first_step == LIZARD & second_step == ROCK)
		return 1;
	else if (first_step == PAPER & second_step == SPOCK)
		return 0;
	else if (first_step == SPOCK & second_step == PAPER)
		return 1;
	else if (first_step == LIZARD & second_step == PAPER)
		return 0;
	else if (first_step == PAPER & second_step == LIZARD)
		return 1;
	else if (first_step == SCISSORS & second_step == ROCK)
		return 1;
	else if (first_step == ROCK & second_step == SCISSORS)
		return 0;
	else if (first_step == SPOCK & second_step == SCISSORS)
		return 0;
	else if (first_step == SCISSORS & second_step == SPOCK)
		return 1;
	else {
		printf("Error when calc winner\n");
			return 2;
	}
}
void replace_enum_with_string(step step, char* string){
	switch (step) {
	case SPOCK: strcpy(string, "SPOCK");
	case SCISSORS: strcpy(string, "SCISSORS");
	case PAPER: strcpy(string, "PAPER");
	case ROCK: strcpy(string, "ROCK");
	case LIZARD: strcpy(string, "LIZARD");
	}
}

static HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPDWORD p_thread_id)
{
	/* Should check for NULL pointers. Skipped for the sake of simplicity. */

	return CreateThread(
		NULL,            /*  default security attributes */
		0,               /*  use default stack size */
		p_start_routine, /*  thread function */
		NULL,            /*  argument to thread function */
		0,               /*  use default creation flags */
		p_thread_id);    /*  returns the thread identifier */
}