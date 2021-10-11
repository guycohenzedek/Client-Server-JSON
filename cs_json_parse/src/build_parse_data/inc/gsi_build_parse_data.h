/**************************************************************************
* Name : gsi_build_parse_data.h 
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : API to send and receive messages between client and server
* 				Using JSON format.
* 				Client messages file has the following format for each line:
* 				'M' - Regular message
* 				'H' - Heartbeat message
* 				'#' - comment line
* 				"RS" - Read String
* 				"WS" - Write String
* 				"RF" - Read File
* 				"WF" - write File
* 				"PL" - Print Log
*				"RFID" - Read message from file by id
*
* 				Examples:
* 				"M:RS:2" - Regular message that will read string in index 2
* 				"H:WD" 	 - HeartBeat message
* 				"M:WS:3 new-message" - Regular message that will write the "new-message" in index 3
* 				"M:RF <file_name>" - Regular message that will read all content of file_name
* 				"M:WF <target_file_name> <msg_id> <msg>" - Regular message with id that write into target file
* 				"M:PL <file_name>" - Regular message that will print log file to screen
* 				"M:RFID <file name> <msg id> - Regular message that will search message in file according to id and print to screen
*****************************************************************************/
#ifndef GSI_BUILD_PARSE_DATA_H_
#define GSI_BUILD_PARSE_DATA_H_

/* Includes */
#include <stdio.h>
#include "gsi_is_network_tcp.h"

/* Defines and Macros */
#define 	GSI_IS_HEART_BEAT      'H'
#define 	GSI_IS_MESSAGE	       'M'
#define 	GSI_IS_COMMENT	       '#'
#define 	GSI_IS_READ_STR        "RS"
#define 	GSI_IS_WRITE_STR       "WS"
#define		GSI_IS_READ_FILE       "RF"
#define 	GSI_IS_WRITE_FILE  	   "WF"
#define 	GSI_IS_PRINT_LOG  	   "PL"
#define		GSI_IS_READ_FILE_BY_ID "RFID"

/* Structures */
/*****************************************************************************
 * Name : gsi_json_msg
 * Used by:	Server and Client over TCP
 * Members:
 *----------------------------------------------------------------------------
 *		int i_msg_type		  - message type: HEARTBEAT or MESSAGE
 *----------------------------------------------------------------------------
 *		int i_op_code		  - operation code
 *----------------------------------------------------------------------------
 *		unsigned int ui_port  - Port number
 *----------------------------------------------------------------------------
 *		int i_index			  - index in server array to read/write messages
 *----------------------------------------------------------------------------
 *		int	i_data_len		  - message length
 *----------------------------------------------------------------------------
 *		int i_file_len		  - file name length
 *----------------------------------------------------------------------------
 *		char* s_file_name	  - file name to read from / write to
 *----------------------------------------------------------------------------
 *		char* s_data		  - message content
 *****************************************************************************/
struct gsi_json_msg
{
	int i_msg_type;
	int i_op_code;
	unsigned int ui_port;
	int i_index;
	int i_data_len;
	int i_file_len;
	char* s_file_name;
	char* s_data;
};

/* Enums */
/***************************************************************************
 * Name:		gsi_is_json_rc
 * Description: Return values of gsi_build_parse_data.h functions
 ***************************************************************************/
enum gsi_is_json_rc
{
	GSI_JSON_ERROR = -1,
	GSI_JSON_SUCCESS,
	GSI_JSON_INVALID_ERR,
	GSI_JSON_OPEN_ERR,
	GSI_JSON_CLOSE_ERR,
	GSI_JSON_READ_ERROR
};

/***************************************************************************
 * Name:		gsi_is_op_codes_parse_build
 * Description: Operations Code to send between client and server
 ***************************************************************************/
enum gsi_is_op_codes_parse_build
{
	GSI_READ_STR,
	GSI_WRITE_STR,
	GSI_READ_FILE,
	GSI_WRITE_FILE,
	GSI_PRINT_LOG,
	GSI_READ_FILE_BY_ID
};


/*******************/
/* API Declaration */
/*******************/
/*###########################################################################
	 * Name:       	gsi_is_open_msg_file
	 * Description: Open messages file of client. MUST be closed by gsi_is_close_msg_file()
	 * Parameter:   [in] const char* s_file_name - file's name to open in READ mode
	 * Return:		Success - FILE* - handler to opened file
	 * 				Failure - NULL
#############################################################################*/
FILE* gsi_is_open_msg_file(const char* s_file_name);


/*###########################################################################
	 * Name:        gsi_is_close_msg_file
	 * Description: Close messages file that opened by gsi_is_open_msg_file()
	 * Parameter:   [in] FILE* f_msg_file - handler to opened file
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_CLOSE_ERR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_close_msg_file(FILE* f_msg_file);


/*###########################################################################
	 * Name:		gsi_build_parse_reset_object
	 * Description: Reset all the fields of struct gsi_json_msg
	 * Parameter:   [in-out] struct gsi_json_msg* p_json_msg - pointer to reset
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_build_parse_reset_object(struct gsi_json_msg* p_json_msg);


/*###########################################################################
	 * Name:		gsi_is_send_all_json_msg
	 * Description: Send all messages that exist in f_msg_file
	 * Parameter:   [in] FILE* f_msg_file - handler to opened file
	 * Parameter:   [in] struct gsi_net_tcp* p_client - client that wants to send the messages
	 * Return :		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_send_all_json_msg(FILE* f_msg_file, struct gsi_net_tcp* p_client);


/*###########################################################################
	 * Name:		gsi_is_send_json_msg
	 * Description: Send one message from client to server
	 * Parameter:   [in] struct gsi_net_tcp* p_client - client that wants to send the message
	 * Parameter:   [in] struct gsi_json_msg* p_json_msg - pointer to message structure
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_send_json_msg(struct gsi_net_tcp* p_client, struct gsi_json_msg* p_json_msg);


/*###########################################################################
	 * Name:		gsi_is_recv_json_msg
	 * Description: Receive one message on listening port, and make operation according to the OP_CODE
	 * Parameter:   [in] struct gsi_net_tcp* p_server - server that listen to port
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to message structure
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_recv_json_msg(struct gsi_net_tcp* p_server, struct gsi_json_msg* p_json_msg);


/*###########################################################################
	 * Name:		gsi_is_get_next_msg
	 * Description:	Get the next message in f_msg_file
	 * Parameter:   [in] FILE* f_msg_file - handler to opened file
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to message structure
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_get_next_msg(FILE* f_msg_file, struct gsi_json_msg* p_json_msg);


#endif /* GSI_BUILD_PARSE_DATA_H_ */
