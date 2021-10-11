/**************************************************************************
* Name : gsi_build_parse_data.c
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Implementation of "gsi_build_parse_data.h"
*****************************************************************************/

/* Includes */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <json-c/json.h>
#include "gsi_is_log_api.h"
#include "gsi_build_parse_data.h"

/* Defines and Macros */
#define 	GSI_IS_BUFFER_SIZE    1024

/********************************/
/* Static functions declaration */
/********************************/
static int gsi_build_parse_get_msg_len(char* s_msg);
static int gsi_build_parse_get_msg_type(char** s_line);
static int gsi_build_parse_get_msg_index(char** s_line);
static int gsi_build_parse_get_msg_op_code(char** s_line);
static int gsi_build_parse_build_msg(char* s_line, struct gsi_json_msg* p_json_msg);
static int gsi_build_parse_string_to_json_object(char *s_data, struct json_object** p_json);
static int gsi_build_parse_set_op_code_args(char** s_line, struct gsi_json_msg* p_json_msg);
static int gsi_build_parse_handle_op_code(struct json_object *p_json, struct gsi_json_msg* p_json_msg);
static int gsi_build_parse_json_object_to_json_msg(struct json_object *p_json, struct gsi_json_msg* p_json_msg);

static char* gsi_build_parse_op_code_to_string(int i_op_code);
static char* gsi_build_parse_strdup(const char* s_src);
static char* gsi_build_parse_get_file_name(char** s_line);
static char* gsi_build_parse_get_msg_content(char** s_line);
static char* gsi_build_parse_json_obj_to_string(struct json_object *p_json, struct gsi_json_msg* p_json_msg);

/**********************/
/* API implementation */
/**********************/
/*###########################################################################
	 * Name:       	gsi_is_open_msg_file
	 * Description: Open messages file of client. MUST be closed by gsi_is_close_msg_file()
	 * Parameter:   [in] const char* s_file_name - file's name to open in READ mode
	 * Return:		Success - FILE* - handler to opened file
	 * 				Failure - NULL
#############################################################################*/
FILE* gsi_is_open_msg_file(const char* s_file_name)
{
	FILE* f_msg_file = NULL;

	// Check input validation
	if (NULL == s_file_name)
	{
		LOG_ERROR("invalid argument!");
		return NULL;
	}

	// Open messages file
	f_msg_file = fopen(s_file_name, "r");
	if (NULL == f_msg_file)
	{
		LOG_ERROR("couldn't open file: %s", s_file_name);
	}

	return f_msg_file;
}

