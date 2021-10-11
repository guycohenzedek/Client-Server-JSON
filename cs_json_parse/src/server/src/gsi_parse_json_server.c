/**************************************************************************
* Name : gsi_parse_json_server.c
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Server demo program using Networking and ThreadPool
* 				Read configuration from file supplied on command line <a.out> --config=<config_file>
* 				Default values if no file supplied: <a.out> --config=
* 				listen on 3 ports and receive messages from clients on those ports
* 				writing log messages into log file.
*****************************************************************************/

/* Includes */
#include <stdlib.h>
#include <time.h>
#include "gsi_parse_json_config.h"
#include "gsi_is_log_api.h"
#include "gsi_thread_pool.h"
#include "gsi_is_network_tcp.h"
#include "gsi_build_parse_data.h"

/* Defines and Macros */
#define 	GSI_IS_FAIL				-1
#define		GSI_IS_TRUE				 1
#define 	GSI_IS_FALSE			 0
#define		GSI_IS_SERVER_THREADS	 3
#define		GSI_IS_SERVER_QUEUE_SIZE 3
#define 	GSI_IS_MAX_STRINGS		200  /* Max Number of global array size of strings */
#define 	GSI_IS_MAX_STR_LEN		1024 /* Max Length of each string in the global array */
#define 	GSI_IS_NO_PRINT			0	 /* Boolean flag to indicate that NO print to screen */
#define 	GSI_IS_PRINT_SCREEN		1	 /* Boolean flag to indicate that print to screen */
#define		GSI_IS_MAX_BUF_SIZE		1024

/* Global variables */

// Global array of strings for server READ/WRITE OP_CODES
static char** g_arr_strings = NULL;

// instance of client structure contains all its config parameters
extern struct gsi_prase_json_config_server_params g_config_server_params;

/********************************/
/* Static functions declaration */
/********************************/
static int gsi_server_init_clients();
static int gsi_server_init_strings(char* s_file_name);
static void gsi_server_clean_strings(int i_index);
static void* gsi_server_thread_parse_client(void* p_args);
static void gsi_server_timed_service();
static void gsi_server_infinite_service();
static int gsi_server_handle_op_code(struct gsi_json_msg* p_json_msg);
static int gsi_server_handle_read_str(int i_index);
static int gsi_server_handle_write_str(int i_index, char* s_new_str, int i_len);
static int gsi_server_handle_read_file(char* s_file_name, int flags);
static int gsi_server_handle_write_file(char* s_file_name, char* s_msg);
static int gsi_server_handle_print_log(char* s_file_name);
static int gsi_server_handle_read_file_by_id(char* s_file_name, int i_id);
static int gsi_server_port_to_client(unsigned int ui_port);

