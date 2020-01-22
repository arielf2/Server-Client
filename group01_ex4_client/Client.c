
#include "Client.h"
#include "SendReceive.h"

// Global Socket
SOCKET m_socket;

void MainClient(char* ip_address, char* port, char* username)
{
	SOCKADDR_IN clientService;

	//check parse command:

	//char *MyMsg = NULL;
	//PrepareMessage(&MyMsg, "CLIENT_REQUEST", "Ariel", "Rotem", NULL, 2);
	//char *parameters = NULL;
	//char *Accepted_Str = NULL;
	//char message_type[15] = "";
	//parse_command(MyMsg, message_type, parameters);

	int user_exit = 0;
	// Initialize Winsock.
	WSADATA wsaData;


	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//Call the socket function and return its value to the m_socket variable. 
	// Create a socket.

	CreateAndCheckSocket();
	 // Connect to a server.

	 //Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip_address); //Setting the IP address to connect to
	clientService.sin_port = htons(atoi(port)); //Setting the port to connect to.


	int wait = 0, server_resp = 0, connect_res = 0, game_res = 0;
	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	
	while (1) {
		//try connection
		connect_res = connect(m_socket, (SOCKADDR*)& clientService, sizeof(clientService));
		if (connect_res == SOCKET_ERROR) {
			printf("Failed connection, error %ld\n", WSAGetLastError());
			if (ReconnectMenu(0, ip_address, port) == 1) //try reconnecting
				continue;
			else {
				WSACleanup();
				break;
			} // user wants to exit
		}

		else { 	//connection successfull:
			//send client request
			if (SendClientRequest(username) != 0) {
				printf("couldnt write client request to socket\n");
				exit(1);
			}
			//else - successful write to socket, get server response.

			//Approve/Deny Client

			char *AcceptedStr = NULL;
			wait = WaitForMessage(&AcceptedStr, SERVER_WAIT_TIMEOUT);
			
			if (wait == 1) { // TIMEOUT - show menu
				closesocket(m_socket); //disconnect from server
				if (ReconnectMenu(1, ip_address, port) == 1) {
					m_socket = CreateAndCheckSocket();
					continue;
				}
				//else - got 2 exit from user. 
				printf("Bye\n");
				return;
			}

			//else - wait = 0 so we received a valid message
			server_resp = CheckServerResponse(AcceptedStr);
			if (server_resp == 1) { //server denied - show menu
				closesocket(m_socket); //disconnect from server
				if (ReconnectMenu(2, ip_address, port) == 1) {
					m_socket = CreateAndCheckSocket();
					continue;
				}
				//else - got 2 exit from user
				printf("Bye\n");
				return;
			}

			else { //server resp = 0 - server approved
				printf("Connected to server on %s:%s\n", ip_address, port);
				game_res = GameFlow();
				if (game_res == ERROR_TIMEOUT) { // TIMEOUT during game
					closesocket(m_socket); //disconnect from server
					if (ReconnectMenu(1, ip_address, port) == 1) {
						m_socket = CreateAndCheckSocket();
						continue;
					}
					//else - got 2 exit from user. 
					printf("Bye\n");
					return;
				}
				else if (game_res == 4) {
					closesocket(m_socket);
					break;
				}
				
			}
									
		}			
	}
	//user_exit = ReconnectMenu(0, &clientService, ip_address, port);
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
		return 1;
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
	}

	//free(AcceptedStr);
	return error;

}

int SendClientRequest(char *username) {
	char ClientRequest[MAX_CLIENT_REQUEST_LEN + 1] = CLIENT_REQUEST;  // CLIENT_REQUEST:
	TransferResult_t SendRes;

	strcat_s(ClientRequest, MAX_CLIENT_REQUEST_LEN + 1, username); //CLIENT_REQUEST:Ariel
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
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	return 0;
}

int CheckServerResponse(char* response) {
	//if (response == NULL) {
	//	return 1;
	//}

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
	else if ((CompareProtocolMessages(response, SERVER_PLAYER_MOVE_REQUEST) == 0))
		resp = 3;
	//else if ((CompareProtocolMessages(response), SERVER_GAME_)


	else if ((CompareProtocolMessages(response, SERVER_NO_OPPONENTS) == 0))
		resp = 9;
	free(response);
	return resp;
}


