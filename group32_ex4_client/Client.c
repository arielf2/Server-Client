
#include "Client.h"
#include "SendReceive.h"

// Global Socket
SOCKET m_socket;

int MainClient(char* ip_address, char* port, char* username)
{
	SOCKADDR_IN clientService;
	m_socket = INVALID_SOCKET;

	// Initialize Winsock.
	WSADATA wsaData;

	// Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	// Create Socket
	int user_exit = 0;
	if (CreateAndCheckSocket())
		return 1;

	// Connect to a server.

	//Create a sockaddr_in object clientService and set values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip_address); //Setting the IP address to connect to
	clientService.sin_port = htons(atoi(port)); //Setting the port to connect to.


	int wait = 0, server_resp = 0, connect_res = 0, game_res = 0, error = 0, socket_ret = 0;
	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	
	while (1) {
		//try connection
		connect_res = connect(m_socket, (SOCKADDR*)& clientService, sizeof(clientService));
		if (connect_res == SOCKET_ERROR) {
			//printf("Failed connection, error %ld\n", WSAGetLastError());
			if (ReconnectMenu(0, ip_address, port) == 1) //try reconnecting
				continue;
			else { // user wants to exit
				break;

				//WSACleanup();
				//return 0;
			} 
		}

		else { 	//connection successfull:
			//send client request
			if (SendClientRequest(username) != 0) {
				printf("couldnt write client request to socket\n");
				error = 1;
				break;
				//WSACleanup();
				//return 1;
			}
			//else - successful write to socket, get server response.

			//Approve/Deny Client

			char *AcceptedStr = NULL;
			wait = WaitForMessage(&AcceptedStr, SERVER_WAIT_TIMEOUT);
			if (wait == TIMEOUT_ERROR) { // TIMEOUT - show menu
				closesocket(m_socket); //disconnect from server
				if (ReconnectMenu(2, ip_address, port) == 1) {
					if (CreateAndCheckSocket())
						return 1;
					continue;
				}
				//else - got 2 exit from user. 
				return 0;
			}
			//else - we received a valid message
			server_resp = CheckServerResponse(AcceptedStr, NULL);
			if (server_resp == 1) { //server denied - show menu
				closesocket(m_socket); //disconnect from server
				if (ReconnectMenu(2, ip_address, port) == 1) {
					if (CreateAndCheckSocket()) {
						error = 1;
						break;
						//WSACleanup();
						//return 1;
					}
					continue;
				}
				//else - got 2 exit from user

				//return 0;
				break;
			}

			else { //server resp = 0 - server approved
				printf("Connected to server on %s:%s\n", ip_address, port);
				game_res = GameFlow();
				if (closesocket(m_socket) != 0) {
					printf("Error closing client socket with error code %d\n", WSAGetLastError());
					error = 1;
				}
				if (game_res == TIMEOUT_ERROR) { // TIMEOUT during game
					if (ReconnectMenu(2, ip_address, port) == 1) {
						if (CreateAndCheckSocket()) {
							error = 1;
							break;
							//WSACleanup();
							//return 1;
						}
						continue;
					}
					//else - got 2 exit from user. 
					break;
					//return 0;
				}
				else if (game_res == 1) {
					printf("Error in sending message during game. Exiting\n");
					error = 1;
					break;
				}
				else if (game_res == 4)
					break;
			}								
		}			
	}

	WSACleanup();
	return error;
}


