/**************************************************************************
* Name : gsi_parse_json_config.h 
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : API for read parameters from configuration file.
* 				lines are in that format: <KEY>:<VALUE> (no spaces).
* 				lines starts with '#' consider as comment line.
* 				On command line: ./<a.out> --cfg=[config_file]
* 				if no config file supplied by command line - default parameters will be set
* 				The client must supply config file
* 				The server is optional
* 				General usage:
* 					call to: gsi_parse_json_config_get_config(int i_argc, char* p_argv[]) in your code.
* 					pass it the following arguments:
* 					1. argc from main
* 					2. argv from main
*
*****************************************************************************/
#ifndef GSI_PARSE_JSON_CONFIG_H_
#define GSI_PARSE_JSON_CONFIG_H_

/* Includes */
#include <stdio.h>

/* Defines and Macros */
#define  GSI_PARSE_JSON_CONFIG_MAX_FILE_NAME 128
#define  GSI_PARSE_JSON_CONFIG_IP_LEN		 sizeof("255.255.255.255")

/* Structures */
/*****************************************************************************
 * Name : gsi_prase_json_config_params
 * Used by: Different modules of cs_json_parse project
 * Members:
 *----------------------------------------------------------------------------
 *		unsigned int ui_port1 - port of client 1
 *----------------------------------------------------------------------------
 *		unsigned int ui_port2 - port of client 2
 *----------------------------------------------------------------------------
 *		unsigned int ui_port3 - port of client 3
 *----------------------------------------------------------------------------
 * 		char s_ip - ip address
 *----------------------------------------------------------------------------
 *		int i_server_timer - time server is up
 *----------------------------------------------------------------------------
 *		char* s_server_data_file - strings files of server for its global array
 *----------------------------------------------------------------------------
*****************************************************************************/
struct gsi_prase_json_config_server_params
{
	unsigned int ui_port1;
	unsigned int ui_port2;
	unsigned int ui_port3;
	int i_server_timer;
	char s_ip[GSI_PARSE_JSON_CONFIG_IP_LEN];
	char s_server_data_file[GSI_PARSE_JSON_CONFIG_MAX_FILE_NAME];
};

/*****************************************************************************
 * Name : gsi_prase_json_config_client_params
 * Used by: Different modules of cs_json_parse project
 * Members:
*----------------------------------------------------------------------------
 *		unsigned int ui_port1 - port of client
 *----------------------------------------------------------------------------
 *		char s_ip - ip address
 *----------------------------------------------------------------------------
 *		char* s_messages_file - messages file of client
 *----------------------------------------------------------------------------
*****************************************************************************/
struct gsi_prase_json_config_client_params
{
	unsigned int ui_port;
	char s_ip[GSI_PARSE_JSON_CONFIG_IP_LEN];
	char s_messages_file[GSI_PARSE_JSON_CONFIG_MAX_FILE_NAME];
};

/* Enums */
/***************************************************************************
 * Name:		gsi_prase_json_config_rc
 * Description: Return Code values for GSI-PARSE_JSON-CONFIG functions.
 ***************************************************************************/
enum gsi_prase_json_config_rc
{
	GSI_PARSE_JSON_CONFIG_INVALID = -1,
	GSI_PARSE_JSON_CONFIG_SUCCESS,
	GSI_PARSE_JSON_CONFIG_OPEN_ERR,
	GSI_PARSE_JSON_CONFIG_CLOSE_ERR,
	GSI_PARSE_JSON_CONFIG_READ_ERR,
	GSI_PARSE_JSON_CONFIG_EOF,
	GSI_PARSE_JSON_CONFIG_NOT_FOUND,
	GSI_PARSE_JSON_CONFIG_ERROR
};

/***************************************************************************
 * Name:		gsi_prase_json_config_opts
 * Description: Codes of configurable parameters to the program.
 ***************************************************************************/
enum gsi_prase_json_config_opts
{
	// Server parameters
	GSI_PARSE_JSON_PARAM_SERVER_PORT_1,
	GSI_PARSE_JSON_PARAM_SERVER_PORT_2,
	GSI_PARSE_JSON_PARAM_SERVER_PORT_3,
	GSI_PARSE_JSON_PARAM_SERVER_IP,
	GSI_PARSE_JSON_PARAM_SERVER_TIMER,
	GSI_PARSE_JSON_PARAM_SERVER_DATA,

	// Client parameters
	GSI_PARSE_JSON_PARAM_CLIENT_PORT,
	GSI_PARSE_JSON_PARAM_CLIENT_IP,
	GSI_PARSE_JSON_PARAM_CLIENT_MSG,
};

/*******************/
/* API Declaration */
/*******************/
/*###########################################################################
	 * Name:		gsi_parse_json_config_open
	 * Description: Open configuration file to read its parameters.
	 * 				Must be cleanup by gsi_parse_json_config_close()
	 * Parameter:   [in] char* s_config_file - configuration file to open
	 * Return:		Success - pointer to open file
	 * 				Failure - NULL
#############################################################################*/
FILE* gsi_parse_json_config_open(char* s_config_file);


/*###########################################################################
	 * Name:		gsi_parse_json_config_close
	 * Description: Close configuration file at the end of its use
	 * Parameter:   [in] FILE* f_config - pointer to open file to be closed
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_CLOSE_ERR *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_close(FILE* f_config);


/*###########################################################################
	 * Name:		gsi_parse_json_config_read_line
	 * Description: Read the next line from configuration file <KEY>:<VALUE>
	 * Parameter:   [in] FILE* f_config - pointer to open file to read from
	 * Parameter:   [out] char* s_key - name of the parameter
	 * Parameter:   [out] char* s_value - value of the parameter
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_READ_ERR *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_read_line(FILE* f_config, char* s_key, char* s_value);


/*###########################################################################
	 * Name:		gsi_parse_json_config_get_entry
	 * Description: Get the entry index of given key parameter to set its value
	 * Parameter:   [in] char* s_key - name of the parameter to get its entry
	 * Parameter:   [out] int* p_index - index of of given key OR -1 (if not found)
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_NOT_FOUND *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_get_next_entry(char* s_key, int* p_index);


/*###########################################################################
	 * Name:		gsi_parse_json_config_get_config
	 * Description: Read configuration parameters according to cmd arguments
	 * 				Initialize to defaults if no cmd arguments supplied
	 * Parameter:   [in] int i_argc - number of arguments in command line (argc)
	 * Parameter:   [in] char* p_argv[] - vector of arguments from command line (argv)
	 * Return:		Success - GSI_PARSE_JSON_CONFIG_SUCCESS
	 * 				Failure - GSI_PARSE_JSON_CONFIG_OPEN_ERR *OR* GSI_PARSE_JSON_CONFIG_INVALID
#############################################################################*/
enum gsi_prase_json_config_rc gsi_parse_json_config_get_config(int i_argc, char* p_argv[]);


#endif /* GSI_PARSE_JSON_CONFIG_H_ */
