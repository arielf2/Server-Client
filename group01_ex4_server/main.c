
#include "Header.h"



HANDLE ThreadHandles[NUM_OF_WORKER_THREADS] = { NULL,NULL };
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS] = { NULL,NULL };
BOOL   ThreadIndex[NUM_OF_WORKER_THREADS] = { FALSE, FALSE };




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
	exit_thread_param_struct exit_thread_param;
	thread_param_struct threads_params[NUM_OF_WORKER_THREADS];
	int thread_id;
	// Initialize Winsock.
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);


	if (StartupRes != NO_ERROR)
	{
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return 1;
	}


	// Create a socket.    
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (MainSocket == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
	}


	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		goto server_cleanup_2;
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(atoi(argv[1])); //The htons function converts a u_short from host to TCP/IP network byte order 
									   //( which is big-endian ).

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

	// Listen on the Socket.
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
	}
	/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;

	exit_thread_param.MainSocket = &MainSocket;
	exit_thread = CreateThreadSimple(exit_thread_dword, &exit_thread_param, &thread_id);

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
			threads_params[Ind].MySocket = &(ThreadInputs[Ind]);
			threads_params[Ind].my_index = Ind;
			ThreadHandles[Ind] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ServiceThread,
				&threads_params[Ind],
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


//Service thread is the thread that opens for each successful client connection and "talks" to the client.
static DWORD ServiceThread(LPVOID lpParam)
{
	
	thread_param_struct *thread_param;
	int return_val = 0;
	SOCKET *t_socket;
	int my_index;
	int other_index;
	char SendStr[SEND_STR_SIZE];
	int find_other_player = -1;
	int player_number = -1;
	BOOL Done = FALSE;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	step others_step = 0;
	step cpu_step = 0;
	char message_type[15];
	char* parameters[4];
	char user_name[20] = "";
	step step = 0;
	char step_c[9] = "";
	status status = -1;
	FILE *fp_game_session = NULL;
	char game_session_delim = ';';
	parameters_struct parameters_s;

	/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

	/* Check if lpParam is NULL */
	if (NULL == lpParam)
	{
		return -1;
	}

	/*
	* Convert (void *) to parameters type.
	*/
	thread_param = (thread_param_struct*)lpParam;
	t_socket = thread_param->MySocket;
	my_index = thread_param->my_index;
	other_index = abs(my_index - 1);
	/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/


	while (!Done)
	{
		char *AcceptedStr = NULL;
		if (WaitForMessage(&AcceptedStr,150, *t_socket) == -1) {////
			printf("Error in wait.\n");

		}
		parse_command(AcceptedStr, &parameters_s);
		
		
		if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_REQUEST")) {

			strcpy_s(user_name, 20, parameters_s.param1); // with strcpy user name doesnt become gibrish
			strcpy(SendStr, "SERVER_APPROVED\n");
			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
			strcpy(SendStr, "SERVER_MAIN_MENU\n");

			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_MAIN_MENU"))
		{
			strcpy(SendStr, "SERVER_MAIN_MENU\n");

			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_CPU")) {
			status = CPU;
			cpu_step = rand_step();
			strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
			
		}

		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_VERSUS")) {

			ThreadIndex[my_index] = 1;
			if (ThreadIndex[other_index]) {//check if other's bit is 1
				status = VERSUS;//change only here?
				strcpy(SendStr, "SERVER_INVITE\n");
				SendRes = SendString(SendStr, *t_socket);

				if (SendRes == TRNS_FAILED)
				{
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
					return 1;
				}

				strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
				SendRes = SendString(SendStr, *t_socket);

				if (SendRes == TRNS_FAILED)
				{
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
					return 1;
				}

			}
			else {
				
				//wait for another client for 30 second - wait to see that someone wrote to file
				if (wait_for_another_player(other_index, 1)) {//there is another player
					status = VERSUS;//change only here?
					strcpy(SendStr, "SERVER_INVITE\n");
					SendRes = SendString(SendStr, *t_socket);

					if (SendRes == TRNS_FAILED)
					{
						printf("Service socket error while writing, closing thread.\n");
						closesocket(*t_socket);
						return 1;
					}

					strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
					SendRes = SendString(SendStr, *t_socket);

					if (SendRes == TRNS_FAILED)
					{
						printf("Service socket error while writing, closing thread.\n");
						closesocket(*t_socket);
						return 1;
					}
				}
				else {//there is not other player
					strcpy(SendStr, "SERVER_NO_OPPONENTS\n");
					SendRes = SendString(SendStr, *t_socket);

					if (SendRes == TRNS_FAILED)
					{
						printf("Service socket error while writing, closing thread.\n");
						closesocket(*t_socket);
						return 1;
					}
				}
			}

		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_LEADERBOARD")) {
			SendRes = send_leader_board(*t_socket);
			if (SendRes == TRNS_FAILED)
				{
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
				}
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_PLAYER_MOVE")) {
			if (status == CPU) {
				char move[50] = "";
				strcpy_s(move, 50, parameters_s.param1);
				replace_string_with_enum(&step, move);
				int winner = -1;
				winner = find_who_wins(cpu_step, step);
				replace_enum_with_string(cpu_step, step_c);

				if (winner == 0) {
					//sprintf(SendStr, "SERVER_GAME_RESULTS:server;%s;%s;server\n", step_c, parameters_s.param1);
					sprintf(SendStr, "SERVER_GAME_RESULTS:Server %s %s %s\n", step_c, move, user_name);
					Done = 0;
				}
				else if (winner == 1) {
					sprintf(SendStr, "SERVER_GAME_RESULTS:%s %s %s %s\n",user_name, step_c, move, user_name);
				}
				else if (winner == 2) {
					//sprintf(SendStr, "SERVER_GAME_RESULTS:Tie;%s;%s;server\n", step_c, parameters_s.param1);
					sprintf(SendStr, "SERVER_GAME_RESULTS:Tie %s %s %s\n", step_c, move, user_name);
				}
				else 
					printf("Error in player move\n");

			}
			else if (status == VERSUS) {
				/*send results when playing against other player */
				char line_versus[255] = ""; // the line from the file is read into this variable

				int i = 0;
				char other_step_c[10] = "";
				char other_user_name[20] = "";
				char move[50] = "";
				strcpy_s(move, 50, parameters_s.param1); // keep the user move because param1 becomes gibrish
				if (check_if_file_exists()) {
					step = 0;
					int win = -1;
					write_move_and_username_to_file(move, user_name);
					while (check_how_many_lines(line_versus) < 2) {
						Sleep(1000);
					}
					//I read other player move so turn off my biy
					ThreadIndex[my_index] = 0;

					sscanf(line_versus, "%[^;];%s", other_step_c, other_user_name); // get the other player's name and move

					replace_string_with_enum(&step, move);
					replace_string_with_enum(&others_step, other_step_c);
					win = find_who_wins(others_step, step); 
					if (win == 0) {
						replace_enum_with_string(step, step_c);
						//sprintf(SendStr, "SERVER_GAME_RESULTS:%s;%s;%s,%s\n", other_user_name, step_c, move, other_user_name);
						sprintf(SendStr, "SERVER_GAME_RESULTS:%s %s %s %s\n", other_user_name, other_step_c, move, other_user_name);
					}
					else if (win == 1) {
						replace_enum_with_string(step, step_c);
						//sprintf(SendStr, "SERVER_GAME_RESULTS:%s;%s;%s,%s\n", user_name, step_c, move, other_user_name);
						sprintf(SendStr, "SERVER_GAME_RESULTS:%s %s %s %s\n", user_name, other_step_c, move, other_user_name);
					}
					else if (win == 2) {
						replace_enum_with_string(step, step_c);
						//sprintf(SendStr, "SERVER_GAME_RESULTS:Tie;%s;%s\n", step_c, move, other_user_name);
						sprintf(SendStr, "SERVER_GAME_RESULTS:Tie %s %s %s \n", step_c, move, other_user_name);
					}
					else
						printf("Error in player move\n");
				}
				else 
				{
					
					fp_game_session = fopen("GameSession.txt", "w");//create file
					fclose(fp_game_session);
					int win = -1;
					while (check_how_many_lines(line_versus) < 1) {
						Sleep(1000);
					}
					sscanf(line_versus, "%[^;];%s", other_step_c, other_user_name);
					write_move_and_username_to_file(move, user_name);	

					replace_string_with_enum(&step, move);
					replace_string_with_enum(&others_step, other_step_c);
					win = find_who_wins(others_step, step);
					if (win == 0) {
						replace_enum_with_string(step, step_c);
						//sprintf(SendStr, "SERVER_GAME_RESULTS:%s;%s;%s,%s\n", other_user_name, step_c, move, other_user_name);
						sprintf(SendStr, "SERVER_GAME_RESULTS:%s %s %s %s\n", other_user_name, other_step_c, move, other_user_name);
					}
					else if (win == 1) {
						replace_enum_with_string(step, step_c);
						//sprintf(SendStr, "SERVER_GAME_RESULTS:%s;%s;%s,%s\n", user_name, step_c, move, other_user_name);
						sprintf(SendStr, "SERVER_GAME_RESULTS:%s %s %s %s\n", user_name, other_step_c, move, other_user_name);
					}
					else if (win == 2) {
						replace_enum_with_string(step, step_c);
						//sprintf(SendStr, "SERVER_GAME_RESULTS:Tie;%s;%s\n", step_c, move, other_user_name);
						sprintf(SendStr, "SERVER_GAME_RESULTS:Tie %s %s %s\n", step_c, move, other_user_name);
					}
					else
						printf("Error in player move\n");

					ThreadIndex[my_index] = 0;
					wait_for_another_player(other_index, 0);//wait for other player to finish read from file
					if(remove("GameSession.txt") != 0)
						printf("Error whem removing file.\n"); 
				}

				SendRes = SendString(SendStr, *t_socket);

				if (SendRes == TRNS_FAILED)
				{
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
					return 1;
				}
			}

			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
			strcpy(SendStr, "SERVER_GAME_OVER_MENU\n");

			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_REPLAY")) {
			if (status == CPU) {//as  client cpu
				status = CPU;
				cpu_step = rand_step();
				strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
				SendRes = SendString(SendStr, *t_socket);

				if (SendRes == TRNS_FAILED)
				{
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
					return 1;
				}
			}
			else if (status == VERSUS) {//same to client versus
				ThreadIndex[my_index] = 1;
				if (ThreadIndex[other_index]) {//check if other's bit is 1
					status = VERSUS;//change only here?

					strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
					SendRes = SendString(SendStr, *t_socket);

					if (SendRes == TRNS_FAILED)
					{
						printf("Service socket error while writing, closing thread.\n");
						closesocket(*t_socket);
						return 1;
					}

				}
				else {

					//wait for another client for 30 second - wait to see that someone wrote to file
					if (wait_for_another_player(other_index, 1)) {//there is another player
						status = VERSUS;//change only here?

						strcpy(SendStr, "SERVER_PLAYER_MOVE_REQUEST\n");
						SendRes = SendString(SendStr, *t_socket);

						if (SendRes == TRNS_FAILED)
						{
							printf("Service socket error while writing, closing thread.\n");
							closesocket(*t_socket);
							return 1;
						}
					}
					else {//there is not other player
						strcpy(SendStr, "SERVER_NO_OPPONENTS\n");
						SendRes = SendString(SendStr, *t_socket);

						if (SendRes == TRNS_FAILED)
						{
							printf("Service socket error while writing, closing thread.\n");
							closesocket(*t_socket);
							return 1;
						}
						strcpy(SendStr, "SERVER_MAIN_MENU\n");

						SendRes = SendString(SendStr, *t_socket);

						if (SendRes == TRNS_FAILED)
						{
							printf("Service socket error while writing, closing thread.\n");
							closesocket(*t_socket);
							return 1;
						}
					}
				}

			}

		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_REFRESH")) {
			/*bonus*/
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_DISCONNECT"))
		{
			Done = 1;
		}
		else
		{
			strcpy(SendStr, "I don't understand");
		}


		free(AcceptedStr);
	}
	////////close?
	
	closesocket(*t_socket);
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

int check_if_file_exists() {
	FILE* fp;
	int l_wait_code = -1;
	int l_ret_val = -1;
	int exists = -1;
	HANDLE l_mutex_handle = NULL;
	l_mutex_handle = OpenMutex(SYNCHRONIZE, TRUE, "game_session_file_mutex");

	l_wait_code = WaitForSingleObject(l_mutex_handle, INFINITE);
	if (WAIT_OBJECT_0 != l_wait_code)
	{
		printf("Error when waiting for mutex\n");

	}

	fp = fopen("GameSession.txt", "r");
	
	l_ret_val = ReleaseMutex(l_mutex_handle);
	if (FALSE == l_ret_val)
	{
		printf("Error when releasing game_session_file_mutex\n");
	}

	if (fp != NULL) {
		fclose(fp);
		return 1;
	}
	else
		return 0;
}
int check_how_many_lines(char *line) {
	FILE* fp;
	int l_wait_code = -1;
	int l_ret_val = -1;
	int counter = 0; 
	HANDLE l_mutex_handle = NULL;

	
	l_mutex_handle = OpenMutex(SYNCHRONIZE, TRUE, "game_session_file_mutex");

	l_wait_code = WaitForSingleObject(l_mutex_handle, INFINITE);
	if (WAIT_OBJECT_0 != l_wait_code)
	{
		printf("Error when waiting for mutex\n");

	}

	int file_open_error = fopen_s(&fp, "GameSession.txt", "r");
	//fp = fopen("GameSession.txt", "r");
	if (file_open_error != 0) {
		printf("error opening file with error %d", GetLastError());
	}
	if (fp == NULL) {
		printf("Error when check_how_many_lines\n");
		return -1;
	}
	while (feof(fp) == 0) {
		
		if (NULL != fgets(line, 30, fp)) {
			if (!(STRINGS_ARE_EQUAL(line, "")))
				counter++;
		}
	}
	fclose(fp);
	l_ret_val = ReleaseMutex(l_mutex_handle);
	if (FALSE == l_ret_val)
	{
		printf("Error when releasing game_session_file_mutex\n");
	}
	return counter;
	
}

void write_move_and_username_to_file(char *move, char *username) {
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

	int file_open_error = fopen_s(&fp, "GameSession.txt", "a");
	if (file_open_error != 0) {
		printf("Error opening file with error: %d", GetLastError());
	}
	fputs(move, fp);
	fputs(";", fp);
	fputs(username, fp);
	fputs("\n", fp);

	fclose(fp);
	l_ret_val = ReleaseMutex(l_mutex_handle);
	if (FALSE == l_ret_val)
	{
		printf("Error when releasing game_session_file_mutex\n");
	}
	
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
int parse_command(char *command,  parameters_struct* parameters_s) {
	char s[2] = ":";
	char delim[2] = ";";
	int counter = 0;
	int counter_p = 0;
	int i = 0;
	int j = 0;
	char* parameters_string;
	char l_string[250] = "";
	/*count how many parameters*/
	while (command[i] != '\n') {
		l_string[i] = command[i];
		if (command[i] == ';')
			counter++;
		if (command[i] == ':')
			counter_p++;
		i++;
	}
	l_string[i] = '\0';
	//test
	counter++;

	parameters_s->message_type = NULL;
	parameters_s->param1 = NULL;
	parameters_s->param2 = NULL;
	parameters_s->param3 = NULL;
	parameters_s->param4 = NULL;

	parameters_s->message_type = strtok(l_string, s);
	if (counter == 1 & counter_p == 1)
		parameters_s->param1 = strtok( NULL, s);
	else {
		parameters_string = strtok(NULL, s);
		if (counter > 1) {
			parameters_s->param1 = strtok(parameters_string, delim);
		if (counter >= 2)
			parameters_s->param2 = strtok(NULL, delim);
		if (counter >= 3)
			parameters_s->param3 = strtok(NULL, delim);
		if (counter == 4)
			parameters_s->param4 = strtok(NULL, delim);
		}
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
step rand_step() {
	int r = rand() % 5;
	switch (r) {
	case 0: return ROCK;
	case 1: return PAPER;
	case 2: return SCISSORS;
	case 3: return LIZARD;
	case 4: return SPOCK;
	}
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
	else if (first_step == SCISSORS & second_step == SCISSORS)
		return 2;
	else if (first_step == SPOCK & second_step == SPOCK)
		return 2;

	else if (first_step == ROCK & second_step == ROCK)
		return 2;

	else if (first_step == PAPER & second_step == PAPER)
		return 2;

	else if (first_step == LIZARD & second_step == LIZARD)
		return 2;

	else {
		printf("Error when calc winner\n");
			return 3;
	}
}
void replace_enum_with_string(step step, char* string){
	switch (step) {
	case SPOCK: {		
		strcpy(string, "SPOCK");
		break;
	}
	case SCISSORS: {
		strcpy(string, "SCISSORS");
		break;

	}
	case PAPER: {
		strcpy(string, "PAPER");
		break;

	}
	case ROCK:{
		strcpy(string, "ROCK");
		break;

	}
	case LIZARD:{
		strcpy(string, "LIZARD");
		break;

	}
	}
}
void replace_string_with_enum(step *step, char* string) {
	if(strcmp(string, "SPOCK")==0)
		*step = SPOCK;
	else if(strcmp(string, "PAPER")==0)
		*step = PAPER;
	else if (strcmp(string, "LIZARD")==0)
		*step = LIZARD;
	else if (strcmp(string, "SCISSORS")==0)
		*step = SCISSORS;
	else if (strcmp(string, "ROCK")==0)
		*step = ROCK;
	else
		printf("Error in replace_string_with_enum\n");
}
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters, LPDWORD p_thread_id)
{
	HANDLE thread_handle = NULL;

	if (NULL == p_start_routine)
	{
		printf("Error when creating a thread");
		printf("Received null pointer");
		exit(1);
	}

	if (NULL == p_thread_id)
	{
		printf("Error when creating a thread");
		printf("Received null pointer");
		exit(1);
	}

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_start_routine,     /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		p_thread_id);        /*  returns the thread identifier */

	return thread_handle;
}
DWORD WINAPI exit_thread_dword(LPVOID lpParam)
{
	thread_param_struct *thread_param;
	int return_val = 0;

	/* Check if lpParam is NULL */
	if (NULL == lpParam)
	{
		return -1;
	}

	/*
	* Convert (void *) to parameters type.
	*/
	thread_param = (thread_param_struct*)lpParam;
	exit_function(thread_param);


	return 0;
}
void exit_function(exit_thread_param_struct *thread_param) {

	int socket_return_val = -1;
	int return_val = -1;
	int main_return_val = -1;
	char input[5];
	while (1) {
		scanf("%s",input);
		if (strcmp(input, "exit") == 0) {
			/*close handles*/
			for (int i = 0; i < NUM_OF_WORKER_THREADS; i++) {

				if (ThreadHandles[i] != NULL) {
					return_val = CloseHandle(ThreadHandles[i]);
					if (return_val == 0)////////////////// get return val  close handle returns non zero if its successful
						printf("Error when exiting\n");
				}
			}
			/* close main socket */
			main_return_val = closesocket(*thread_param->MainSocket);
			if (main_return_val != 0)////////////////// get return val
				printf("Error when exiting %d\n", WSAGetLastError());
			return 0;
		}
	}
}
int wait_for_another_player(int index, BOOL val) {
	int i = 0;
	for (i = 0; i < 3; i++) {
		Sleep(10000); // 10 seconds
		if (ThreadIndex[index] == val)
			return 1;
	}
	return 0;
}

int WaitForMessage(char **AcceptedString, int wait_period, SOCKET m_socket) {
	int error = 0;

	fd_set set;
	struct timeval time;
	time.tv_sec = wait_period;
	time.tv_usec = 0;

	FD_ZERO(&set);
	FD_SET(m_socket, &set);

	//char *AcceptedStr = NULL;
	TransferResult_t RecvRes;

	error = select(0, &set, NULL, NULL, &time);

	if (error == 0) {
		printf("TIMEOUT: %d seconds passed and no response from server, in WaitForMessage, ReceiveData client\n", wait_period);
		return -1;
	}
	RecvRes = ReceiveString(AcceptedString, m_socket);

	if (RecvRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}
	else if (RecvRes == TRNS_DISCONNECTED)
	{
		printf("Server closed connection. Bye!\n");
		return 0x555;
	}
	else
	{
		//printf("accepted string is: %s\n", *AcceptedString);
		//return error;
	}

	//free(AcceptedStr);
	//return error;

}
