/**************************************************************************
* Name : gsi_is_network_tcp.c
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Implementation of "gsi_is_network_tcp.h"
*****************************************************************************/

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include "gsi_is_network_tcp.h"
#include "gsi_is_log_api.h"

/* Defines and Macros */
#define 	GSI_IS_TRUE				  	  1
#define 	GSI_IS_FALSE				  0
#define 	GSI_IS_MAX_LISTEN			  10 	/* max of queue length in listen() */
#define 	GSI_IS_POLL_SOCKET_LISTEN 	  0		/* index in fd array */
#define 	GSI_IS_POLL_SOCKET_CONNECT    1		/* index in fd array */
#define 	GSI_IS_POLL_DELAY_MSECS	      10000 /* timeout for poll() */
#define 	GSI_IS_MAX_MSG_COUNT		  5		/* max messages without heart beat */

/********************************/
/* Static functions declaration */
/********************************/
static char* set_address_parameters(struct gsi_net_tcp *p_this, char* s_tcp_addr);
static enum gsi_is_network_return_code read_check_heartbeat(struct gsi_net_tcp *p_this);

/********************/
/* Common Functions */
/********************/
/*###########################################################################
	 * Name:		gsi_is_network_tcp_reset
	 * Description: reset all the fields of struct gsi_net_tcp
	 * Parameter:   [in] struct gsi_net_tcp* p_this - pointer to structure TCP
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_reset(struct gsi_net_tcp* p_this)
{
	// Check input validation
	if (NULL == p_this)
	{
		LOG_ERROR("invalid argument!");
		return GSI_NET_RC_ERROR;
	}

	// Set all fields to 0/NULL according to their type
	memset(p_this, 0, sizeof(struct gsi_net_tcp));

	return GSI_NET_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_network_tcp_set_sockaddr
	 * Description: Set up connection parameters:
	 * 				- IP Address (from hostname)
	 * 				- Port
	 * Parameter:   [out] struct sockaddr_in *p_addr - INET Parameters (IP Address & Port) struct
	 * Parameter:   [in]  char *s_hostname - IP Address
	 * Parameter:   [in]  int i_port - port number
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_set_sockaddr(struct sockaddr_in *p_addr,
														  	    char *s_hostname,
																int i_port)
{
	// Check input validation
	if (NULL == p_addr)
	{
		LOG_ERROR("invalid argument!");
		return GSI_NET_RC_ERROR;
	}

	// Setup socket parameters to INET (IPv4)
	p_addr->sin_family = AF_INET;

	// Translate Hostname to IP Address
	if (0 >= inet_pton(AF_INET, s_hostname, &p_addr->sin_addr))
	{
		LOG_ERROR("translation ip failed");
		return GSI_NET_RC_ERROR;
	}

	// Translate Port to network byte order
	p_addr->sin_port = htons(i_port);

	LOG_INFO("set socket parameters success");
	return GSI_NET_RC_SUCCESS;
}


/********************/
/* Client Functions */
/********************/
/*###########################################################################
	 * Name:		gsi_is_network_tcp_connect
	 * Description: Create a Socket and connect to server.
	 * Parameter:   [in]  struct sockaddr_in *p_serv_addr - address of server (IP Address + Port).
	 * Parameter:   [out] int *p_socket_fd - pointer to socket file descriptor.
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_connect(struct sockaddr_in *p_serv_addr,
													 	   int *p_socket_fd)
{
	// Check input validation
	if ((NULL == p_serv_addr) || (NULL == p_socket_fd))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_NET_RC_ERROR;
	}

	// Open a Socket and update p_socket_fd
	if (0 > (*p_socket_fd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		LOG_ERROR("socket fail");
		return GSI_NET_RC_ERROR;
	}

	// Connect to Server
	if (0 > connect(*p_socket_fd, (struct sockaddr *)p_serv_addr, sizeof(struct sockaddr_in)))
	{
		LOG_ERROR("connect fail");
		return GSI_NET_RC_CONNECTERR;
	}

	LOG_INFO("connect success!");
	return GSI_NET_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_network_tcp_send
	 * Description: Send a single message over an IPv4 TCP socket
	 * 				Generates a new connection for each message.
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP
	 * Parameter:   [in] char *s_msg - message to send
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR *OR* GSI_NET_RC_CONNECTERR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_send(struct gsi_net_tcp *p_this,
														char *s_msg)
{
	int i_len = 0;
	int i_count = 0;
	struct gsi_cs_tcp_message *p_msg = (struct gsi_cs_tcp_message *)s_msg;

	// Check input validation
	if ((NULL == p_this) || (NULL == p_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_NET_RC_ERROR;
	}

	// Save message size for later comparison
	i_len = sizeof(*p_msg) - sizeof(char*);

	/* 	Attempt to WRITE:
	 * 		if i_count == original requested i_len -- Success
	 * 		if i_count < 0 -- Error, Attempt to Re-Connect
	 * 		if 0 < i_count < i_len - Partial. Consider an Error
	 */
	while ((i_count = write(p_this->i_connection_fd, p_msg, i_len)) < 0)
	{
		LOG_INFO("try to reconnect...");

		if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_connect(&p_this->serv_addr, &p_this->i_connection_fd))
		{
			LOG_ERROR("connection failed!");
			return GSI_NET_RC_CONNECTERR;
		}
	}

	// Send message content
	if ((GSI_REGULAR_MSG == p_msg->e_type_msg) &&
		(0 > write(p_this->i_connection_fd, p_msg->s_message, p_msg->ui_len)))
	{
		LOG_ERROR("write message failed");
		return GSI_NET_RC_ERROR;
	}

	// Case of: 0 < i_count < i_len
	if (i_count < i_len)
	{
		LOG_ERROR("partial write");
		return GSI_NET_RC_ERROR;
	}

	LOG_INFO("message sent successfully");
	return GSI_NET_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_network_tcp_client_init
	 * Description:	Initializes an Instance of struct TCP Client,
	 * 				Connects over TCP
	 * Parameter:   [out] struct gsi_net_tcp *p_this - pointer to structure TCP Client
	 * Parameter:   [in] char* s_tcp_addr - Address to connect to <IPAddress>:<Port>
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR *OR* GSI_NET_RC_CONNECTERR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_client_init(struct gsi_net_tcp *p_this, char* s_tcp_addr)
{
	// Check input validation
	if ((NULL == p_this) || (NULL == s_tcp_addr))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_NET_RC_ERROR;
	}

	// Initialize address parameters
	char* s_port = set_address_parameters(p_this, s_tcp_addr);
	if (NULL == s_port)
	{
		LOG_ERROR("invalid form of address");
		return GSI_NET_RC_ERROR;
	}

	// Set port member
	p_this->ui_port = atoi(s_port);

	// Set socket parameters
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_set_sockaddr(&p_this->serv_addr,
															   p_this->s_tcp_addr,
															   p_this->ui_port))
	{
		LOG_ERROR("set socket parameters failed");
		return GSI_NET_RC_ERROR;
	}

	// Connect to Server
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_connect(&p_this->serv_addr, &p_this->i_connection_fd))
	{
		LOG_ERROR("connection failed!");
		return GSI_NET_RC_CONNECTERR;
	}

	LOG_INFO("client init successfully");
	return GSI_NET_RC_SUCCESS;
}