/*###########################################################################
 	 * Name:        main.
 	 * Description: Entry point of the program, this is server demo program
 	 * Parameter:   char** argv - [1] - config file
 	 * Return: 	    Success - 0
 	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
int main(int argc, char** argv)
{
	// Create log file from config file
	FILE* f_log = gsi_is_create_log_file("gsi-log-server", NULL);
	if (NULL == f_log)
	{
		return GSI_IS_FAIL;
	}

	// Read configuration from file or default according to command line arguments
	if (GSI_PARSE_JSON_CONFIG_SUCCESS != gsi_parse_json_config_get_config(argc, argv))
	{
		printf("cannot load configuration parameters\n");
		return GSI_IS_FAIL;
	}

	gsi_is_title_to_log(f_log, "server test is up");

	do
	{
		// Allocate array of strings for server read them from file in config file
		if (0 != gsi_server_init_strings(g_config_server_params.s_server_data_file))
		{
			LOG_ERROR("couldn't init array of strings");
			break;
		}

		// Set up 3 listening threads
		if (0 != gsi_server_init_clients())
		{
			LOG_ERROR("couldn't init clients");
		}
	}
	while (0);

	// Free resources
	gsi_server_clean_strings(GSI_IS_MAX_STRINGS);

	// Close log file to free resources
	if (GSI_LOG_RC_SUCCESS != gsi_is_close_log(f_log))
	{
		printf("couldn't close log file");
	}

	return 0;
}

/***********************************/
/* Static functions implementation */
/***********************************/
/*###########################################################################
	 * Name:		gsi_server_init_strings
	 * Description: Allocate array of strings for server use.
	 * 				Initialize array by read strings from s_file_name
	 * Parameter:   [in] char* s_file_name - file to read from
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_init_strings(char* s_file_name)
{
	int i_len = 0;
	int i_index = 0;
	char s_buffer[GSI_IS_MAX_STR_LEN];
	FILE* f_str_file = NULL;

	// Check input validation
	if (NULL == s_file_name)
	{
		LOG_ERROR("invalid argument!");
		return GSI_IS_FAIL;
	}

	// Memory allocation for array of strings
	g_arr_strings = (char **)calloc(GSI_IS_MAX_STRINGS, sizeof(char *));
	if (NULL == g_arr_strings)
	{
		LOG_ERROR("memory allocation for global array failed");
		return GSI_IS_FAIL;
	}

	// Open file to read from it
	f_str_file = fopen(s_file_name, "r");
	if (NULL == f_str_file)
	{
		LOG_ERROR("couldn't open %s", s_file_name);

		free(g_arr_strings);
		g_arr_strings = NULL;

		return GSI_IS_FAIL;
	}

	// Read strings from file up to GSI_IS_MAX_STRINGS
	for (i_index = 0; i_index < GSI_IS_MAX_STRINGS; ++i_index)
	{
		// Reset buffer
		memset(s_buffer, 0, sizeof(s_buffer));

		// Read from file into buffer
		if (NULL == fgets(s_buffer, GSI_IS_MAX_STR_LEN, f_str_file))
		{
			break;
		}

		i_len = strlen(s_buffer);

		// Replace the '\n' at the end of the string by '\0'
		s_buffer[i_len - 1] = '\0';

		// Allocate memory for each new string
		g_arr_strings[i_index] = malloc(i_len);
		if (NULL == g_arr_strings[i_index])
		{
			LOG_ERROR("memory allocation for string in global array failed");

			// Rollback free
			gsi_server_clean_strings(i_index);
			break;
		}

		// Copy Content
		strcpy(g_arr_strings[i_index], s_buffer);
	}

	// Check if error occurred
	if (i_index != GSI_IS_MAX_STRINGS)
	{
		LOG_ERROR("couldn't initialize the global array");

		gsi_server_clean_strings(i_index);
		fclose(f_str_file);

		return GSI_IS_FAIL;
	}

	// Close file
	fclose(f_str_file);

	LOG_INFO("Successfully init global array of string");

	return 0;
}

/*###########################################################################
	 * Name:		gsi_server_clean_strings
	 * Description: Roll back clean function to free all memory allocations of global array
	 * Parameter:   [in] int i_index - index of the position to start clean
	 * Return:		None
#############################################################################*/
static void gsi_server_clean_strings(int i_index)
{
	// Check input validation
	if ((0 > i_index) || (GSI_IS_MAX_STRINGS < i_index))
	{
		LOG_ERROR("index %d is out of range", i_index);
		return;
	}

	// Loop to free all allocated entries
	for (int i = (i_index - 1); i >= 0; --i)
	{
		free(g_arr_strings[i]);
		g_arr_strings[i] = NULL;
	}

	// Free the global array
	free(g_arr_strings);
	g_arr_strings = NULL;

	LOG_INFO("Successfully clean all the resources in global array");
}

/*###########################################################################
	 * Name:		gsi_server_init_clients
	 * Description: Init 3 threads to listen on 3 different ports
	 * Return: 		Success - 0
	 * 				Failure - -1
#############################################################################*/
static int gsi_server_init_clients()
{
	unsigned int ui_port_1 = g_config_server_params.ui_port1;
	unsigned int ui_port_2 = g_config_server_params.ui_port2;
	unsigned int ui_port_3 = g_config_server_params.ui_port3;

	// Create ThreadPool from parameters in config file
	gsi_thread_pool_t* pool = gsi_is_thread_pool_create(GSI_IS_SERVER_THREADS, GSI_IS_SERVER_QUEUE_SIZE);
	if (NULL == pool)
	{
		LOG_ERROR("couldn't create the ThreadPool");
		return GSI_IS_FAIL;
	}

	do
	{
		// Active 3 threads to listen 3 different ports.
		if (GSI_TP_RC_SUCCESS != gsi_is_thread_pool_add(pool, gsi_server_thread_parse_client, (void *)(&ui_port_1)))
		{
			LOG_ERROR("couldn't add job to the ThreadPool");
			break;
		}

		if (GSI_TP_RC_SUCCESS != gsi_is_thread_pool_add(pool, gsi_server_thread_parse_client, (void *)(&ui_port_2)))
		{
			LOG_ERROR("couldn't add job to the ThreadPool");
			break;
		}

		if (GSI_TP_RC_SUCCESS != gsi_is_thread_pool_add(pool, gsi_server_thread_parse_client, (void *)(&ui_port_3)))
		{
			LOG_ERROR("couldn't add job to the ThreadPool");
			break;
		}
	}
	while (0);

	// Destroy ThreadPool
	if (GSI_TP_RC_SUCCESS != gsi_is_thread_pool_destroy(pool, GSI_TP_DESTROY_GRACEFUL))
	{
		LOG_ERROR("couldn't destroy the ThreadPool");
		return 1;
	}

	return 0;
}

