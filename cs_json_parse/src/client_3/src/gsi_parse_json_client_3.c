/**************************************************************************
* Name : gsi_parse_json_client_3.c
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Client demo program using Networking.
* 				Read configuration from file supplied on command line
* 				Usage : ./<a.out> --cfg=<config_file>
* 				On every 5 Messages we have to send Heart beat alert to server
* 				Otherwise the connection will be closed by the server.
* 				writing log messages into log file.
*****************************************************************************/

/* Includes */
#include <stdlib.h>
#include "gsi_parse_json_config.h"
#include "gsi_is_log_api.h"
#include "gsi_is_network_tcp.h"
#include "gsi_build_parse_data.h"

/* Defines and Macros */
#define 	GSI_IS_RECONNECT_TRY    3	/* Number of retry connection in case of failure */
#define 	GSI_IS_ADDRESS_LENGTH   sizeof("255.255.255.255:<port>")  /* IP address length as string */
#define 	GSI_IS_FAIL				-1

/* Global variables */

// instance of client structure contains all its config parameters
extern struct gsi_prase_json_config_client_params g_config_client_params;

/********************************/
/* Static functions declaration */
/********************************/
static enum gsi_is_network_return_code gsi_client_connect_client_to_server(struct gsi_net_tcp* p_client, unsigned int ui_port);

/*###########################################################################
 	 * Name:        main.
 	 * Description: Entry point of the program, this is client demo program
	 * 				messages file must contain messages in the form of:
 	 * 				M: <message> - Regular message (Note for the white space after ':')
 	 * 				H:			 - Heart Beat message
 	 * 				# <comment>  - Ignore this line
 	 * 				for more details see "gsi_build_parse_data.h" file
 	 * Parameter:   char** argv - [1] config file name
 	 * Return: 	    Success - 0
 	 * 				Failure - GSI_IS_FAIL
#############################################################################*/
int main(int argc, char **argv)
{
	struct gsi_net_tcp client;
	char s_file_name[GSI_IS_LOG_MAX_FILE_NAME];
	FILE* f_messages = NULL;
	FILE* f_log = NULL;

	if (2 != argc)
	{
		printf("usage error: <a.out> --config=<config_file>\n");
		return GSI_IS_FAIL;
	}

	// Create log file
	memset(s_file_name, 0, sizeof(s_file_name));
	sprintf(s_file_name, "gsi-log-client-3");

	f_log = gsi_is_create_log_file(s_file_name, NULL);
	if (NULL == f_log)
	{
		return GSI_IS_FAIL;
	}

	// Set configuration parameters for client program read it from config file given by argv
	if (GSI_PARSE_JSON_CONFIG_SUCCESS != gsi_parse_json_config_get_config(argc, argv))
	{
		printf("cannot load configuration parameters\n");
		return GSI_IS_FAIL;
	}

	// Connect client to server
	if (GSI_NET_RC_SUCCESS != gsi_client_connect_client_to_server(&client, g_config_client_params.ui_port))
	{
		LOG_ERROR("client connect failed");
		return GSI_IS_FAIL;
	}

	// Open the Messages file to read messages from it.
	f_messages = gsi_is_open_msg_file(g_config_client_params.s_messages_file);
	if (NULL == f_messages)
	{
		LOG_ERROR("couldn't open messages file");
		exit(0);
	}

	// Send messages
	if (GSI_JSON_SUCCESS != gsi_is_send_all_json_msg(f_messages, &client))
	{
		LOG_ERROR("send messages to server failed");
	}

	// Close messages file
	if (GSI_JSON_SUCCESS != gsi_is_close_msg_file(f_messages))
	{
		LOG_ERROR("couldn't close messages file")
	}

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
	 * Name:		connect_client_to_server
	 * Description: Sets client parameters to connect server.
	 * 				If the connection failed - retry GSI_IS_RECONNECT_TRY times.
	 * Parameter:   [in] struct gsi_net_tcp* p_client - pointer to client
	 * Parameter:   [in] unsigned int ui_port - port for connection
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR *OR* GSI_NET_RC_CONNECTERR
#############################################################################*/
static enum gsi_is_network_return_code gsi_client_connect_client_to_server(struct gsi_net_tcp* p_client, unsigned int ui_port)
{
	int i_rc = 0;
	int i_retry = 0;
	char s_buffer[GSI_IS_ADDRESS_LENGTH];

	// Check input validation
	if (NULL == p_client)
	{
		LOG_ERROR("invalid arguments");
		return GSI_NET_RC_ERROR;
	}

	// Reset server fields
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_reset(p_client))
	{
		LOG_ERROR("reset client fields failed");
		return GSI_NET_RC_ERROR;
	}

	do
	{
		// Reset buffer
		memset(s_buffer, 0, sizeof(s_buffer));

		// Create string <IP>:<Port> connect to local host
		sprintf(s_buffer, "127.0.0.1:%d", ui_port);

		// Init client and connect to server
		i_rc = gsi_is_network_tcp_client_init(p_client, s_buffer);
		if (GSI_NET_RC_SUCCESS == i_rc)
		{
			break;
		}

		LOG_ERROR("client init failed");
		LOG_ERROR("Reconnect...");

		++i_retry;

		sleep(1);
	}
	while (i_retry < GSI_IS_RECONNECT_TRY);

	// No respond - exit program
	if ((GSI_NET_RC_CONNECTERR == i_rc) && (i_retry == GSI_IS_RECONNECT_TRY))
	{
		LOG_ERROR("server is not responding...client leave");
		return GSI_NET_RC_CONNECTERR;
	}

	// General error - exit program
	if (GSI_NET_RC_ERROR == i_rc)
	{
		LOG_ERROR("error has been occurred");
		return GSI_NET_RC_ERROR;
	}

	LOG_INFO("client connect successfully to port %d", p_client->ui_port);
	return GSI_NET_RC_SUCCESS;
}