/********************/
/* Server Functions */
/********************/
/*###########################################################################
	 * Name:		gsi_is_network_tcp_server_init
	 * Description:	Initializes an Instance of struct TCP Server
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Parameter:   [in] unsigned int ui_port - port to connect to
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_server_init(struct gsi_net_tcp *p_this, unsigned int ui_port)
{
	// Check input validation
	if (NULL == p_this)
	{
		LOG_ERROR("invalid arguments!");
		return GSI_NET_RC_ERROR;
	}

	// Reset server parameters
	if(GSI_NET_RC_SUCCESS != gsi_is_network_tcp_reset(p_this))
	{
		LOG_ERROR("reset server parameters failed");
		return GSI_NET_RC_ERROR;
	}

	// Set local ip
	p_this->s_tcp_addr = "127.0.0.1";
	p_this->ui_port = ui_port;

	// Set socket parameters
	if (GSI_NET_RC_SUCCESS != gsi_is_network_tcp_set_sockaddr(&p_this->serv_addr,
															   p_this->s_tcp_addr,
															   p_this->ui_port))
	{
		LOG_ERROR("set socket parameters failed");
		return GSI_NET_RC_ERROR;
	}

	// Clear the field of last message
	p_this->s_last_msg = NULL;

	// Create Listen Socket, used to receive connections
    p_this->i_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > p_this->i_listen_fd)
    {
    	LOG_ERROR("socket failed");
    	return GSI_NET_RC_ERROR;
    }

    // Reuse local address
    int i_reuse_addr = GSI_IS_TRUE;
    if(0 < setsockopt(p_this->i_listen_fd, SOL_SOCKET, SO_REUSEADDR, &i_reuse_addr, sizeof(int)))
	{
		LOG_ERROR("cannot set socket options");
		return GSI_NET_RC_ERROR;
	}

    // Bind the socket fd to specific address
    if (0 > bind(p_this->i_listen_fd, (struct sockaddr*)(&p_this->serv_addr), sizeof(struct sockaddr)))
    {
    	LOG_ERROR("bind failed");
    	return GSI_NET_RC_ERROR;
    }

    // Set socket to listen for connection requests
    if (0 > listen(p_this->i_listen_fd, GSI_IS_MAX_LISTEN))
    {
    	LOG_ERROR("listen failed");
    	return GSI_NET_RC_ERROR;
    }

    LOG_INFO("server is listening on port %d", p_this->ui_port);
	return GSI_NET_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_network_tcp_server_poll
	 * Description:	Polls if TCP Server has available pending
	 * 			    connections or data on an established connection,
	 * 			    If pending connections are available, they 1st connection is
	 * 			    accepted.
	 * 			    If data is available on an established connection,
	 * 			    returns GSI_NET_RC_HASDATA.
	 *				POLL allows the TCP server to work in a non-blocking mode
	 *				which allows the server to perform other tasks while
	 *				waiting for either a connection or DATA.
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Return:		Success - GSI_NET_RC_HASDATA *OR* GSI_NET_RC_SUCCESS(non empty buffer)
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_server_poll(struct gsi_net_tcp *p_this)
{
	// Check input validation
	if (NULL == p_this)
	{
		LOG_ERROR("invalid argument!");
		return GSI_NET_RC_ERROR;
	}

	// Setup POLL for Listen FD (Server connection requests)
	p_this->pfds[GSI_IS_POLL_SOCKET_LISTEN].fd 	= p_this->i_listen_fd;
	p_this->pfds[GSI_IS_POLL_SOCKET_LISTEN].events = POLLIN;
	if (0 == p_this->i_listen_fd)
	{
		LOG_ERROR("server not listen");
		return GSI_NET_RC_ERROR;
	}

	// If Already connected, Setup POLL for Connection FD as well
	if (0 < p_this->i_connection_fd)
	{
		p_this->pfds[GSI_IS_POLL_SOCKET_CONNECT].fd	 = p_this->i_connection_fd;
		p_this->pfds[GSI_IS_POLL_SOCKET_CONNECT].events = POLLIN;
	}

	// Start poll to alert ready fds
	int i_rc = poll(p_this->pfds, GSI_IS_MAX_CONN, GSI_IS_POLL_DELAY_MSECS);

	switch (i_rc)
	{
		case POLLERR:
			LOG_ERROR("poll error!");
			return GSI_NET_RC_ERROR;
		default:
			if (p_this->pfds[GSI_IS_POLL_SOCKET_LISTEN].revents & POLLIN)
			{
				// Accept New Connection
				p_this->i_connection_fd = accept(p_this->i_listen_fd, (struct sockaddr*)NULL, NULL);
				if (0 > p_this->i_connection_fd)
				{
					LOG_ERROR("accept failed!");
					return GSI_NET_RC_ERROR;
				}
				LOG_INFO("new connection accepted on port %d", p_this->ui_port);
			}

			if (p_this->i_connection_fd && (p_this->pfds[GSI_IS_POLL_SOCKET_CONNECT].revents & POLLIN))
			{
				// Get Input from Channel
				return read_check_heartbeat(p_this);
			}

			break;
	}

	LOG_INFO("server poll successfully");
	return GSI_NET_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_network_tcp_server_read
	 * Description: Read one message from the TCP Server (Conn FD).
	 * 				Note! this function will allocate memory for the s_message buffer
	 * 				The user is responsible to free it after use.
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Parameter:   [out] char* s_msg - buffer to fill with the read message.
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure	- GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_server_read(struct gsi_net_tcp *p_this,
														 	   char* s_msg)
{
	int i_count = 0;
	struct gsi_cs_tcp_message *p_msg = (struct gsi_cs_tcp_message *)s_msg;

	// Check input validation
	if ((NULL == p_this) || (NULL == p_msg))
	{
		LOG_ERROR("invalid arguments!");
		return GSI_NET_RC_ERROR;
	}

	// Reset p_msg buffer
	memset(p_msg, 0, sizeof(struct gsi_cs_tcp_message));

	// Check if buffer is empty
	if (NULL == p_this->s_last_msg)
	{
		if (GSI_NET_RC_ERROR == read_check_heartbeat(p_this))
		{
			LOG_ERROR("heartbeat failed");
			return GSI_NET_RC_ERROR;
		}
	}

	// Read the data from the buffer of last message
	i_count = strlen(p_this->s_last_msg) + 1;

	// Allocate memory for s_message buffer
	p_msg->s_message = (char *)malloc(i_count);
	if (NULL == p_msg->s_message)
	{
		LOG_ERROR("memory allocation for s_message failed");
		return GSI_NET_RC_ERROR;
	}

	// Copy the message content
	memcpy(p_msg->s_message, p_this->s_last_msg, i_count);

	// Set message length
	p_msg->ui_len = i_count;

	// Set message type
	p_msg->e_type_msg = GSI_REGULAR_MSG;

	// Copy the content of ui_port
	p_msg->ui_port = p_this->ui_port;

	// Reset the last message buffer for the next message
	free(p_this->s_last_msg);
	p_this->s_last_msg = NULL;

	LOG_INFO("server read new message");
	return GSI_NET_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_network_tcp_server_cleanup
	 * Description: Cleans up the TCP Server.
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_server_cleanup(struct gsi_net_tcp *p_this)
{
	// Check input validation
	if (NULL == p_this)
	{
		LOG_ERROR("invalid argument!");
		return GSI_NET_RC_ERROR;
	}

	// Close connection socket (if open)
	if (p_this->i_connection_fd)
	{
		if (0 > close(p_this->i_connection_fd))
		{
			LOG_ERROR("close connection fd failed");
			return GSI_NET_RC_ERROR;
		}

		// Reset poll_fds array of connection type
		p_this->pfds[GSI_IS_POLL_SOCKET_CONNECT].fd	 = 0;
		p_this->pfds[GSI_IS_POLL_SOCKET_CONNECT].events = 0;
	}

	// Close listen socket
	if (0 > close(p_this->i_listen_fd))
	{
		LOG_ERROR("close listen fd failed");
		return GSI_NET_RC_ERROR;
	}

	// Reset poll_fds array of listen type
	p_this->pfds[GSI_IS_POLL_SOCKET_LISTEN].fd		= 0;
	p_this->pfds[GSI_IS_POLL_SOCKET_LISTEN].events	= 0;

	LOG_INFO("cleanup successfully\n");
	return GSI_NET_RC_SUCCESS;
}

/***********************************/
/* Static functions implementation */
/***********************************/
/*###########################################################################
	 * Name: 		set_address_parameters
	 * Description: Get address as string in format: <IP>:<Port> and split it
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Parameter:   [in] char* s_tcp_addr - address to connect to
	 * Return:		Success - char* - pointer to port as string
	 * 				Failure - NULL
#############################################################################*/
static char* set_address_parameters(struct gsi_net_tcp *p_this, char* s_tcp_addr)
{
	// Check input validation
	if ((NULL == p_this) || (NULL == s_tcp_addr))
	{
		LOG_ERROR("invalid arguments!");
		return NULL;
	}

	// Update the field in the pointer
	p_this->s_tcp_addr = s_tcp_addr;

	// Find the ':' character inside the string
	char *s_port = strchr(p_this->s_tcp_addr, ':');
	if (NULL == s_port)
	{
		LOG_ERROR("The s_tcp_addr string doesn't contain ':'");
		return NULL;
	}

	// Split Connect String <host_ip>:<port number> to Host IP Addr and Port Number
	*s_port = '\0';
	++s_port;

	// Clear the Serv Addr (struct sockaddr_in)
	memset(&p_this->serv_addr, 0, sizeof(p_this->serv_addr));

	LOG_INFO("set address parameters successfully");
	return s_port;
}