/*###########################################################################
	 * Name:        gsi_is_close_msg_file
	 * Description: Close messages file that opened by gsi_is_open_msg_file()
	 * Parameter:   [in] FILE* f_msg_file - handler to opened file
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_CLOSE_ERR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_close_msg_file(FILE* f_msg_file)
{
	// Check input validation
	if (NULL == f_msg_file)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_INVALID_ERR;
	}

	// Close file
	if (0 != fclose(f_msg_file))
	{
		LOG_ERROR("couldn't close messages file");
		return GSI_JSON_CLOSE_ERR;
	}

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_build_parse_reset_object
	 * Description: Reset all the fields of struct gsi_json_msg
	 * Parameter:   [in-out] struct gsi_json_msg* p_json_msg - pointer to reset
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_build_parse_reset_object(struct gsi_json_msg* p_json_msg)
{
	// Check input validation
	if (NULL == p_json_msg)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_INVALID_ERR;
	}

	// Check if need to free s_file_name
	if (NULL != p_json_msg->s_file_name)
	{
		free(p_json_msg->s_file_name);
	}

	// Check if need to free s_data
	if (NULL != p_json_msg->s_data)
	{
		free(p_json_msg->s_data);
	}

	// Reset fields
	memset(p_json_msg, 0, sizeof(struct gsi_json_msg));

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_send_all_json_msg
	 * Description: Send all messages that exist in f_msg_file
	 * Parameter:   [in] FILE* f_msg_file - handler to opened file
	 * Parameter:   [in] struct gsi_net_tcp* p_client - client that wants to send the messages
	 * Return :		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_send_all_json_msg(FILE* f_msg_file, struct gsi_net_tcp* p_client)
{
	struct gsi_json_msg json_msg;
	int i_rc = GSI_JSON_SUCCESS;

	// Check input validation
	if ((NULL == f_msg_file) || (NULL == p_client))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_JSON_INVALID_ERR;
	}

	// Reset fields
	memset(&json_msg, 0, sizeof(json_msg));

	// Main loop to send all messages
	while (1)
	{
		// Get next message from file
		i_rc = gsi_is_get_next_msg(f_msg_file, &json_msg);
		if (GSI_JSON_READ_ERROR == i_rc)
		{
			i_rc = GSI_JSON_SUCCESS;
			break;
		}
		else if ((GSI_JSON_ERROR == i_rc) || (GSI_JSON_INVALID_ERR == i_rc))
		{
			break;
		}

		if (GSI_COMMENT == json_msg.i_msg_type)
		{
			continue;
		}

		// Send message
		if (GSI_JSON_SUCCESS != gsi_is_send_json_msg(p_client, &json_msg))
		{
			i_rc = GSI_JSON_ERROR;
			break;
		}

		// Reset the json-msg object
		gsi_build_parse_reset_object(&json_msg);
	}

	// Reset the json-msg object
	if (GSI_JSON_SUCCESS != gsi_build_parse_reset_object(&json_msg))
	{
		return GSI_JSON_ERROR;
	}

	return i_rc;
}

/*###########################################################################
	 * Name:		gsi_is_send_json_msg
	 * Description: Send one message from client to server
	 * Parameter:   [in] struct gsi_net_tcp* p_client - client that wants to send the message
	 * Parameter:   [in] struct gsi_json_msg* p_json_msg - pointer to message structure
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_send_json_msg(struct gsi_net_tcp* p_client, struct gsi_json_msg* p_json_msg)
{
	struct gsi_cs_tcp_message msg;
	struct json_object *p_json = NULL;
	char* s_full_object = NULL;

	// Check input validation
	if ((NULL == p_client) || (NULL == p_json_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_JSON_INVALID_ERR;
	}

	// Reset message fields
	memset(&msg, 0, sizeof(msg));

	// Set fields
	msg.e_type_msg = p_json_msg->i_msg_type;
	msg.ui_port = p_client->ui_port;
	p_json_msg->ui_port = p_client->ui_port;

	// Create new json object
	p_json = json_object_new_object();
	if (NULL == p_json)
	{
		LOG_ERROR("allocate new json object failed");
		return GSI_JSON_ERROR;
	}

	// Stringify the json-msg to string. using JSON-C library functions
	// create object for each member field in structure and its value
	s_full_object = gsi_build_parse_json_obj_to_string(p_json, p_json_msg);
	if (NULL == s_full_object)
	{
		LOG_ERROR("json object to string failed");
		json_object_put(p_json);
		return GSI_JSON_ERROR;
	}

	LOG_DEBUG("\nJSON:\n%s\n", s_full_object);

	// Get full length
	msg.ui_len = strlen(s_full_object) + 1;

	// Duplicate the new message
	msg.s_message = gsi_build_parse_strdup(s_full_object);
	if (NULL == msg.s_message)
	{
		LOG_ERROR("memory allocation for s_message failed");
		json_object_put(p_json);
		return GSI_JSON_ERROR;
	}

	// Free the json object
	json_object_put(p_json);

	// Send message to server
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_send(p_client, (char *)&msg))
	{
		LOG_ERROR("send message failed on port %d", p_client->ui_port);

		// Free the message memory
		free(msg.s_message);
		msg.s_message = NULL;

		return GSI_NET_RC_ERROR;
	}

	// Free the message memory
	free(msg.s_message);
	msg.s_message = NULL;

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_recv_json_msg
	 * Description: Receive one message on listening port, and make operation according to the OP_CODE
	 * Parameter:   [in] struct gsi_net_tcp* p_server - server that listen to port
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to message structure
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
enum gsi_is_json_rc gsi_is_recv_json_msg(struct gsi_net_tcp* p_server, struct gsi_json_msg* p_json_msg)
{
	struct gsi_cs_tcp_message msg;
	struct json_object *p_json = NULL;

	// Check input validation
	if ((NULL == p_server) || (NULL == p_json_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_JSON_INVALID_ERR;
	}

	// Read new message
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_server_read(p_server, (char *)&msg))
	{
		LOG_ERROR("server read on port %d failed", p_server->ui_port);

		// Check if need to free s_message
		if (NULL != msg.s_message)
		{
			free(msg.s_message);
			msg.s_message = NULL;
		}

		return GSI_JSON_ERROR;
	}

	LOG_DEBUG("\nGot JSON:\n%s\n", msg.s_message);

	// Convert string to json object using JSON-C library functions
	if (GSI_JSON_SUCCESS != gsi_build_parse_string_to_json_object(msg.s_message, &p_json))
	{
		LOG_ERROR("convert string to json object failed");

		free(msg.s_message);
		msg.s_message = NULL;

		json_object_put(p_json);

		return GSI_JSON_ERROR;
	}

	// Free s_message, finish his job
	free(msg.s_message);
	msg.s_message = NULL;

	// Convert json object to json-msg object using JSON-C library functions
	if (GSI_JSON_SUCCESS != gsi_build_parse_json_object_to_json_msg(p_json, p_json_msg))
	{
		LOG_ERROR("convert json object to json-msg failed");

		json_object_put(p_json);

		return GSI_JSON_ERROR;
	}

	// Free the json object
	json_object_put(p_json);

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_get_next_msg
	 * Description:	Get the next message in f_msg_file
	 * Parameter:   [in] FILE* f_msg_file - handler to opened file
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to message structure
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR *OR* GSI_JSON_READ_ERROR
#############################################################################*/
enum gsi_is_json_rc gsi_is_get_next_msg(FILE* f_msg_file, struct gsi_json_msg* p_json_msg)
{
	char s_msg_buffer[GSI_IS_BUFFER_SIZE];