int ReconnectMenu(int reason_for_connect, char* ip_address, char* port) {
	int user_decision = 0;
	long int user_dec = 0;
	char user_resp[2];
start:

	if (reason_for_connect == 0) // this is a first-time connection {
		printf("Failed connecting to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
	else if (reason_for_connect == 1) // reason_for_connect == 1 - server denied
		printf("Server on %s:%s denied the connection request.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
	else //reason_for_connect = 2 - timeout or connection lost
		printf("Connection to server on %s:%s has been lost.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", ip_address, port);
	
	gets_s(user_resp, 2);
	user_dec = strtol(user_resp, NULL, 10);
	if (user_dec != 1 && user_dec != 2)
		goto start;
	else {
		return user_dec;
	}
}

int WaitForMessage(char **AcceptedString, int wait_period) {
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
		return TIMEOUT_ERROR;
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

int SendClientRequest(char *username) {
	char ClientRequest[MAX_CLIENT_REQUEST_LEN + 1] = CLIENT_REQUEST;  // CLIENT_REQUEST:
	TransferResult_t SendRes;

	strcat_s(ClientRequest, MAX_CLIENT_REQUEST_LEN + 1, username); //CLIENT_REQUEST:
	ClientRequest[strlen(ClientRequest)] = '\n'; // Turn the string to the correct protocol message, that ends with '\n' instead of '\0'
	SendRes = SendString(ClientRequest, m_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}

	//else - success, return 0
	return 0;
}

int CreateAndCheckSocket() {
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\nCould not create socket. Exiting\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	return 0;
}

int CheckServerResponse(char* response, char* parameter) {
	//if (response == NULL) {
	//	return 1;
	//}
	parameters_struct p_struct;
	char msg_type[40] = "";
	char param[40] = "";
	// Messages without parameters
	int resp = -1;
	if (CompareProtocolMessages(response, SERVER_APPROVED) == 0) {
		resp = 0;
	}
	else if (CompareProtocolMessages(response, SERVER_DENIED) == 0) {
		resp = 1;
	}
	else if (CompareProtocolMessages(response, SERVER_MAIN_MENU) == 0) {
		resp = 2;
	}
	else if ((CompareProtocolMessages(response, SERVER_PLAYER_MOVE_REQUEST) == 0)) {
		resp = 3;
	}
	else if ((CompareProtocolMessages(response, SERVER_NO_OPPONENTS) == 0)) {
		resp = 9;
	}
	else if ((CompareProtocolMessages(response, SERVER_INVITE) == 0)) {
		resp = 10;
	}
	else {
		parse_command(response, &p_struct);
		if (strcmp(p_struct.message_type, SERVER_OPPONENT_QUIT) == 0) {
			strcpy_s(parameter, 20, p_struct.param1);
		}
		resp = 11;
	}

	//printf("Server response: %s\n", response);
	// Messages with parameters



	free(response);
	return resp;
}

int GameFlow() {
	int wait = 0, server_response = 0, user_response = 0, finish_game = 0, game_ret_val = 0;
	
	while (1) {
		char *AcceptedStr = NULL;
		wait = WaitForMessage(&AcceptedStr, SERVER_WAIT_TIMEOUT);
		if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
			return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
		}
		server_response = CheckServerResponse(AcceptedStr, NULL);
		if (server_response != 2) {
			printf("Debug Print:\nServer Didn't send SERVER_MAIN_MENU\n");
			return 1;
		}
		//else - server response is 2 - main menu, show main menu to user
		user_response = ShowMainMenu();
		char *MessageToSend = NULL;
		if (user_response == 1) { 			// Play against another client

			PrepareMessage(&MessageToSend, "CLIENT_VERSUS", NULL, NULL, NULL, 0);
			if (SendMessageToDest(MessageToSend, &m_socket) != 0)
				return 1;
			game_ret_val = ClientVersusClient();
			if (game_ret_val == TIMEOUT_ERROR || game_ret_val == 1)
				return game_ret_val;
			else continue;
		}
		else if (user_response == 2) { //Play against the server
			PrepareMessage(&MessageToSend, "CLIENT_CPU", NULL, NULL, NULL, 0);
			if (SendMessageToDest(MessageToSend, &m_socket) != 0)
				return 1;
			game_ret_val = ClientVersusServer();
			if (game_ret_val == TIMEOUT_ERROR || game_ret_val == 1)
				return game_ret_val;
			else continue;

		}
		else
		{ //user response == 4 quit
			PrepareMessage(&MessageToSend, "CLIENT_DISCONNECT", NULL, NULL, NULL, 0);
			if (SendMessageToDest(MessageToSend, &m_socket) != 0) {
				return 1;
			}
			return 4;
		}
	}
}

int ShowMainMenu() {
	long int user_decision = 0;
	char user_resp[2];
start:
	printf("Choose what to do next:\n1. Play against another client\n2. Play against the server\n3. View the leaderboard\n4. Quit\n");
	gets_s(user_resp, 2);
	user_decision = strtol(user_resp, NULL, 10);
	if (user_decision != 1 && user_decision != 2 && user_decision != 4) {
		printf("Please choose a valid option. Leaderboard is not supported\n");
		goto start;
	}

	return user_decision;
}

int SendMessageToDest(char *message, SOCKET *local_socket) {
	int return_val = 0;
	TransferResult_t SendRes;

	SendRes = SendString(message, *local_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return_val = 0x555;
	}
	free(message);
	return return_val;
}

int ClientVersusServer() {
	int wait = 0, server_response = 0, user_decision = 0;
	char user_move[MAX_MOVE_LENGTH] = "";
	while (1) { // Game Loop
	char *MoveRequest = NULL, *GameResults = NULL, *GameOverMenu = NULL;
	char *PlayerMove = NULL, *ClientMainMenu = NULL, *ClientReplay = NULL;
	parameters_struct param_struct;

	// Wait and check for SERVER_PLAYER_MOVE_REQUEST message from server

		wait = WaitForMessage(&MoveRequest, SERVER_WAIT_TIMEOUT);
		if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
			return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
		}

		server_response = CheckServerResponse(MoveRequest, NULL);
		//printf("Move request: %s", MoveRequest);
		if (server_response != 3) {
			printf("Debug Print:\nServer Didn't send SERVER_PLAYER_MOVE_REQUEST\n");
			//error?
		}

	// Get user choice of move, and send it to server

		GetUserMove(user_move);
		PrepareMessage(&PlayerMove, "CLIENT_PLAYER_MOVE", user_move, NULL, NULL, 1);
		if (SendMessageToDest(PlayerMove, &m_socket) != 0) // bad value check
			return 1;

		wait = WaitForMessage(&GameResults, SERVER_WAIT_TIMEOUT);

		if (wait == TIMEOUT_ERROR) //got TIMEOUT in receiving the message from server
			return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user

		SummarizeGameResultsClientCPU(GameResults);

	// Wait and check for game over menu message from server

		wait = WaitForMessage(&GameOverMenu, SERVER_WAIT_TIMEOUT);
		if (wait == TIMEOUT_ERROR) //got TIMEOUT in receiving the message from server
			return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user

	// Show game over menu and get user choice for next action

		user_decision = ShowPostGameMenu();
		if (user_decision == 2) {
			PrepareMessage(&ClientMainMenu, "CLIENT_MAIN_MENU", NULL, NULL, NULL, 0);
			if (SendMessageToDest(ClientMainMenu, &m_socket) != 0)
				return 1;
			break;	
		}

		//else - user chose 1. Play Again
		PrepareMessage(&ClientReplay, "CLIENT_REPLAY", NULL, NULL, NULL, 0);
		if (SendMessageToDest(ClientReplay, &m_socket) != 0)
			return 1;

	}
	return 5;
}

int ShowPostGameMenu() {
	long int user_decision = 0;
	char user_resp[2];
start:
	printf("Choose what to do next:\n1. Play again\n2. Return to the main menu\n");
	gets_s(user_resp, 2);
	user_decision = strtol(user_resp, NULL, 10);
	if (user_decision != 1 && user_decision != 2)
		goto start;
	return user_decision;
}

void GetUserMove(char* move) {
start:
	printf("Choose a move from the list: Rock, Paper, Scissors, Lizard or Spock:\n");
	gets_s(move, MAX_MOVE_LENGTH);
	_strupr_s(move, MAX_MOVE_LENGTH); // convert to uppercase
	if (strcmp(move, "ROCK") && strcmp(move, "PAPER") && strcmp(move, "SCISSORS") && strcmp(move, "LIZARD") && strcmp(move, "SPOCK")) {
		printf("Please choose a valid move\n");
		goto start;
	}
}

int Leaderboard() {
	int wait, server_response;
	char *LeaderboardMsg = NULL, *LeaderboardMenu = NULL;
	wait = WaitForMessage(&LeaderboardMsg, SERVER_WAIT_TIMEOUT);
	if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
		return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
	}

	server_response = CheckServerResponse(LeaderboardMsg, NULL);
	//if (server_response != 2) {
	//	printf("Debug Print:\nServer Didn't send SERVER_LEADERBOARD\n");
	//	//error?
	//}
	free(LeaderboardMsg);

	//parse response, print leaderboard

	wait = WaitForMessage(&LeaderboardMenu, SERVER_WAIT_TIMEOUT);
	if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
		return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
	}

	server_response = CheckServerResponse(LeaderboardMenu,NULL );
	//if (server_response != 2) {
	//	printf("Debug Print:\nServer Didn't send SERVER_LEADERBOARD_MENU\n");
	//	//error?
	//}

	ShowLeaderboardMenu();

}