/*###########################################################################
	 * Name:		read_check_heartbeat
	 * Description: Read from connection fd when it is ready,
	 * 				Check if there is data or just heartbeat message
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Return:		Success - GSI_NET_RC_HASDATA *OR* GSI_NET_RC_SUCCESS(non empty buffer)
	 * 				Failure - GSI_NET_RC_ERROR *OR* GSI_NET_RC_CONNECTERR
#############################################################################*/
static enum gsi_is_network_return_code read_check_heartbeat(struct gsi_net_tcp *p_this)
{
	struct gsi_cs_tcp_message msg;

	// Check input validation
	if (NULL == p_this)
	{
		LOG_ERROR("invalid argument!");
		return GSI_NET_RC_ERROR;
	}

	// Reset message
	memset(&msg, 0, sizeof(msg));

	// Check if buffer is empty
	if (NULL == p_this->s_last_msg)
	{
		// Read part of the structure to know what is the message length
		int i_count = read(p_this->i_connection_fd, &msg, sizeof(msg) - sizeof(char *));
		if (0 > i_count)
		{
			LOG_ERROR("read failed");
			return GSI_NET_RC_ERROR;
		}
		else if (0 < i_count)
		{
			LOG_DEBUG("first read %d bytes from fd: %d", i_count, p_this->i_connection_fd);
			LOG_DEBUG("msg type: %d", msg.e_type_msg);
			LOG_DEBUG("msg length: %d", msg.ui_len);

			switch(msg.e_type_msg)
			{
				case GSI_REGULAR_MSG:
				{
					if (GSI_IS_MAX_MSG_COUNT > p_this->i_msg_count)
					{
						// Allocate memory for last message (free it in server_read function)
						p_this->s_last_msg = (char *)calloc(msg.ui_len, sizeof(char));
						if (NULL == p_this->s_last_msg)
						{
							LOG_ERROR("memory allocation for last message failed");
							return GSI_NET_RC_ERROR;
						}

						// Read the message form the connection_fd
						int i_count = 0;
						int i_res = 0;
						int i_len = msg.ui_len;
						while (0 != i_len)
						{
							i_count = read(p_this->i_connection_fd, p_this->s_last_msg + i_res, i_len);
							if (0 > i_count)
							{
								free(p_this->s_last_msg);
								p_this->s_last_msg = NULL;

								LOG_ERROR("read failed");
								return GSI_NET_RC_ERROR;
							}

							i_res += i_count;
							i_len -= i_count;
						}

						LOG_DEBUG("second read %d bytes from fd: %d", i_count, p_this->i_connection_fd);
						LOG_DEBUG("is message complete: %d", (strlen(p_this->s_last_msg) == (i_count - 1)));

						// Update the message counter
						++(p_this->i_msg_count);

						// Reset the heart beat in case that we are in the first message
						if (1 == p_this->i_msg_count)
						{
							p_this->i_heartbeat = 0;
						}

						LOG_INFO("has data");
						return GSI_NET_RC_HASDATA;
					}
					else if ((GSI_IS_MAX_MSG_COUNT <= p_this->i_msg_count) && (p_this->i_heartbeat != 1))
					{
						// Enter here if after MAX_MSG_COUNT messages there is no heart beat
						LOG_ERROR("client on port %d is not responding...closing connection", p_this->ui_port);
						return GSI_NET_RC_CONNECTERR;
					}
					break;
				}
				case GSI_HEARTBEAT_MSG:
				{
					// Enter here if we got heart beat message
					++(p_this->i_heartbeat);

					// Reset message counter for next phase
					p_this->i_msg_count = 0;
					LOG_INFO("got heartbeat from port: %d\n", p_this->ui_port);

					break;
				}
				default:
					LOG_ERROR("classified message failed");
					return GSI_NET_RC_ERROR;

			} /* end  of switch case */
		}
		else // i_count = 0 so there is connection error
		{
			LOG_ERROR("client on port %d closed his channel", p_this->ui_port);
			return GSI_NET_RC_CONNECTERR;
		}
	}

	return GSI_NET_RC_SUCCESS;
}
