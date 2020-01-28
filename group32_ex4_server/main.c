
#include "ServerFuncs.h"

HANDLE ThreadHandles[NUM_OF_WORKER_THREADS] = { NULL,NULL };
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS] = { NULL,NULL };
BOOL   ThreadIndex[NUM_OF_WORKER_THREADS] = { FALSE, FALSE };
BOOL   exit_state = FALSE;
SOCKET AcceptSocket = INVALID_SOCKET;

int main(int argc, char *argv[]) {
	int Ind, Loop, bindRes, ListenRes, ret_val, thread_id, accept_thread_id, return_val = -1, wait_code = -1;
	unsigned long Address;
	SOCKADDR_IN service;
	SOCKET MainSocket = INVALID_SOCKET;
	HANDLE game_session_file_mutex = NULL, handles_array[2] = { NULL, NULL }/*The first is for exit and second for accept*/;
	exit_thread_param_struct exit_thread_param;
	accept_thread_param_struct accept_thread_param;
	thread_param_struct threads_params[NUM_OF_WORKER_THREADS];
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	TransferResult_t SendRes;
	char SendStr[SEND_STR_SIZE];

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
	service.sin_port = htons(atoi(argv[1]));

	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	//Create game session file mutex
	game_session_file_mutex = CreateMutex(NULL, FALSE, "game_session_file_mutex");
	if (game_session_file_mutex == NULL)
		printf("Error while creating game_session_file_mutex\n");

	//Listen on the Socket
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;

	//Initialize threads parameters and create exit thread
	exit_thread_param.MainSocket = &MainSocket;
	accept_thread_param.MainSocket = &MainSocket;

	handles_array[0] = CreateThreadSimple(exit_thread_dword, &exit_thread_param, &thread_id);
	if (handles_array[0] == NULL)
		printf("Error when create exit thread\n");

	//Start waiting for clients
	printf("Waiting for a client to connect...\n");

	while (exit_state == FALSE)//wait for clients until server got exit input in console
	{
		handles_array[1] = CreateThreadSimple(accept_thread_dword, &accept_thread_param, &accept_thread_id);
		if (handles_array[1] == NULL)
			printf("Error when create exit thread\n");

		//wait for the first between exit and accept threads finish
		wait_code = WaitForMultipleObjects(2, handles_array, FALSE, INFINITE);
		if (wait_code != 0 && wait_code != 1)
			printf("Error in wait for multiple error %ld\n", GetLastError());

		if (exit_state) {//the exit thread finished
			goto server_cleanup_3;
		}
		//exit thread did not finish means accept thread finished first. 
		if (AcceptSocket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			goto server_cleanup_3;
		}

		ret_val = CloseHandle(handles_array[1]);//close accept thread
		if (0 == ret_val)
			printf("Error when closing handle\n");

		printf("Client Connected.\n");

		Ind = FindFirstUnusedThreadSlot();

		if (Ind == NUM_OF_WORKER_THREADS) //no slot is available
		{
			printf("No slots available for client, dropping the connection.\n");
			if (send_message_simple("SERVER_DENIED\n", AcceptSocket))
				goto server_cleanup_3;
			closesocket(AcceptSocket); //Closing the socket, dropping the connection.
		}
		else
		{
			ThreadInputs[Ind] = AcceptSocket;
			threads_params[Ind].MySocket = &(ThreadInputs[Ind]);
			threads_params[Ind].my_index = Ind;
			ThreadHandles[Ind] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServiceThread, &threads_params[Ind], 0, NULL);
		}
	}

server_cleanup_3:
	ret_val = CloseHandle(handles_array[0]);//close exit thread
	if (handles_array[1] != NULL)
		CloseHandle(handles_array[1]);
	if (0 == ret_val)
		printf("Error when closing exit handle\n");
	CleanupWorkerThreads();

server_cleanup_2:
	ret_val = closesocket(MainSocket);

server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	return 0;
}


