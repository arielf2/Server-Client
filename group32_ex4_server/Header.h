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

static int FindFirstUnusedThreadSlot();
static void CleanupWorkerThreads();
static DWORD ServiceThread(LPVOID lpParam);
int parse_command(char *command,  parameters_struct* parameters_s);
void write_move_and_username_to_file(char *move, char *username);
void replace_enum_with_string(step step, char* string);
int find_who_wins(step first_step, step second_step);
void replace_string_with_enum(step *step, char* string);
int rand_step();
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters, LPDWORD p_thread_id);
int check_if_file_exists();
int exit_function(exit_thread_param_struct *thread_param);
int wait_for_another_player(int index, BOOL val);
int WaitForMessage(char **AcceptedString, int wait_period, SOCKET m_socket);
DWORD WINAPI exit_thread_dword(LPVOID lpParam);
DWORD WINAPI accept_thread_dword(LPVOID lpParam);
int accept_function(accept_thread_param_struct *thread_param);
int send_message_simple(char message[], SOCKET a_socket);
void create_game_results_message(step a_step, step others_step, char other_user_name[], char user_name[], char* SendStr);


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