int ShowLeaderboardMenu() {
	long int user_decision = 0;
	char user_resp[2];
start:
	printf("Choose what to do next:\n1. Refresh Leaderboard\n2. Return to the main menu\n");
	gets_s(user_resp, 2);
	user_decision = strtol(user_resp, NULL, 10);
	if (user_decision != 1 && user_decision != 2)
		goto start;
	return user_decision;
}

int ClientVersusClient() {
	int wait, server_response, user_decision, game_not_finished = 1;
	char user_move[MAX_MOVE_LENGTH] = "";
	char *ServerInviteOrNoOpponent = NULL;
	char *ClientMainMenu = NULL;
	char rival_name[20] = "";
	wait = WaitForMessage(&ServerInviteOrNoOpponent, SERVER_WAIT_TIMEOUT * 2); //need to wait for 30 seconds for another opponent
	if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
		return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
	}
	server_response = CheckServerResponse(ServerInviteOrNoOpponent, rival_name); // need to parse parameters
	if (server_response == 9) { //no opponent - back to gameflow to wait for receiving server-main-menu
		return 5;
	}
	if (server_response != 10) { 
		printf("Debug Print:\nServer Didn't send SERVER_INVITE\n");
		//error?
	}

	while (game_not_finished) {
		char *MoveRequestOrOpponentQuit = NULL, *GameResults = NULL, *GameOverMenu = NULL;
		char *PlayerMove = NULL, *ClientReplay = NULL;

		wait = WaitForMessage(&MoveRequestOrOpponentQuit, (SERVER_WAIT_TIMEOUT * 2) + 1);
		if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
			return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
		}
		server_response = CheckServerResponse(MoveRequestOrOpponentQuit, rival_name); // need to parse parameters

		if (server_response == 11) { // SERVER OPPONENT QUIT
			printf("%s has left the game!\n", rival_name);
			game_not_finished = 0;
			//return to main menu value
			break;
		}
		else if (server_response != 3) { // here 3 is move request
			printf("didnt receive move request or opponent quit");
			return 1;
		}

		GetUserMove(user_move);
		PrepareMessage(&PlayerMove, "CLIENT_PLAYER_MOVE", user_move, NULL, NULL, 1);
		//printf("Player Move: %s", PlayerMove);
		if (SendMessageToDest(PlayerMove, &m_socket) != 0) { //bad value check
			printf("Error sending a message\n");
		}
		//wait for server game results

		wait = WaitForMessage(&GameResults, SERVER_WAIT_TIMEOUT * 10);

		if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
			return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
		}
		// parse results and print to screen  (check that SERVER_GAME_RESULTS was received)
		//CheckServerResponse(GameResults);
		
		//printf("Game results: %s", GameResults);
		SummarizeGameResultsClientVersus(GameResults);

		wait = WaitForMessage(&GameOverMenu, SERVER_WAIT_TIMEOUT);	
		//check that we got gameovermenu
		//printf("Gameover menu: %s", GameOverMenu);
		if (wait == TIMEOUT_ERROR) { //got TIMEOUT in receiving the message from server
			return TIMEOUT_ERROR; //go back to main while loop, disconnect and show initial menu to user
		}

		//check that SERVER_GAME_OVER_MENU was received
		user_decision = ShowPostGameMenu();
		if (user_decision == 2) {
			PrepareMessage(&ClientMainMenu, "CLIENT_MAIN_MENU", NULL, NULL, NULL, 0);
			SendMessageToDest(ClientMainMenu, &m_socket);
			return 5;
		}
		//else - user chose 1. Play Again
		PrepareMessage(&ClientReplay, "CLIENT_REPLAY", NULL, NULL, NULL, 0);
		if (SendMessageToDest(ClientReplay, &m_socket) != 0) {
			printf("Error sending client replay message");
		}
	}

	//return 6;
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

