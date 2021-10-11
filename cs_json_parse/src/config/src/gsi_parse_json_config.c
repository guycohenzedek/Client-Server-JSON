/**************************************************************************
* Name : gsi_parse_json_config.c
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Implementation of "gsi_parse_json_config.h" API
*****************************************************************************/

/* Includes */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "gsi_is_log_api.h"
#include "gsi_parse_json_config.h"

/* Defines and Macros */
#define 	GSI_PARSE_JSON_CONFIG_MAX_LINE 		256
#define 	GSI_PARSE_JSON_CONFIG_MAX_KEY_LEN 	128
#define 	GSI_PARSE_JSON_CONFIG_MAX_VALUE_LEN GSI_PARSE_JSON_CONFIG_MAX_KEY_LEN

/********************************/
/* Static functions declaration */
/********************************/
static void gsi_parse_json_config_parse_main_args(int i_index, char* s_value);
static void gsi_parse_json_config_init_default_params();
static enum gsi_prase_json_config_rc gsi_parse_json_config_init_from_config_file(char* s_config_file);

/* Global variables */
struct gsi_prase_json_config_server_params g_config_server_params = {0};
struct gsi_prase_json_config_client_params g_config_client_params = {0};

// commmand line argument options for getopt_long()
static struct option g_config_cmd_opts = {"cfg", optional_argument, NULL, 0};

// mapping key string to value index
static char* g_config_params_keys[] =
{
	// Server parameters
	[GSI_PARSE_JSON_PARAM_SERVER_PORT_1] 		= "server_port_1",
	[GSI_PARSE_JSON_PARAM_SERVER_PORT_2] 		= "server_port_2",
	[GSI_PARSE_JSON_PARAM_SERVER_PORT_3]		= "server_port_3",
	[GSI_PARSE_JSON_PARAM_SERVER_IP]			= "server_ip",
	[GSI_PARSE_JSON_PARAM_SERVER_TIMER]   		= "server_timer",
	[GSI_PARSE_JSON_PARAM_SERVER_DATA] 			= "server_data",

	// Client parameters
	[GSI_PARSE_JSON_PARAM_CLIENT_PORT]  		= "client_port",
	[GSI_PARSE_JSON_PARAM_CLIENT_IP]			= "client_ip",
	[GSI_PARSE_JSON_PARAM_CLIENT_MSG] 	  		= "client_messages",
};

/**********************/
/* API implementation */
/**********************/
/*###########################################################################
	 * Name:		gsi_parse_json_config_open
	 * Description: Open configuration file to read its parameters.
	 * 				Must be cleanup by gsi_parse_json_config_close()
	 * Parameter:   [in] char* s_config_file - configuration file to open
	 * Return:		Success - pointer to open file
	 * 				Failure - NULL
#############################################################################*/
FILE* gsi_parse_json_config_open(char* s_config_file)
{
	FILE* f_config = NULL;

	// Check input validation
	if (NULL == s_config_file)
	{
		LOG_ERROR("invalid configuration file");
		return NULL;
	}

	// Open the configuration file
	f_config = fopen(s_config_file, "r");
	if (NULL == f_config)
	{
		LOG_ERROR("failed to open: %s", s_config_file);
		return NULL;
	}

	return f_config;
}