/*###########################################################################
	 * Name:		gsi_server_thread_parse_client
	 * Description: Main thread function to listen on connection
	 * Parameter:   [in] void* p_args - holds the port number
	 * Return:		Always NULL
#############################################################################*/
static void* gsi_server_thread_parse_client(void* p_args)
{
	unsigned int ui_port = *((unsigned int *)p_args);
	struct gsi_net_tcp server;

	// Check input validation
	if (NULL == p_args)
	{
		LOG_ERROR("invalid argument!");
		return NULL;
	}

	// Init Server object
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_server_init(&server, ui_port))
	{
		LOG_ERROR("server init failed");
		return NULL;
	}

	// Check if server needs timer
	if ((-1 == g_config_server_params.i_server_timer) ||
		 (0 == g_config_server_params.i_server_timer))
	{
		gsi_server_infinite_service(&server);
	}
	else
	{
		gsi_server_timed_service(&server);
	}

	// Cleanup - close open file descriptors.
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_server_cleanup(&server))
	{
		LOG_ERROR("cleanup failed");
	}

	return NULL;
}

/*###########################################################################
	 * Name:		gsi_server_timed_service
	 * Description: run server on timer
	 * Parameter:   [in] struct gsi_net_tcp* p_server - pointer to server
	 * Return:		None
#############################################################################*/
static void gsi_server_timed_service(struct gsi_net_tcp* p_server)
{
	int i_rc = 0;
	int i_run_flag = GSI_IS_TRUE;
	time_t t_seconds = g_config_server_params.i_server_timer;
	time_t t_start_time = time(NULL);
	struct gsi_json_msg json_msg;

	// Reset json-msg
	memset(&json_msg, 0, sizeof(json_msg));

	while ((i_run_flag) && (time(NULL) - t_start_time) < t_seconds)
	{
		// Check events on fds of server
		i_rc = gsi_is_network_tcp_server_poll(p_server);
		switch (i_rc)
		{
		case GSI_NET_RC_SUCCESS:
			LOG_INFO("still listen\n");
			break;

		case GSI_NET_RC_HASDATA:
			LOG_INFO("client %d sent message:", gsi_server_port_to_client(p_server->ui_port));

			if (GSI_JSON_SUCCESS != gsi_is_recv_json_msg(p_server, &json_msg))
			{
				LOG_ERROR("receive message failed");
			}

			// Operate according to operation code
			if (0 != gsi_server_handle_op_code(&json_msg))
			{
				LOG_ERROR("server handle op code failed");
			}
			break;

		case GSI_NET_RC_CONNECTERR:
			i_run_flag = GSI_IS_FALSE;
			LOG_ERROR("client from port %d disconnected\n", p_server->ui_port);
			break;

		default:
			i_run_flag = GSI_IS_FALSE;
			LOG_ERROR("error has been occurred");
		}

		// Reset and free resources of json-msg object
		gsi_build_parse_reset_object(&json_msg);

		sleep(1);
	}

	// Check finish status
	if (i_run_flag)
	{
		LOG_INFO("thread on port %d timeout\n", p_server->ui_port);
	}
	else
	{
		LOG_ERROR("thread on port %d stopped\n", p_server->ui_port);
	}
}

/*###########################################################################
	 * Name:		gsi_server_infinite_service
	 * Description: run server on infinite loop (until error will occur)
	 * Parameter:   [in] struct gsi_net_tcp* p_server - pointer to server
	 * Return:		None
#############################################################################*/
static void gsi_server_infinite_service(struct gsi_net_tcp* p_server)
{
	int i_rc = 0;
	int i_run_flag = GSI_IS_TRUE;
	struct gsi_json_msg json_msg;

	// Reset json-msg
	memset(&json_msg, 0, sizeof(json_msg));

	while (i_run_flag)
	{
		// Check events on fds of server
		i_rc = gsi_is_network_tcp_server_poll(p_server);
		switch (i_rc)
		{
		case GSI_NET_RC_SUCCESS:
			LOG_INFO("still listen\n");
			break;

		case GSI_NET_RC_HASDATA:
			LOG_INFO("client %d sent message:", gsi_server_port_to_client(p_server->ui_port));

			if (GSI_JSON_SUCCESS != gsi_is_recv_json_msg(p_server, &json_msg))
			{
				LOG_ERROR("receive message failed");
			}

			// Operate according to operation code
			if (0 != gsi_server_handle_op_code(&json_msg))
			{
				LOG_ERROR("server handle op code failed");
			}
			break;

		case GSI_NET_RC_CONNECTERR:
			i_run_flag = GSI_IS_FALSE;
			LOG_ERROR("client from port %d disconnected\n", p_server->ui_port);
			break;

		default:
			i_run_flag = GSI_IS_FALSE;
			LOG_ERROR("error has been occurred");
		}

		// Reset and free resources of json-msg object
		gsi_build_parse_reset_object(&json_msg);

		sleep(1);
	}

	LOG_ERROR("thread on port %d stopped\n", p_server->ui_port);
}