int GameFlow() {
	int wait = 0, server_response = 0, user_response = 0, finish_game = 0, game_ret_val = 0;
	
	// get user response from:
	//1 Play against another client 
	//2 Play against the server
	//3 View the leaderboard
	//4 Quit
	while (1) {
		char *AcceptedStr = NULL;
		wait = WaitForMessage(&AcceptedStr, SERVER_WAIT_TIMEOUT);
		if (wait == 1) { //got TIMEOUT in receiving the message from server
			return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
		}
		server_response = CheckServerResponse(AcceptedStr);
		if (server_response != 2) {
			printf("Debug Print:\nServer Didn't send SERVER_MAIN_MENU\n");
			//error?
		}
		//else - server response is 2 - main menu, show main menu to user
		user_response = ShowMainMenu();
		char *MessageToSend = NULL;
		if (user_response == 1) {
			//play against another client
			PrepareMessage(&MessageToSend, "CLIENT_VERSUS", NULL, NULL, NULL, 0);
			if (SendMessageToDest(MessageToSend) != 0) {
				//error sending message
			}
			//WaitForOpponent()
			game_ret_val = ClientVersusClient();
			if (game_ret_val != 6) { //Client Versus Client has three return values: 5 for game done, 6 for replay, -1 for error timeout
				// make some game loop here. NO - loop is inside CvC
			}
			free(MessageToSend);

		}
		else if (user_response == 2) {
			//play against server
			PrepareMessage(&MessageToSend, "CLIENT_CPU", NULL, NULL, NULL, 0);
			if (SendMessageToDest(MessageToSend) != 0) {
				//error sending message
			}
			free(MessageToSend);
			while (finish_game == 0) {
				game_ret_val = ClientVersusServer();
				if (game_ret_val == 5) //Client Versus Server has three return values: 5 for game done, 6 for replay, -1 for timeout
					finish_game = 1;
				else if (game_ret_val == ERROR_TIMEOUT) //timeout during one of the messages in ClientVServer
					return ERROR_TIMEOUT;
				// else, ClientVersusServer returned 6 which is replay - continue while loop.
			}
			
			continue; // if we reached here, user decided to go back to main menu;


		}
		else if (user_response == 3) {
			// view leader board
			PrepareMessage(&MessageToSend, "CLIENT_LEADERBOARD", NULL, NULL, NULL, 0);
			if (SendMessageToDest(MessageToSend) != 0) {
				//error sending message
			}
			Leaderboard();
			free(MessageToSend);
		}
		else
		{//user response == 4 quit
			PrepareMessage(&MessageToSend, "CLIENT_DISCONNECT", NULL, NULL, NULL, 0);
			if (SendMessageToDest(MessageToSend) != 0) {
				//error sending message
			}
			free(MessageToSend);
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
	if (user_decision != 1 && user_decision != 2 && user_decision != 3 && user_decision != 4)
		goto start;
	return user_decision;
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
	strcat_s(*Dest, len, ":");
	if (num_of_valid_params != 0) {
		// at least one valid parameter
		strcat_s(*Dest, len, (const char*) parameter_1);
		if (num_of_valid_params >= 2) {
			strcat_s(*Dest, len, ";");
			strcat_s(*Dest, len, (const char*) parameter_2);
			if (num_of_valid_params == 3) {
				strcat_s(*Dest, len, ";");
				strcat_s(*Dest, len, (const char*) parameter_3);
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

int SendMessageToDest(char *message) {
	TransferResult_t SendRes;

	SendRes = SendString(message, m_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}
	return 0;
}

int ClientVersusServer() {
	int wait, server_response, user_decision;
	char user_move[MAX_MOVE_LENGTH] = "";
	char *MoveRequest = NULL, *GameResults = NULL, *GameOverMenu = NULL;
	char *PlayerMove = NULL, *ClientMainMenu = NULL, *ClientReplay = NULL;

	wait = WaitForMessage(&MoveRequest, SERVER_WAIT_TIMEOUT);
	if (wait == 1) { //got TIMEOUT in receiving the message from server
		return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
	}

	server_response = CheckServerResponse(MoveRequest);
	printf("Move request: %s", MoveRequest);
	if (server_response != 3) {
		printf("Debug Print:\nServer Didn't send SERVER_PLAYER_MOVE_REQUEST\n");
		//error?
	}
	GetUserMove(user_move);

	PrepareMessage(&PlayerMove, "CLIENT_PLAYER_MOVE", user_move, NULL, NULL, 1);
	printf("Player Move: %s", PlayerMove);
	if (SendMessageToDest(PlayerMove) == 0) { //bad value check
		//error
	}
	//wait for server game results

	wait = WaitForMessage(&GameResults, SERVER_WAIT_TIMEOUT);

	if (wait == 1) { //got TIMEOUT in receiving the message from server
		return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
	} 
	// parse results and print to screen  (check that SERVER_GAME_RESULTS was received)
	//CheckServerResponse(GameResults);
	printf("Game results: %s", GameResults);

	wait = WaitForMessage(&GameOverMenu, SERVER_WAIT_TIMEOUT);
	printf("Gameover menu: %s", GameOverMenu);
	if (wait == 1) { //got TIMEOUT in receiving the message from server
		return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
	}

	//check that SERVER_GAME_OVER_MENU was received
	user_decision = ShowPostGameMenu();
	if (user_decision == 2) {
		PrepareMessage(&ClientMainMenu, "CLIENT_MAIN_MENU", NULL, NULL, NULL, 0);
		SendMessageToDest(ClientMainMenu);
		return 5;
	}
	//else - user chose 1. Play Again
	PrepareMessage(&ClientReplay, "CLIENT_REPLAY", NULL, NULL, NULL, 0);
	printf("replay: %s", ClientReplay);
	SendMessageToDest(ClientReplay);

	return 6;
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
	if (wait == 1) { //got TIMEOUT in receiving the message from server
		return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
	}

	server_response = CheckServerResponse(LeaderboardMsg);
	//if (server_response != 2) {
	//	printf("Debug Print:\nServer Didn't send SERVER_LEADERBOARD\n");
	//	//error?
	//}
	free(LeaderboardMsg);

	//parse response, print leaderboard

	wait = WaitForMessage(&LeaderboardMenu, SERVER_WAIT_TIMEOUT);
	if (wait == 1) { //got TIMEOUT in receiving the message from server
		return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
	}

	server_response = CheckServerResponse(LeaderboardMenu);
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
	char *ServerInviteOrNoOpponent = NULL, *MoveRequestOrOpponentQuit = NULL, *GameResults = NULL, *GameOverMenu = NULL;
	char *PlayerMove = NULL, *ClientMainMenu = NULL, *ClientReplay = NULL;

	wait = WaitForMessage(&ServerInviteOrNoOpponent, SERVER_WAIT_TIMEOUT * 2); //need to wait for 30 seconds for another opponent
	if (wait == 1) { //got TIMEOUT in receiving the message from server
		return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
	}

	server_response = CheckServerResponse(ServerInviteOrNoOpponent); // need to parse parameters
	//check that the response is "server invite" or "no opponent"
	if (server_response == 9) { //no opponent
		//return to main menu: send "CLIENT MAIN MENU
		PrepareMessage(&ClientMainMenu, "CLIENT_MAIN_MENU", NULL, NULL, NULL, 0);
		SendMessageToDest(ClientMainMenu);
		return 5;
	}
		
	if (server_response != 10) { //change value of 10 to something
		printf("Debug Print:\nServer Didn't send SERVER_INVITE\n");
		//error?
	}


	while (game_not_finished) {
		wait = WaitForMessage(&MoveRequestOrOpponentQuit, 15);
		if (wait == 1) { //got TIMEOUT in receiving the message from server
			return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
		}
		server_response = CheckServerResponse(MoveRequestOrOpponentQuit); // need to parse parameters
		if (server_response == 13113) { // 13113 is (example)  SERVER OPPONENT QUIT
			printf("Debug Print: opponent quits \n");
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
		if (SendMessageToDest(PlayerMove) == 0) { //bad value check
			//error
		}
		//wait for server game results

		wait = WaitForMessage(&GameResults, SERVER_WAIT_TIMEOUT * 2);

		if (wait == 1) { //got TIMEOUT in receiving the message from server
			return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
		}
		// parse results and print to screen  (check that SERVER_GAME_RESULTS was received)
		//CheckServerResponse(GameResults);
		printf("Game results: %s", GameResults);

		wait = WaitForMessage(&GameOverMenu);
		//check that we got gameovermenu
		printf("Gameover menu: %s", GameOverMenu);
		if (wait == 1) { //got TIMEOUT in receiving the message from server
			return ERROR_TIMEOUT; //go back to main while loop, disconnect and show initial menu to user
		}

		//check that SERVER_GAME_OVER_MENU was received
		user_decision = ShowPostGameMenu();
		if (user_decision == 2) {
			PrepareMessage(&ClientMainMenu, "CLIENT_MAIN_MENU", NULL, NULL, NULL, 0);
			SendMessageToDest(ClientMainMenu);
			return 5;
		}
		//else - user chose 1. Play Again
		PrepareMessage(&ClientReplay, "CLIENT_REPLAY", NULL, NULL, NULL, 0);
		printf("replay: %s", ClientReplay);
		SendMessageToDest(ClientReplay);
	}
	return 6;
}


int WaitForOpponent() {

}