/*###########################################################################
	 * Name:		gsi_parse_json_config_close
	 * Description: Close configuration file at the end of its use
	 * Parameter:   [in] FILE* f_config - pointer to open file to be closed
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_CLOSE_ERR *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_close(FILE* f_config)
{
	// Check input validation
	if (NULL == f_config)
	{
		LOG_ERROR("invalid argument");
		return GSI_PARSE_JSON_CONFIG_INVALID;
	}

	// Close configuration file
	if (0 != fclose(f_config))
	{
		LOG_ERROR("failed to close configuration file");
		return GSI_PARSE_JSON_CONFIG_CLOSE_ERR;
	}

	return GSI_PARSE_JSON_CONFIG_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_parse_json_config_read_line
	 * Description: Read the next line from configuration file <KEY>:<VALUE>
	 * Parameter:   [in] FILE* f_config - pointer to open file to read from
	 * Parameter:   [out] char* s_key - buffer to get the name of the parameter
	 * Parameter:   [out] char* s_value - buffer to get the value of the parameter
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_READ_ERR *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_read_line(FILE* f_config, char* s_key, char* s_value)
{
	char s_line[GSI_PARSE_JSON_CONFIG_MAX_LINE];
	char* s_separator = NULL;

	// Check input validation
	if ((NULL == f_config) || (NULL == s_key) || (NULL == s_value))
	{
		LOG_ERROR("read_line: invalid arguments");
		return GSI_PARSE_JSON_CONFIG_INVALID;
	}

	// Main loop - skip lines starts with '#' or empty lines
	do
	{
		// Reset buffer
		memset(s_line, 0, sizeof(s_line));

		// Read line from file
		if (NULL == fgets(s_line, GSI_PARSE_JSON_CONFIG_MAX_LINE, f_config))
		{
			// Check if get EOF
			if (feof(f_config))
			{
				LOG_WARNING("get EOF from config file");
				return GSI_PARSE_JSON_CONFIG_EOF;
			}
			else
			{
				LOG_ERROR("couldn't read from file");
				return GSI_PARSE_JSON_CONFIG_READ_ERR;
			}
		}
	}
	while (('#' == s_line[0]) || ('\n' == s_line[0]));

	// Get the position of ':' char in the current line
	s_separator = strchr(s_line, ':');
	if (NULL == s_separator)
	{
		LOG_ERROR("there is no separator ':' in the line");
		return GSI_PARSE_JSON_CONFIG_READ_ERR;
	}

	// Break the line into <KEY> <VALUE>
	*s_separator = 0;

	// Set the buffers
	strcpy(s_key, s_line);
	strcpy(s_value, ++s_separator);

	return GSI_PARSE_JSON_CONFIG_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_parse_json_config_get_next_entry
	 * Description: Get the entry index of given key parameter to set its value
	 * Parameter:   [in] char* s_key - name of the parameter to get its entry
	 * Parameter:   [out] int* p_index - index of of given key OR -1 (if not found)
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_NOT_FOUND *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_get_next_entry(char* s_key, int* p_index)
{
	// Check input validation
	if ((NULL == s_key) || (NULL == p_index))
	{
		LOG_ERROR("invalid arguments");
		return GSI_PARSE_JSON_CONFIG_INVALID;
	}

	// Search the key in the map
	for (int i_option = 0; g_config_params_keys[i_option] != NULL; ++i_option)
	{
		if (0 == strcmp(s_key, g_config_params_keys[i_option]))
		{
			*p_index = i_option;
			return GSI_PARSE_JSON_CONFIG_SUCCESS;
		}
	}

	// Not found so index is -1
	*p_index = -1;
	LOG_WARNING("key: %s not found", s_key);

	return GSI_PARSE_JSON_CONFIG_NOT_FOUND;
}

/*###########################################################################
	 * Name:		gsi_parse_json_config_get_config
	 * Description: Read configuration parameters according to cmd arguments
	 * 				Initialize to defaults if no cmd arguments supplied
	 * Parameter:   [in] int i_argc - number of arguments in command line (argc)
	 * Parameter:   [in] char* p_argv[] - vector of arguments from command line (argv)
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_OPEN_ERR *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_get_config(int i_argc, char* p_argv[])
{
	int i_option_index = 0;
	int i_rc = 0;

	// Check input validation
	if ((0 > i_argc) || (NULL == p_argv))
	{
		LOG_ERROR("invalid arguments");
		return GSI_PARSE_JSON_CONFIG_INVALID;
	}

	// Parse command line arguments by getopt_long(), the argument is optional
	i_rc = getopt_long(i_argc, p_argv, "c::", &g_config_cmd_opts, &i_option_index);
	if (-1 == i_rc)
	{
		LOG_ERROR("usage error: <a.out> --cfg=[file]");
		return GSI_PARSE_JSON_CONFIG_ERROR;
	}

	// No argument supplied so run default configuration.
	if (0 == strcmp("", optarg))
	{
		gsi_parse_json_config_init_default_params();
	}
	// Read from config file given in command line
	else
	{
		if (GSI_PARSE_JSON_CONFIG_SUCCESS != gsi_parse_json_config_init_from_config_file(optarg))
		{
			LOG_ERROR("failed to config parameters from file %s", optarg);
			return GSI_PARSE_JSON_CONFIG_ERROR;
		}
	}

	return GSI_PARSE_JSON_CONFIG_SUCCESS;
}

/***********************************/
/* Static functions implementation */
/***********************************/
/*###########################################################################
	 * Name:		gsi_parse_json_config_parse_main_args
	 * Description: Set parameter value at specific index
	 * Parameter:   [in] int i_index - index of the parameter to set its value
	 * Parameter:   [in] char* s_value - value to set
	 * Return:		None
#############################################################################*/
static void gsi_parse_json_config_parse_main_args(int i_index, char* s_value)
{
	// Check input validation
	if ((0 > i_index) || (NULL == s_value))
	{
		LOG_ERROR("invalid arguments");
		return;
	}

	switch(i_index)
	{
		// Server parameters
		case GSI_PARSE_JSON_PARAM_SERVER_PORT_1:
			g_config_server_params.ui_port1 = atoi(s_value);
			LOG_DEBUG("server_port_1: %d", g_config_server_params.ui_port1);
			break;

		case GSI_PARSE_JSON_PARAM_SERVER_PORT_2:
			g_config_server_params.ui_port2 = atoi(s_value);
			LOG_DEBUG("server_port_2: %d", g_config_server_params.ui_port2);
			break;

		case GSI_PARSE_JSON_PARAM_SERVER_PORT_3:
			g_config_server_params.ui_port3 = atoi(s_value);
			LOG_DEBUG("server_port_3: %d", g_config_server_params.ui_port3);
			break;

		case GSI_PARSE_JSON_PARAM_SERVER_IP:
			strcpy(g_config_server_params.s_ip ,s_value);
			// Replace the '\n' by '\0'
			g_config_server_params.s_ip[strlen(g_config_server_params.s_ip) - 1] = '\0';
			LOG_DEBUG("server_ip: %s", g_config_server_params.s_ip);
			break;

		case GSI_PARSE_JSON_PARAM_SERVER_TIMER:
			g_config_server_params.i_server_timer = atoi(s_value);
			LOG_DEBUG("server_timer: %d", g_config_server_params.i_server_timer);
			break;

		case GSI_PARSE_JSON_PARAM_SERVER_DATA:
			strcpy(g_config_server_params.s_server_data_file, s_value);
			// Replace the '\n' by '\0'
			g_config_server_params.s_server_data_file[strlen(g_config_server_params.s_server_data_file) - 1] = '\0';
			LOG_DEBUG("server_data: %s", g_config_server_params.s_server_data_file);
			break;

		// Client parameters
		case GSI_PARSE_JSON_PARAM_CLIENT_PORT:
			g_config_client_params.ui_port = atoi(s_value);
			LOG_DEBUG("client_port: %d", g_config_client_params.ui_port);
			break;

		case GSI_PARSE_JSON_PARAM_CLIENT_IP:
			strcpy(g_config_client_params.s_ip ,s_value);
			// Replace the '\n' by '\0'
			g_config_client_params.s_ip[strlen(g_config_client_params.s_ip) - 1] = '\0';
			LOG_DEBUG("client_ip: %s", g_config_client_params.s_ip);
			break;

		case GSI_PARSE_JSON_PARAM_CLIENT_MSG:
			strcpy(g_config_client_params.s_messages_file ,s_value);
			// Replace the '\n' by '\0'
			g_config_client_params.s_messages_file[strlen(g_config_client_params.s_messages_file) - 1] = '\0';
			LOG_DEBUG("client_messages: %s", g_config_client_params.s_messages_file);
			break;

		default:
			LOG_ERROR("index is not match to any option");
	}
}