/*###########################################################################
	 * Name:		gsi_server_handle_op_code
	 * Description: Check the operation code of the message and call the right action
	 * Parameter:   [in] struct gsi_json_msg* p_json_msg - pointer to json-msg
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_handle_op_code(struct gsi_json_msg* p_json_msg)
{
	// Check input validation
	if (NULL == p_json_msg)
	{
		LOG_ERROR("invalid argument!");
		return GSI_IS_FAIL;
	}

	// Check op-code
	switch(p_json_msg->i_op_code)
	{
		case GSI_READ_STR:
			return gsi_server_handle_read_str(p_json_msg->i_index);

		case GSI_WRITE_STR:
			return gsi_server_handle_write_str(p_json_msg->i_index, p_json_msg->s_data, p_json_msg->i_data_len);

		case GSI_READ_FILE:
			return gsi_server_handle_read_file(p_json_msg->s_file_name, GSI_IS_NO_PRINT);

		case GSI_WRITE_FILE:
			return gsi_server_handle_write_file(p_json_msg->s_file_name, p_json_msg->s_data);

		case GSI_PRINT_LOG:
			return gsi_server_handle_print_log(p_json_msg->s_file_name);

		case GSI_READ_FILE_BY_ID:
			return gsi_server_handle_read_file_by_id(p_json_msg->s_file_name, atoi(p_json_msg->s_data));

		default:
			return GSI_IS_FAIL;
	}
}

/*###########################################################################
	 * Name:		gsi_server_handle_read_str
	 * Description: Handle the Read Str op-code and print the: g_arr_strings[i_index]
	 * Parameter:   [in] int i_index - index in global array to read its string
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_handle_read_str(int i_index)
{
	// Check input validation
	if ((0 > i_index) || (GSI_IS_MAX_STRINGS < i_index))
	{
		LOG_ERROR("index %d is out of range", i_index);
		return GSI_IS_FAIL;
	}

	// Print the string to screen
	printf("%s\n", g_arr_strings[i_index]);

	return 0;
}

/*###########################################################################
	 * Name:		gsi_server_handle_write_str
	 * Description:	Handle the Write Str op-code and write the s_new_str into g_arr_strings[i_index]
	 * 				if the new string length is bigger than the current - the function use realloc
	 * Parameter:   [in] int i_index - index in global array to write into
	 * Parameter:   [in] char* s_new_str - the new string to insert
	 * Parameter:   [in] int i_len - the length of new_str
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_handle_write_str(int i_index, char* s_new_str, int i_len)
{
	// Check input validation
	if ((0 > i_index) || (GSI_IS_MAX_STRINGS < i_index))
	{
		LOG_ERROR("index %d is out of range", i_index);
		return GSI_IS_FAIL;
	}

	// Check if need to use realloc
	if (i_len > strlen(g_arr_strings[i_index]))
	{
		g_arr_strings[i_index] = realloc(g_arr_strings[i_index], i_len);
		if (NULL == g_arr_strings[i_index])
		{
			LOG_ERROR("memory reallocation failed");
			return GSI_IS_FAIL;
		}
	}

	// Copy new string content
	strcpy(g_arr_strings[i_index], s_new_str);

	return 0;
}

/*###########################################################################
	 * Name:		gsi_server_handle_read_file
	 * Description: Handle the Read File op-code and read the file's content
	 * Parameter:   [in] char* s_file_name - file to read
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_handle_read_file(char* s_file_name, int flags)
{
	int i_index = 0;
	char s_buffer[GSI_IS_MAX_STRINGS][GSI_IS_MAX_STR_LEN];
	FILE* f_read_file = NULL;

	// Check input validation
	if (NULL == s_file_name)
	{
		LOG_ERROR("invalid argument!");
		return GSI_IS_FAIL;
	}

	// Open file to read from it
	f_read_file = fopen(s_file_name, "r");
	if (NULL == f_read_file)
	{
		perror("open: ");
		LOG_ERROR("failed to open %s", s_file_name);
		return GSI_IS_FAIL;
	}

	// Reset buffer
	memset(s_buffer, 0, sizeof(s_buffer));

	// Main loop to read up to GSI_IS_MAX_STRINGS and print them
	for (i_index = 0; i_index < GSI_IS_MAX_STRINGS; ++i_index)
	{
		// Reset buffer
		memset(s_buffer[i_index], 0, sizeof(s_buffer[i_index]));

		// Read line from file
		if (NULL == fgets(s_buffer[i_index], GSI_IS_MAX_STR_LEN, f_read_file))
		{
			break;
		}
	}

	// Check if the user want to print to screen
	if (GSI_IS_PRINT_SCREEN == flags)
	{
		for (int i = 0; i < GSI_IS_MAX_STRINGS; ++i)
		{
			printf("%s", s_buffer[i]);
		}
	}

	// Close file
	fclose(f_read_file);

	return 0;
}

/*###########################################################################
	 * Name:		gsi_server_handle_write_file
	 * Description: Handle the Write File op-code and write string into file
	 * Parameter:   [in] char* s_file_name - target file name
	 * Parameter:   [in] char* s_msg - new message to insert
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_handle_write_file(char* s_file_name, char* s_msg)
{
	// Check input validation
	if ((NULL == s_file_name) || (NULL == s_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_IS_FAIL;
	}

	// Open source file
	FILE* f_target = fopen(s_file_name, "a+");
	if (NULL == f_target)
	{
		LOG_ERROR("failed to open %s", s_file_name);
		return GSI_IS_FAIL;
	}

	// Insert meesage into file
	fputs(s_msg, f_target);

	// Close file
	fclose(f_target);

	return 0;
}

/*###########################################################################
	 * Name:		gsi_server_handle_print_log
	 * Description: Handle the Print Log op-code and print it to screen
	 * Parameter:   [in] char* s_file_name - log file to print
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_handle_print_log(char* s_file_name)
{
	return gsi_server_handle_read_file(s_file_name, GSI_IS_PRINT_SCREEN);
}

/*###########################################################################
	 * Name:		gsi_server_handle_read_file_by_id
	 * Description: Open file and search for message with specific id and print to screen
	 * Parameter:   [in] char* s_file_name - file to open for search
	 * Parameter:   [in] int i_id - message id
	 * Return:		Success - 0
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_handle_read_file_by_id(char* s_file_name, int i_id)
{
	char s_buffer[GSI_IS_MAX_BUF_SIZE];
	char* s_res = s_buffer;
	int i_current_id = 0;
	int i_found = GSI_IS_FALSE;

	// Check input validation
	if ((NULL == s_file_name) || (0 > i_id))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_IS_FAIL;
	}

	// Open source file
	FILE* f_target = fopen(s_file_name, "r");
	if (NULL == f_target)
	{
		LOG_ERROR("failed to open %s", s_file_name);
		return GSI_IS_FAIL;
	}

	// Main loop - run until found the message with the match id
	while (1)
	{
		// Reset buffer
		memset(s_buffer, 0, sizeof(s_buffer));

		// Read string from file into buffer
		if (NULL == fgets(s_buffer, GSI_IS_MAX_BUF_SIZE, f_target))
		{
			break;
		}

		// Extract the id from the string
		i_current_id = strtol(s_buffer, &s_res, 10);

		// Check if match
		if (i_current_id == i_id)
		{
			i_found = GSI_IS_TRUE;
			printf("message id: %d\ncontent: %s", i_id, s_res);
			break;
		}
	}

	// Check if not found any message
	if (GSI_IS_FALSE == i_found)
	{
		LOG_WARNING("not found message with id: %d in file: %s", i_id, s_file_name);
	}

	// Close file
	fclose(f_target);

	return 0;
}

/*###########################################################################
	 * Name:		gsi_server_port_to_client
	 * Description: Convert port number to client number
	 * Parameter:   [in] unsigned int ui_port - port number
	 * Return:		Success - Number representation of client
	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
static int gsi_server_port_to_client(unsigned int ui_port)
{
	if (ui_port == g_config_server_params.ui_port1)
	{
		return 1;
	}
	if (ui_port == g_config_server_params.ui_port2)
	{
		return 2;
	}
	if (ui_port == g_config_server_params.ui_port3)
	{
		return 3;
	}

	return GSI_IS_FAIL;
}
