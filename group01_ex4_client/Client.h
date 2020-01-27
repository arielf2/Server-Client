#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAX_CLIENT_REQUEST_LEN 36 //CLIENT_REQUEST:20\n 36
#define MAX_USERNAME_LENGTH 21
#define MAX_IP_LENGTH 16 // 255.255.255.255, 15 + 1
#define MAX_PORT_LENGTH 5 // 8888, + 1
#define MAX_MOVE_LENGTH 9
#define CLIENT_REQUEST "CLIENT_REQUEST:"
#define SERVER_APPROVED "SERVER_APPROVED\n"
#define SERVER_DENIED "SERVER_DENIED\n"
#define SERVER_MAIN_MENU "SERVER_MAIN_MENU\n"
#define SERVER_PLAYER_MOVE_REQUEST "SERVER_PLAYER_MOVE_REQUEST\n"
#define SERVER_NO_OPPONENTS "SERVER_NO_OPPONENTS\n"
#define SERVER_OPPONENT_QUIT "SERVER_OPPONENT_QUIT"
#define SERVER_INVITE "SERVER_INVITE\n"
#define SERVER_WAIT_TIMEOUT 15
#define TIMEOUT_ERROR -1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "SharedFuncs.h"
#pragma comment(lib, "Ws2_32.lib")


void MainClient(char* ip_address, char* port, char* username);


/* ########################################################################### */
/* #						        Menus			    					 # */
/* ########################################################################### */

/*	Description:	Show The reconnection menu to the user, and display the correct error according to the reason for reconnection
	Parameters:		reason_for_connect - 0: initial connection
										 1: server denied -> reconnection
										 2: timeout connection lost -> reconnection
					ip_address - the IP to which we want to connect
					port - the port to which we want to connet
	Returns:		user decision - 2 if user asked to exit, 1 to reconnect  */
int ReconnectMenu(int reason_for_connect, char* ip_address, char* port);

/*	Description: Show the main menu to the user and accept the response
	Parameters:
	Returns:	integer representing the response */
int ShowMainMenu();

/*	Description: Shows the after-game menu to the user and gets his/her desired action
	Parameters: None
	Return: 1 if user wants to replay, 2 if users wants to return to main menu */
int ShowPostGameMenu();

/*	Description: Shows the leaderboard menu to the user
	Parameters: None
	Returns:	1 if user wants to refresh leaderboard, 2 if user wants to return to main menu */
int ShowLeaderboardMenu();



/* ########################################################################### */
/* #						        Game			    					 # */
/* ########################################################################### */


/*	Description:
	Parameters:
	Returns:	*/
int GameFlow();

/*	Description: Initiates a game between client and server, by sending and receiving the messages according to the requirements
	Parameters: None
	Returns: 5 if user wants to finish game, 6 for replay, -1 for TIMEOUT error during game */
int ClientVersusServer();

/*	Description: Gets the chosen move by the user through the keyboard
	Parameters: char* move - it will point to the given move by the user
	Returns: None */
void GetUserMove(char* move);

/*	Description: Initiates a game between client and client, by sending and receiving the messages according to the requirements
	Parameters: None
	Returns: 5 if user wants to finish game, 6 for replay, -1 for TIMEOUT error during game */
int ClientVersusClient();

/*	Description: 
	Parameters:
	Returns:	*/
int Leaderboard();


/*	Description: Wait for the server to send a message. TIMEOUT if no response within 15 seconds
	Parameters:  AcceptedString - pointer to a char pointer, that will eventually point to the received string
		         int wait period - number of seconds to wait for message receiving.
	Returns:	 error code	*/
int WaitForMessage(char **AcceptedString, int wait_period);


/*	Description: Send the client request to the server, with the name of the user
	Parameters:	username - name of the user, as received from the command line
	Returns: 0 upon successfull send, error otherwise */
int SendClientRequest(char *username);

int CreateAndCheckSocket();

/*	Description: After receiving a response from the server, check which type of message it is.
	Parameters:	 reponse - the string received from the the server
	Returns:	 integer representing the message	*/
int CheckServerResponse(char* response, char* parameter);

/*	Description: Gets the total length of the valid parameters for a specific message
	Parameters:	 parameter_1/2/3 - parameters for the message
				 num_of_valid_params - number of valid params (up to 3)
	Returns		 int - total length (in bytes) of the valid parameters	*/
//int GetTotalLen(char* parameter_1, char* parameter_2, char* parameter_3, int num_of_valid_params);


/*	Description: Sends the message in the specified format to the destination, through a global socket
	Parameters: message - the message to send
	Returns: 0 if successfull, !0 if failed
*/
int SendMessageToDest(char *message, SOCKET *local_socket);



int parse_command(char *command, parameters_struct* parameters_s);


SummarizeGameResultsClientVersus(char *GameResults);
SummarizeGameResultsClientCPU(char *GameResults);