/*###########################################################################
	 * Name:		gsi_parse_json_config_init_default_params
	 * Description: Initialize default parameters to server structure
	 * Parameter:   [in] None
	 * Return:		None
#############################################################################*/
static void gsi_parse_json_config_init_default_params()
{
	// Server parameters
	g_config_server_params.ui_port1 = 65533;
	g_config_server_params.ui_port2 = 65534;
	g_config_server_params.ui_port3 = 65535;
	g_config_server_params.i_server_timer = 0;

	strcpy(g_config_server_params.s_ip, "127.0.0.1");
	strcpy(g_config_server_params.s_server_data_file, "../src/server/test_files/server_data.txt");
}

/*###########################################################################
	 * Name:		gsi_parse_json_config_init_from_config_file
	 * Description: Initialize configuration parameters from given file
	 * Parameter:   [in] char* s_config_file - configuration file
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_OPEN_ERR *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
static enum gsi_prase_json_config_rc gsi_parse_json_config_init_from_config_file(char* s_config_file)
{
	int i_param_index = 0;
	int i_rc = GSI_PARSE_JSON_CONFIG_SUCCESS;
	char s_key[GSI_PARSE_JSON_CONFIG_MAX_KEY_LEN];
	char s_value[GSI_PARSE_JSON_CONFIG_MAX_VALUE_LEN];
	FILE* f_config = NULL;

	// Check input validation
	if (NULL == s_config_file)
	{
		LOG_ERROR("invalid argument");
		return GSI_PARSE_JSON_CONFIG_INVALID;
	}

	// Open config file
	f_config = gsi_parse_json_config_open(s_config_file);
	if (NULL == f_config)
	{
		return GSI_PARSE_JSON_CONFIG_OPEN_ERR;
	}

	// Main loop to read all config parameters
	while (1)
	{
		// Reset buffers
		memset(s_key, 0, sizeof(s_key));
		memset(s_value, 0, sizeof(s_value));

		// Read the next line from config file
		i_rc = gsi_parse_json_config_read_line(f_config, s_key, s_value);
		if (GSI_PARSE_JSON_CONFIG_SUCCESS != i_rc)
		{
			break;
		}

		// Get the key index in map
		if (GSI_PARSE_JSON_CONFIG_SUCCESS != gsi_parse_json_config_get_next_entry(s_key, &i_param_index))
		{
			i_rc = GSI_PARSE_JSON_CONFIG_NOT_FOUND;
			break;
		}

		// Set value at given index
		gsi_parse_json_config_parse_main_args(i_param_index, s_value);
	}

	// Close config file
	gsi_parse_json_config_close(f_config);

	if (GSI_PARSE_JSON_CONFIG_EOF == i_rc)
	{
		i_rc = GSI_PARSE_JSON_CONFIG_SUCCESS;
	}

	return i_rc;
}