//Service thread is the thread that opens for each successful client connection and "talks" to the client.
static DWORD ServiceThread(LPVOID lpParam)
{
	thread_param_struct *thread_param;
	int return_val = 0, my_index, other_index, find_other_player = -1, player_number = -1;
	SOCKET *t_socket;
	BOOL Done = FALSE;
	TransferResult_t SendRes, RecvRes;
	step step = 0, others_step = 0, cpu_step = 0;
	char message_type[15], user_name[20] = "", game_session_delim = ';', step_c[STEP_LEN] = "", other_user_name[USER_NAME_LEN] = "", SendStr[SEND_STR_SIZE];
	char* parameters[4];
	status status = -1;
	parameters_struct parameters_s;

	//Initialize thread and get local parameters from thread parameters
	if (NULL == lpParam)
		return -1;
	thread_param = (thread_param_struct*)lpParam;
	t_socket = thread_param->MySocket;
	my_index = thread_param->my_index;
	other_index = abs(my_index - 1);

	while (!Done)//Wait for message until client leave
	{
		char *AcceptedStr = NULL;
		if (WaitForMessage(&AcceptedStr, TIMEOUT, *t_socket) == -1) /* TO or error in recieve message. close thread*/
			return 1;
		parse_command(AcceptedStr, &parameters_s);
		if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_REQUEST")) {
			strcpy_s(user_name, 20, parameters_s.param1);
			if (send_approved_and_main_menu(*t_socket))
				goto local_cleanup;
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_MAIN_MENU")) {
			if (send_message_simple("SERVER_MAIN_MENU\n", *t_socket))
				goto local_cleanup;
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_CPU")) {
			status = CPU;
			cpu_step = rand_step();
			if (send_message_simple("SERVER_PLAYER_MOVE_REQUEST\n", *t_socket))
				goto local_cleanup;
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_VERSUS")) {
			ThreadIndex[my_index] = 1;
			if (ThreadIndex[other_index]) {//check if other's bit is 1
				status = VERSUS;
				if (send_invite_and_move_request(*t_socket))
					goto local_cleanup;
			}
			else {//wait for another client for 30 second - wait to see that someone wrote to file
				if (wait_for_another_player(other_index, 1)) {//there is another player
					status = VERSUS;
					if (send_invite_and_move_request(*t_socket))
						goto local_cleanup;
				}
				else {//there is no other player
					if (send_no_opponent_and_main_menu(*t_socket))
						goto local_cleanup;
				}
			}
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_PLAYER_MOVE")) {
			if (status == CPU) {
				char move[50] = "";
				strcpy_s(move, 50, parameters_s.param1);
				replace_enum_with_string(cpu_step, step_c);
				create_game_results_message(step, cpu_step, "Server", user_name, SendStr);
			}
			else if (status == VERSUS) {/*send results when playing against other player */

				char line_versus[255] = ""; // the line from the file is read into this variable
				char other_step_c[10] = "";
				char move[50] = "";
				strcpy_s(move, 50, parameters_s.param1); // keep the user move because param1 becomes gibrish
				if (check_if_file_exists()) {
					write_move_and_username_to_file(move, user_name);
					while (check_how_many_lines(line_versus) < 2) {
						Sleep(1000);
					}
					ThreadIndex[my_index] = 0;//I read other player move so turn off my biy
					sscanf(line_versus, "%[^;];%s", other_step_c, other_user_name); // get the other player's name and move
					replace_string_with_enum(&step, move);
					replace_string_with_enum(&others_step, other_step_c);
					create_game_results_message(step, others_step, other_user_name, user_name, SendStr);
				}
				else {
					create_file_session();
					while (check_how_many_lines(line_versus) < 1) {
						Sleep(1000);
					}
					sscanf(line_versus, "%[^;];%s", other_step_c, other_user_name);
					write_move_and_username_to_file(move, user_name);
					replace_string_with_enum(&step, move);
					replace_string_with_enum(&others_step, other_step_c);
					create_game_results_message(step, others_step, other_user_name, user_name, SendStr);
					ThreadIndex[my_index] = 0;
					wait_for_another_player(other_index, 0);//wait for other player to finish read from file
					if (remove("GameSession.txt") != 0)
						printf("Error whem removing file.\n");
				}
			}
			if (send_message_simple(SendStr, *t_socket))
				goto local_cleanup;

			if (send_message_simple("SERVER_GAME_OVER_MENU\n", *t_socket))
				goto local_cleanup;
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_REPLAY")) {
			if (status == CPU) {//as  client cpu
				status = CPU;
				cpu_step = rand_step();
				if (send_message_simple("SERVER_PLAYER_MOVE_REQUEST\n", *t_socket))
					goto local_cleanup;
			}
			else if (status == VERSUS) {//same as client versus
				ThreadIndex[my_index] = 1;
				if (ThreadIndex[other_index]) {//check if other's bit is 1
					status = VERSUS;
					if (send_message_simple("SERVER_PLAYER_MOVE_REQUEST\n", *t_socket))
						goto local_cleanup;
				}
				else {//wait for another client for 30 second - wait to see that someone wrote to file
					if (wait_for_another_player(other_index, 1)) {//there is another player
						status = VERSUS;//change only here?
						if (send_message_simple("SERVER_PLAYER_MOVE_REQUEST\n", *t_socket))
							goto local_cleanup;
					}
					else {//there is no other player
						sprintf(SendStr, "SERVER_OPPONENT_QUIT:%s\n", other_user_name);
						if (send_message_simple(SendStr, *t_socket))
							goto local_cleanup;
						if (send_message_simple("SERVER_MAIN_MENU\n", *t_socket))
							goto local_cleanup;
					}
				}
			}
		}
		else if (STRINGS_ARE_EQUAL(parameters_s.message_type, "CLIENT_DISCONNECT"))
			Done = 1;
		else
			strcpy(SendStr, "I don't understand");

		free(AcceptedStr);
	}

	return 0;
