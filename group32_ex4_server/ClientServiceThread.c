#include "ClientServiceThread.h"



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