/*
#pragma once

#pragma comment(lib, "ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib>
#include <string.h>
#include <winsock2.h>
/#include "SendRecv.h"
#define _CRT_SECURE_NO_WARNINGS
#define SERVER_ADDRESS_STR "127.0.0.1"
#define SERVER_PORT 2345
#define NUM_OF_WORKER_THREADS 2

*/
#ifndef HEADER_H
#define HEADER_H

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SERVER_ADDRESS_STR "127.0.0.1"
#define SERVER_PORT 2345
#define NUM_OF_WORKER_THREADS 2
#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )
#pragma comment(lib, "ws2_32.lib")

#define MAX_LOOPS 3
#define TIMEOUT 600
#define SEND_STR_SIZE 350
#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )
#define STEP_LEN 9
#define USER_NAME_LEN 20
//#include "SharedFuncs.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>
#include "SendRecv.h"

extern SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
extern BOOL   ThreadIndex[NUM_OF_WORKER_THREADS];
extern HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
extern BOOL   exit_state;
extern SOCKET AcceptSocket;

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;
typedef enum { ROCK, PAPER, SCISSORS, LIZARD, SPOCK } step;
typedef enum { CPU, VERSUS } status;

typedef struct _exit_thread_param_struct
{
	SOCKET *MainSocket;


} exit_thread_param_struct;
typedef struct _accept_thread_param_struct
{
	SOCKET *MainSocket;
	SOCKET *accept;

} accept_thread_param_struct;

typedef struct _thread_param_struct
{
	SOCKET *MySocket;
	int my_index;

} thread_param_struct;
typedef struct _parameters_struct {
	char *message_type;
	char *param1;
	char *param2;
	char *param3;
	char *param4;
}parameters_struct;
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/


/*	Description: Finds empty thread slots for new created threads
	Parameters: none
	Returns: index of the threads
*/
static int FindFirstUnusedThreadSlot();

/*	Description: Close sockets and thread handles
	Parameters: none
	Returns: none
*/
static void CleanupWorkerThreads();

/*	Description: Main service thread for each client 
	Parameters: thread param struct
	Returns: error code
*/
static DWORD ServiceThread(LPVOID lpParam);

/*	Description: Parser for the received message, splits to a message type and parameters
	Parameters: command - the receive message
				struct of parameters
	Returns:	number of parameters
*/
int parse_command(char *command,  parameters_struct* parameters_s);

/*	Description: Writes the relevant information to the GameSession.txt file
	Parameters: The selected move in the game, and the client username
	Returns: none
*/
void write_move_and_username_to_file(char *move, char *username);

/*	Description: replaces the enum type of the selected move with the represntative string
	Parameters: step enum type (the move), pointer to string that will be updated
	Returns: none
*/
void replace_enum_with_string(step step, char* string);

/*	Description: compare the two moves and decide the winner
	Parameters: two valid moves as enum types step
	Returns: number represtning the winner
*/

int find_who_wins(step first_step, step second_step);

/*	Description: replaces string representing the selected move with the correct step enum type
	Parameters: string - move as string, step - pointer to the enum type that will be updated
	Returns: none
*/
void replace_string_with_enum(step *step, char* string);

/*	Description: randomize move
	Parameters: none
	Returns: integer/enum type representing the step
*/
int rand_step();

/*	Description: Simplified Create Thread Function
	Parameters: thread parameter struct
				name of thread function
				pointer to thread id
	Returns:	handle to the created thread
*/

HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters, LPDWORD p_thread_id);

/*	Description: Check whether the game session file exists. If not, create the gamesession file
	Parameters: none
	Returns: error code
*/

int check_if_file_exists();

/*	Description: The server function responsible for accepting the exit input in the server console
	Parameters: thread parameter struct
	Returns:  error code
*/
int exit_function(exit_thread_param_struct *thread_param);

/*	Description: Waits for another player to join the game, through the global array
	Parameters: index of the global array, value to check
	Returns: integer representing if another user is found
*/

int wait_for_another_player(int index, BOOL val);

/*	Description: Wait for the server to send a message. TIMEOUT if no response within (wait period) seconds
	Parameters:  AcceptedString - pointer to a char pointer, that will eventually point to the received string
				 int wait period - number of seconds to wait for message receiving.
				 m_socket - the socket on which a message should arrive
	Returns:	 error code	*/

int WaitForMessage(char **AcceptedString, int wait_period, SOCKET m_socket);

/*	Description: WINAPI function for the exit thread
	Parameters: thread parameters struct as void (to be casted)
	Returns: error code
*/

DWORD WINAPI exit_thread_dword(LPVOID lpParam);

/*	Description: WINAPI function for the accept thread function
	Parameters: thread parameters struct as void (to be casted)
	Returns: error code
*/

DWORD WINAPI accept_thread_dword(LPVOID lpParam);

/*	Description: accept connection - the function of the accept thread - thread waiting to accept connections to the socket
	Parameters: thread parameters
	Returns: error code
*/
int accept_function(accept_thread_param_struct *thread_param);

/*	Description: Simplified message sender
	Parameters: message, socket on which to send the message
	Returns: error code
*/
int send_message_simple(char message[], SOCKET a_socket);

/*	Description: Creates the game result message according to the desired format with message types and parameters
	Parameters: server/client a move, client b move, other client's username, first client username, pointer to string
	Returns: none
*/
void create_game_results_message(step a_step, step others_step, char other_user_name[], char user_name[], char* SendStr);

/*	Description: Creates session for file
	Parameters: none
	Returns: none
*/
void create_file_session();

/*	Description: When no opponents are found, send the no opponents message and then the main menu to client
	Parameters: socket on which to send the message
	Returns: error code
*/
int send_no_opponent_and_main_menu(SOCKET t_socket);

/*	Description: When a client is approved, send the approval message and then the main menu
	Parameters: socket on which to send the message
	Returns: error code
*/
int send_approved_and_main_menu(SOCKET t_socket);

/*	Description: When two clients are willing to play against each other, send the invite and then move request
	Parameters: socket on which to send the message
	Returns: error code
*/
int send_invite_and_move_request(SOCKET t_socket);


/**
 * SendBuffer() uses a socket to send a buffer.
 *
 * Accepts:
 * -------
 * Buffer - the buffer containing the data to be sent.
 * BytesToSend - the number of bytes from the Buffer to send.
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if sending succeeded
 * TRNS_FAILED - otherwise
 */
TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);

/**
 * SendString() uses a socket to send a string.
 * Str - the string to send.
 * sd - the socket used for communication.
 */
TransferResult_t SendString(const char *Str, SOCKET sd);

/**
 * Accepts:
 * -------
 * ReceiveBuffer() uses a socket to receive a buffer.
 * OutputBuffer - pointer to a buffer into which data will be written
 * OutputBufferSize - size in bytes of Output Buffer
 * BytesReceivedPtr - output parameter. if function returns TRNS_SUCCEEDED, then this
 *					  will point at an int containing the number of bytes received.
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */
TransferResult_t ReceiveBuffer(char* OutputBuffer, int RemainingBytesToReceive, SOCKET sd);

/**
 * ReceiveString() uses a socket to receive a string, and stores it in dynamic memory.
 *
 * Accepts:
 * -------
 * OutputStrPtr - a pointer to a char-pointer that is initialized to NULL, as in:
 *
 *		char *Buffer = NULL;
 *		ReceiveString( &Buffer, ___ );
 *
 * a dynamically allocated string will be created, and (*OutputStrPtr) will point to it.
 *
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving and memory allocation succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */
TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd);

#endif