local_cleanup:
	closesocket(*t_socket);
	return 1;
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
int parse_command(char *command, parameters_struct* parameters_s) {
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
		parameters_s->param1 = strtok(NULL, s);
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
void replace_enum_with_string(step step, char* string) {
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
	case ROCK: {
		strcpy(string, "ROCK");
		break;

	}
	case LIZARD: {
		strcpy(string, "LIZARD");
		break;

	}
	}
}
void replace_string_with_enum(step *step, char* string) {
	*step = 0;
	if (strcmp(string, "SPOCK") == 0)
		*step = SPOCK;
	else if (strcmp(string, "PAPER") == 0)
		*step = PAPER;
	else if (strcmp(string, "LIZARD") == 0)
		*step = LIZARD;
	else if (strcmp(string, "SCISSORS") == 0)
		*step = SCISSORS;
	else if (strcmp(string, "ROCK") == 0)
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
	exit_thread_param_struct *thread_param;
	int return_val = 0;

	/* Check if lpParam is NULL */
	if (NULL == lpParam)
	{
		return -1;
	}

	/*
	* Convert (void *) to parameters type.
	*/
	thread_param = (exit_thread_param_struct*)lpParam;
	return_val = exit_function(thread_param);


	return return_val;
}
DWORD WINAPI accept_thread_dword(LPVOID lpParam)
{
	accept_thread_param_struct *thread_param;
	int return_val = 0;

	/* Check if lpParam is NULL */
	if (NULL == lpParam)
	{
		return -1;
	}

	/*
	* Convert (void *) to parameters type.
	*/
	thread_param = (accept_thread_param_struct*)lpParam;
	accept_function(thread_param);


	return 0;
}
int exit_function(exit_thread_param_struct *thread_param) {

	int socket_return_val = -1;
	int return_val = -1;
	int main_return_val = -1;
	char input[5];
	while (1) {
		scanf("%s", input);
		if (strcmp(input, "exit") == 0) {
			/*close handles*/
			exit_state = TRUE;
			for (int i = 0; i < NUM_OF_WORKER_THREADS; i++) {

				if (ThreadHandles[i] != NULL) {
					return_val = CloseHandle(ThreadHandles[i]);
					if (return_val == 0)
						printf("Error when exiting\n");
				}
			}
			/* close main socket */
			if (*thread_param->MainSocket != NULL)
			{
				main_return_val = closesocket(*thread_param->MainSocket);
				if (main_return_val != 0)
					printf("Error when exiting %d\n", WSAGetLastError());
			}
			return 0;
		}
	}
}
int accept_function(accept_thread_param_struct *thread_param) {
	AcceptSocket = accept(*thread_param->MainSocket, NULL, NULL);
	if (AcceptSocket == INVALID_SOCKET)
	{
		printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());

	}
	return 0;
}
int wait_for_another_player(int index, BOOL val) {
	int i = 0;
	for (i = 0; i < 3; i++) {
		Sleep(5000); // 5 seconds
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
		return -1;
	}
	else if (RecvRes == TRNS_DISCONNECTED)
	{
		printf("Server closed connection. Bye!\n");
		return -1;
	}
	return 0;


}
int send_message_simple(char message[], SOCKET a_socket) {
	char SendStr[SEND_STR_SIZE];
	TransferResult_t SendRes;
	strcpy(SendStr, message);
	SendRes = SendString(SendStr, a_socket);
	if (SendRes == TRNS_FAILED) {
		printf("Service socket error while writing, closing thread.\n");
		return 1;
	}
	return 0;
}
void create_game_results_message(step a_step, step others_step, char other_user_name[], char user_name[], char* SendStr) {
	int win = -1;
	char step_c[STEP_LEN] = "";
	char other_step_c[STEP_LEN] = "";
	win = find_who_wins(others_step, a_step);
	replace_enum_with_string(a_step, step_c);
	replace_enum_with_string(others_step, other_step_c);
	if (win == 0) {
		sprintf(SendStr, "SERVER_GAME_RESULTS:%s %s %s %s\n", other_user_name, other_step_c, step_c, other_user_name);
	}
	else if (win == 1) {
		sprintf(SendStr, "SERVER_GAME_RESULTS:%s %s %s %s\n", user_name, other_step_c, step_c, other_user_name);
	}
	else if (win == 2) {
		sprintf(SendStr, "SERVER_GAME_RESULTS:Tie %s %s %s \n", other_step_c, step_c, other_user_name);
	}
	else
		printf("Error in player move\n");
}
int send_invite_and_move_request(SOCKET t_socket) {
	if (send_message_simple("SERVER_INVITE\n", t_socket))
		return 1;
	if (send_message_simple("SERVER_PLAYER_MOVE_REQUEST\n", t_socket))
		return 1;
	return 0;
}
int send_approved_and_main_menu(SOCKET t_socket) {
	if (send_message_simple("SERVER_APPROVED\n", t_socket))
		return 1;
	if (send_message_simple("SERVER_MAIN_MENU\n", t_socket))
		return 1;
	return 0;
}
int send_no_opponent_and_main_menu(SOCKET t_socket) {
	if (send_message_simple("SERVER_NO_OPPONENTS\n", t_socket))
		return 1;
	if (send_message_simple("SERVER_MAIN_MENU\n", t_socket))
		return 1;
	return 0;
}
void create_file_session() {
	FILE *fp_game_session = NULL;
	fp_game_session = fopen("GameSession.txt", "w");//create file
	fclose(fp_game_session);
}