void SummarizeGameResultsClientVersus(char *GameResults) {
	char msg_type[30] = "";
	char other_player_name[20] = "";
	char my_move[10] = "";
	char other_player_move[10] = "";
	char my_name[20] = "";
	char rest[60] = "";
	char winner[20] = "";
	sscanf(GameResults, "%[^:]:%[^\n]", msg_type, rest);
	sscanf(rest, "%s %s %s %s", winner, other_player_move, my_move, other_player_name);
	printf("You played: %s\n%s played: %s\n", my_move, other_player_name, other_player_move);
	if (strcmp(winner, "Tie") != 0) {
		printf("%s won!\n", winner);
	}
}

void SummarizeGameResultsClientCPU(char *GameResults) {
	char msg_type[30] = "";
	char other_player_name[20] = "";
	char my_move[10] = "";
	char server_move[10] = "";
	char my_name[20] = "";
	char rest[60] = "";
	char winner[20] = "";
	//sscanf(GameResults, "%[^;];%s", other_step_c, other_user_name);
	sscanf(GameResults, "%[^:]:%[^\n]", msg_type, rest);
	sscanf(rest, "%s %s %s %s", winner, server_move, my_move, my_name);
	printf("You played: %s\nServer played: %s\n", my_move, server_move);
	if (strcmp(winner, "Tie") != 0) {
		printf("%s won!\n", winner);
	}

}