	// Check input validation
	if ((NULL == f_msg_file) || (NULL == p_json_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_JSON_INVALID_ERR;
	}

	// Reset buffer
	memset(s_msg_buffer, 0, sizeof(s_msg_buffer));

	// Get next message from Messages file
	if (NULL == fgets(s_msg_buffer, sizeof(s_msg_buffer), f_msg_file))
	{
		LOG_ERROR("couldn't read from messages file");
		return GSI_JSON_READ_ERROR;
	}

	// Build message according to the current line that read from file
	if (0 != gsi_build_parse_build_msg(s_msg_buffer, p_json_msg))
	{
		LOG_ERROR("couldn't build message");
		return GSI_JSON_ERROR;
	}

	return GSI_JSON_SUCCESS;
}

/***********************************/
/* Static functions implementation */
/***********************************/
/*###########################################################################
	 * Name:		gsi_build_parse_build_msg
	 * Description: Initialize fields in struct gsi_json_msg according to current line
	 * Parameter:   [in] char* s_line - current line read from messages file
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to json-msg object
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_INVALID_ERR *OR* GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_build_msg(char* s_line, struct gsi_json_msg* p_json_msg)
{
	// Check input validation
	if ((NULL == s_line) || (NULL == p_json_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_JSON_INVALID_ERR;
	}

	// Set message type
	p_json_msg->i_msg_type = gsi_build_parse_get_msg_type(&s_line);
	if (GSI_JSON_ERROR == p_json_msg->i_msg_type)
	{
		LOG_ERROR("get message type failed");
		return GSI_JSON_ERROR;
	}

	// Only if Regular message sent
	if (GSI_REGULAR_MSG == p_json_msg->i_msg_type)
	{
		// Set operation code
		p_json_msg->i_op_code = gsi_build_parse_get_msg_op_code(&s_line);
		if (GSI_JSON_ERROR == p_json_msg->i_op_code)
		{
			LOG_ERROR("get message op-code failed");
			return GSI_JSON_ERROR;
		}

		// Set operation code arguments
		if (GSI_JSON_SUCCESS != gsi_build_parse_set_op_code_args(&s_line, p_json_msg))
		{
			LOG_ERROR("set op-code-arguments failed");
			return GSI_JSON_ERROR;
		}
	}

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_build_parse_get_msg_type
	 * Description: Check the type of the message and moves the pointer forward
	 * Parameter:   [in] char** s_line - address of current line
	 * Return:		Success - Number that describe the type of the message
	 * 				Failure - GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_get_msg_type(char** s_line)
{
	int i_type = 0;

	// Check input validation
	if (NULL == s_line)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_ERROR;
	}

	// Look at the first character in the current line: 'H'/'M'/'#'
	switch(**s_line)
	{
	case GSI_IS_HEART_BEAT:
		i_type = GSI_HEARTBEAT_MSG;
		break;
	case GSI_IS_MESSAGE:
		i_type = GSI_REGULAR_MSG;
		break;
	case GSI_IS_COMMENT:
		i_type = GSI_COMMENT;
		break;
	default:
		LOG_ERROR("classified message failed");
		return GSI_JSON_ERROR;
	}

	// Move forward the line pointer
	++(*s_line);

	return i_type;
}

/*###########################################################################
	 * Name:		gsi_build_parse_get_msg_op_code
	 * Description: Check the type of the op-code and moves the pointer forward
	 * Parameter:   [in] char** s_line - address of current line
	 * Return:		Success - Number that describe the operation-code
	 * 				Failure - GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_get_msg_op_code(char** s_line)
{
	int i_op_code = 0;

	// Check input validation
	if (NULL == s_line)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_ERROR;
	}

	// Skip the ':' at the beginning of the s_line
	++(*s_line);

	// Decode operation-code from s_line
	if (0 == strncmp(*s_line, GSI_IS_READ_FILE_BY_ID, strlen(GSI_IS_READ_FILE_BY_ID)))
	{
		i_op_code = GSI_READ_FILE_BY_ID;
	}
	else if (0 == strncmp(*s_line, GSI_IS_READ_STR, strlen(GSI_IS_READ_STR)))
	{
		i_op_code = GSI_READ_STR;
	}
	else if (0 == strncmp(*s_line, GSI_IS_WRITE_STR, strlen(GSI_IS_WRITE_STR)))
	{
		i_op_code = GSI_WRITE_STR;
	}
	else if (0 == strncmp(*s_line, GSI_IS_READ_FILE, strlen(GSI_IS_READ_FILE)))
	{
		i_op_code = GSI_READ_FILE;
	}
	else if (0 == strncmp(*s_line, GSI_IS_WRITE_FILE, strlen(GSI_IS_WRITE_FILE)))
	{
		i_op_code = GSI_WRITE_FILE;
	}
	else if (0 == strncmp(*s_line, GSI_IS_PRINT_LOG, strlen(GSI_IS_PRINT_LOG)))
	{
		i_op_code = GSI_PRINT_LOG;
	}
	else
	{
		LOG_ERROR("invalid op code");
		return GSI_JSON_ERROR;
	}

	// Move forward the line pointer
	if (GSI_READ_FILE_BY_ID == i_op_code)
	{
		*s_line += strlen(GSI_IS_READ_FILE_BY_ID);
	}
	else
	{
		*s_line += strlen(GSI_IS_PRINT_LOG);
	}

	return i_op_code;
}

/*###########################################################################
	 * Name:		gsi_build_parse_get_msg_index
	 * Description: Extract the index number from the current line string and moves the pointer forward
	 * Parameter:   [in] char** s_line - address of current line
	 * Return:		Success - Number that describe the index
	 * 				Failure - GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_get_msg_index(char** s_line)
{
	int i_index = 0;

	// Check input validation
	if (NULL == s_line)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_ERROR;
	}

	// Skip the ':' at the beginning of the s_line
	++(*s_line);

	// Extract the index from the s_line, stops at ' '
	i_index = strtol(*s_line, s_line, 10);

	return i_index;
}

/*###########################################################################
	 * Name:		gsi_build_parse_get_msg_len
	 * Description: Return the message length
	 * Parameter:   [in] char* s_msg - message
	 * Return:		Success - message's length
	 * 				Failure - GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_get_msg_len(char* s_msg)
{
	// Check input validation
	if (NULL == s_msg)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_ERROR;
	}

	return strlen(s_msg);
}

/*###########################################################################
	 * Name:		gsi_build_parse_get_msg_content
	 * Description: Return pointer to actual data
	 * Parameter:   [in] char** s_line - address of current line
	 * Return:		Success - pointer to data
	 * 				Failure - NULL
#############################################################################*/
static char* gsi_build_parse_get_msg_content(char** s_line)
{
	// Check input validation
	if (NULL == s_line)
	{
		LOG_ERROR("invalid argument!");
		return NULL;
	}

	// Skip the white spaces at the beginning of the s_line
	while (('\0' != **s_line) && (isspace(**s_line)))
	{
		++(*s_line);
	}

	return *s_line;
}

/*###########################################################################
	 * Name:		gsi_build_parse_set_op_code_args
	 * Description:	Fill the relevant fields in json-msg object according to the OP-CODE
	 * 				Memory allocation must be free by the function gsi_build_parse_reset_object
	 * Parameter:   [in] char** s_line - address of current line
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to json-msg object
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_ERROR *OR* GSI_JSON_INVALID_ERR
#############################################################################*/
static int gsi_build_parse_set_op_code_args(char** s_line, struct gsi_json_msg* p_json_msg)
{
	// Check input validation
	if ((NULL == s_line) || (NULL == p_json_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_JSON_INVALID_ERR;
	}

	switch(p_json_msg->i_op_code)
	{
		case GSI_READ_STR:
			// Get index
			p_json_msg->i_index = gsi_build_parse_get_msg_index(s_line);
			break;

		case GSI_WRITE_STR:
			// Get index + length
			p_json_msg->i_index    = gsi_build_parse_get_msg_index(s_line);
			p_json_msg->i_data_len = gsi_build_parse_get_msg_len(*s_line);

			// Duplicate message content
			p_json_msg->s_data  = gsi_build_parse_strdup(gsi_build_parse_get_msg_content(s_line));
			if (NULL == p_json_msg->s_data)
			{
				return GSI_JSON_ERROR;
			}

			break;

		case GSI_READ_FILE:
		case GSI_PRINT_LOG:
			// Get file length
			p_json_msg->i_file_len = gsi_build_parse_get_msg_len(*s_line);

			// Duplicate file name
			p_json_msg->s_file_name = gsi_build_parse_strdup(gsi_build_parse_get_file_name(s_line));
			if (NULL == p_json_msg->s_file_name)
			{
				return GSI_JSON_ERROR;
			}

			break;

		case GSI_WRITE_FILE:
		case GSI_READ_FILE_BY_ID:
			// Get file length
			p_json_msg->i_file_len = gsi_build_parse_get_msg_len(*s_line);

			// Duplicate file name
			p_json_msg->s_file_name = gsi_build_parse_strdup(gsi_build_parse_get_file_name(s_line));
			if (NULL == p_json_msg->s_file_name)
			{
				return GSI_JSON_ERROR;
			}

			// Data length
			p_json_msg->i_data_len = gsi_build_parse_get_msg_len(*s_line);

			// Allocate memory for s_data
			p_json_msg->s_data = gsi_build_parse_strdup(gsi_build_parse_get_msg_content(s_line));
			if (NULL == p_json_msg->s_data)
			{
				free(p_json_msg->s_file_name);
				p_json_msg->s_file_name = NULL;

				return GSI_JSON_ERROR;
			}

			break;

		default:
			LOG_ERROR("invalid operation code");
			return GSI_JSON_ERROR;
	}

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_build_parse_json_obj_to_string
	 * Description: Convert gsi_json_message to string format.
	 * 				For each field in structure - generate tuple <NAME>:<VALUE>
	 * 				according to JSON format
	 * Parameter:   [in] struct gsi_json_msg* p_json_msg - pointer to structure to convert
	 * Parameter:   [in-out] struct json_object *p_json - pointer to json object to use JSON-C library
	 * Return:		Success - char* - string that contatins all the structure in json format
	 * 				Failure - NULL
#############################################################################*/
static char* gsi_build_parse_json_obj_to_string(struct json_object *p_json, struct gsi_json_msg* p_json_msg)
{
	int i_ret = 0;

	// Check input validation
	if (NULL == p_json_msg)
	{
		LOG_ERROR("invalid argument!");
		return NULL;
	}

	// For each filed in structure - add object: <NAME>:<VALUE>
	i_ret += json_object_object_add(p_json, "Message Type",json_object_new_int(p_json_msg->i_msg_type));
	i_ret += json_object_object_add(p_json, "Op-Code", 	   json_object_new_int(p_json_msg->i_op_code));
	i_ret += json_object_object_add(p_json, "Op-Str", 	   json_object_new_string(gsi_build_parse_op_code_to_string(p_json_msg->i_op_code)));
	i_ret += json_object_object_add(p_json, "Port", 	   json_object_new_int(p_json_msg->ui_port));
	i_ret += json_object_object_add(p_json, "Index",   	   json_object_new_int(p_json_msg->i_index));
	i_ret += json_object_object_add(p_json, "Data Length", json_object_new_int(p_json_msg->i_data_len));
	i_ret += json_object_object_add(p_json, "File Length", json_object_new_int(p_json_msg->i_file_len));

	// Check if has file name or NULL
	if (NULL != p_json_msg->s_file_name)
	{
		i_ret += json_object_object_add(p_json, "File Name", json_object_new_string(p_json_msg->s_file_name));
	}
	else
	{
		i_ret += json_object_object_add(p_json, "File Name", NULL);
	}

	// Check if has data or NULL
	if (NULL != p_json_msg->s_data)
	{
		i_ret += json_object_object_add(p_json, "Data", json_object_new_string(p_json_msg->s_data));
	}
	else
	{
		i_ret += json_object_object_add(p_json, "Data", NULL);
	}

	// Check status of adding all objects
	if (0 != i_ret)
	{
		LOG_ERROR("json object add failed");
		return NULL;
	}

	// Converting the json object to string format, using JSON-C library function
	return (char *)json_object_to_json_string_ext(p_json, JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED);
}

/*###########################################################################
	 * Name:		gsi_build_parse_string_to_json_object
	 * Description: Convert string to json object using JSON-C library functions
	 * Parameter:   [in] char *s_data - string to convert
	 * Parameter:   [out] struct json_object** p_json - pointer to fill
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_INVALID_ERR *OR* GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_string_to_json_object(char *s_data, struct json_object** p_json)
{
	// Check input validation
	if (NULL == s_data)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_INVALID_ERR;
	}

	// parse the string to json object
	*p_json = json_tokener_parse(s_data);
	if (NULL == *p_json)
	{
		LOG_ERROR("string to json failed");
		return GSI_JSON_ERROR;
	}

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_build_parse_json_object_to_json_msg
	 * Description: Convert json object to structure json-msg object
	 * Parameter:   [in] struct json_object *p_json - pointer to json object
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to fill
	 * Return:		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_INVALID_ERR *OR* GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_json_object_to_json_msg(struct json_object *p_json, struct gsi_json_msg* p_json_msg)
{
	// Check input validation
	if ((NULL == p_json) || (NULL == p_json_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_JSON_INVALID_ERR;
	}

	// Initialize p_json_msg fields. using JSON-C library functions
	p_json_msg->i_msg_type = json_object_get_int(json_object_object_get(p_json, "Message Type"));
	p_json_msg->i_op_code  = json_object_get_int(json_object_object_get(p_json, "Op-Code"));
	p_json_msg->ui_port    = json_object_get_int(json_object_object_get(p_json, "Port"));

	// Initialize relevant fields according to operaion code
	if (GSI_JSON_SUCCESS != gsi_build_parse_handle_op_code(p_json, p_json_msg))
	{
		LOG_ERROR("couldn't handle op code");
		return GSI_JSON_ERROR;
	}

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_build_parse_handle_op_code
	 * Description: Initialize fields in json-msg object according to operation code
	 * Parameter:   [in] struct json_object *p_json - pointer to json object
	 * Parameter:   [out] struct gsi_json_msg* p_json_msg - pointer to fill
	 * Return: 		Success - GSI_JSON_SUCCESS
	 * 				Failure - GSI_JSON_INVALID_ERR *OR* GSI_JSON_ERROR
#############################################################################*/
static int gsi_build_parse_handle_op_code(struct json_object *p_json, struct gsi_json_msg* p_json_msg)
{
	// Check input validation
	if (NULL == p_json_msg)
	{
		LOG_ERROR("invalid argument!");
		return GSI_JSON_INVALID_ERR;
	}

	switch (p_json_msg->i_op_code)
	{
		case GSI_READ_STR:
			// Get index
			p_json_msg->i_index = json_object_get_int(json_object_object_get(p_json, "Index"));
			break;

		case GSI_WRITE_STR:
			// Get index and data length
			p_json_msg->i_index    = json_object_get_int(json_object_object_get(p_json, "Index"));
			p_json_msg->i_data_len = json_object_get_int(json_object_object_get(p_json, "Data Length"));

			// Duplicate the data
			p_json_msg->s_data = gsi_build_parse_strdup(json_object_get_string(json_object_object_get(p_json, "Data")));
			if (NULL == p_json_msg->s_data)
			{
				return GSI_JSON_ERROR;
			}

			break;

		case GSI_READ_FILE:
	    case GSI_PRINT_LOG:
			// Get file name length
			p_json_msg->i_file_len 	= json_object_get_int(json_object_object_get(p_json, "File Length"));

			// Duplicate the file name
			p_json_msg->s_file_name = gsi_build_parse_strdup(json_object_get_string(json_object_object_get(p_json, "File Name")));
			if (NULL == p_json_msg->s_file_name)
			{
				return GSI_JSON_ERROR;
			}

			break;

		case GSI_WRITE_FILE:
		case GSI_READ_FILE_BY_ID:

			// Get source file length
			p_json_msg->i_file_len 	= json_object_get_int(json_object_object_get(p_json, "File Length"));

			// Duplicate the source file name
			p_json_msg->s_file_name = gsi_build_parse_strdup(json_object_get_string(json_object_object_get(p_json, "File Name")));
			if (NULL == p_json_msg->s_file_name)
			{
				LOG_ERROR("memory allocation for s_file_name failed");
				return GSI_JSON_ERROR;
			}

			// Get destination file length
			p_json_msg->i_data_len = json_object_get_int(json_object_object_get(p_json, "Data Length"));

			// Duplicate the destination file name
			p_json_msg->s_data = gsi_build_parse_strdup(json_object_get_string(json_object_object_get(p_json, "Data")));
			if (NULL == p_json_msg->s_data)
			{
				free(p_json_msg->s_file_name);
				p_json_msg->s_file_name = NULL;

				return GSI_JSON_ERROR;
			}

			break;

		default:
			LOG_ERROR("invalid operation code");
			return GSI_JSON_ERROR;
	}

	return GSI_JSON_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_build_parse_get_src_file
	 * Description: Extract the source file name from the current line and moves the pointer forward
	 * 				The delimiter to stop extraction is white space: ' '
	 * Parameter:   [in] char** s_line - address of current line
	 * Return:		Success - char* - source file name
	 * 				Failure - NULL
#############################################################################*/
static char* gsi_build_parse_get_file_name(char** s_line)
{
	char* s_src_file = NULL;
	char* s_runner = NULL;

	// Check input validation
	if (NULL == s_line)
	{
		LOG_ERROR("invalid argument!");
		return NULL;
	}

	//Skip all white space before the file name
	while (('\0' != **s_line) && (isspace(**s_line)))
	{
		++(*s_line);
	}

	// Start point to hold the name of the file
	s_src_file = *s_line;

	// Runner pointer to run until meet white space
	s_runner = s_src_file;

	// Main run loop
	while (!isspace(*s_runner) && ('\n' != *s_runner))
	{
		++s_runner;
	}

	// Split the string in the current position
	*s_runner = '\0';

	// Move the original pointer by length of the file name
	*s_line += strlen(s_src_file) + 1;

	return s_src_file;
}

/*###########################################################################
	 * Name:		gsi_build_parse_strdup
	 * Description:	Duplicate a string. using dynamic memory allocation. must free by the user
	 * Parameter:   [in] char* s_src - source string
	 * Return:		Success - duplicate string
	 * 				Failure - NULL
#############################################################################*/
static char* gsi_build_parse_strdup(const char* s_src)
{
	char* s_dest = NULL;

	// Check input validation
	if (NULL == s_src)
	{
		LOG_ERROR("invalid argument!");
		return NULL;
	}

	// Allocate memory for the destination
	s_dest = (char *)malloc(strlen(s_src) + 1);
	if (NULL == s_dest)
	{
		LOG_ERROR("memory allocation in for dupliaction failed");
		return NULL;
	}

	// Copy content
	strcpy(s_dest, s_src);

	return s_dest;
}

/*###########################################################################
	 * Name:		gsi_build_parse_op_code_to_string
	 * Description: Convert op-code to string
	 * Parameter:   [in] int i_op_code - op code number
	 * Return:		Success - op code as string literal
	 * 				Failure - NULL
#############################################################################*/
static char* gsi_build_parse_op_code_to_string(int i_op_code)
{
	switch(i_op_code)
	{
		case GSI_READ_STR:
			return "READ_STR";

		case GSI_WRITE_STR:
			return "WRITE_STR";

		case GSI_READ_FILE:
			return "READ_FILE";

		case GSI_PRINT_LOG:
			return "PRINT_LOG";

		case GSI_WRITE_FILE:
			return "WRITE_FILE";

		case GSI_READ_FILE_BY_ID:
			return "READ_STR_BY_ID";

		default:
			return NULL;
	}
}
