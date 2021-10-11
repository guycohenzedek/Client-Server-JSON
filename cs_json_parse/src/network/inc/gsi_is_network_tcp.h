/**************************************************************************
* Name : gsi_is_network_tcp.h 
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Include file and function prototypes for
 * 				TCP/IP (IPv4) Interface
*****************************************************************************/
#ifndef GSI_IS_NETWORK_TCP_H_
#define GSI_IS_NETWORK_TCP_H_

/* Includes */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

/* Defines and Macros */
#define 	GSI_IS_MAX_CONN		2

/* Enums */
/***************************************************************************
 * Name:  gsi_is_network_return_code
 * Description: Return Code values for GSI-NET functions
 ***************************************************************************/
enum gsi_is_network_return_code {
	GSI_NET_RC_SUCCESS 	  = 0,		// Function completed Successfully
	GSI_NET_RC_ERROR 	  = 1,		// Function completed with Error
	GSI_NET_RC_ABORT 	  = 2,		// Function requires abort (unrecoverable error)
	GSI_NET_RC_CONNECTERR = 4,		// Connection Error
	GSI_NET_RC_EOF 		  = 16,		// End of File Reached
	GSI_NET_RC_HASDATA    = 128		// Data is Available
};

/***************************************************************************
 * Name:		gsi_is_type_message
 * Description: Message types that send between client and server
 ***************************************************************************/
enum gsi_is_type_message {
    GSI_REGULAR_MSG   = 1,	// Message that contains data
	GSI_HEARTBEAT_MSG = 2,	// Message that contains heart beat alert
	GSI_COMMENT 	  = 3   // Line is comment
};

/* Structures */
/*****************************************************************************
 * Name : gsi_net_tcp
 * Used by:	TCP Server and TCP Client Interfaces
 * Members:
 *----------------------------------------------------------------------------
 *		char *s_tcp_addr		- Address to Connect to:
 *								  <ip address>:<port>
 *----------------------------------------------------------------------------
 *		char *s_hostname		- IP address
 *----------------------------------------------------------------------------
 *		unsigned int ui_port	- Port number
 *----------------------------------------------------------------------------
 *		char s_last_msg[GSI_NET_STRING_SIZE] - content of last message
 *----------------------------------------------------------------------------
 *		int	 i_listen_fd		- Socket to listen for connections
 *----------------------------------------------------------------------------
 *		int  i_connection_fd	- Socket used to connect with Partner
 *								  Client / Server
 *----------------------------------------------------------------------------
 *		int i_heartbeat			- Indicate that heart beat was sent
 *----------------------------------------------------------------------------
 *		int i_msg_count		- Counting the number of message until heart beat
 *----------------------------------------------------------------------------
 * 		struct sockaddr_in serv_addr - SockAddr_In structure.
 *								  	   Describer connection address for
 *								  	   socket interface.
 *----------------------------------------------------------------------------
 * 		struct pollfd pfds[MAX_CONN] - Array of POLL elements.
 *****************************************************************************/
struct gsi_net_tcp {
	char *s_tcp_addr;
	char *s_hostname;
	char *s_last_msg;

	int	i_listen_fd;
	int i_connection_fd;
	int i_heartbeat;
	int i_msg_count;
	unsigned int ui_port;

	struct sockaddr_in serv_addr;
	struct pollfd pfds[GSI_IS_MAX_CONN];
};

/*****************************************************************************
 * Name : gsi_cs_tcp_message
 * Used by:	TCP Server and TCP Client for communication
 * Warning: The order of the members is important - DONT change this!
 * Members:
 *----------------------------------------------------------------------------
 *		unsigned int ui_port - Port number
 *----------------------------------------------------------------------------
 *		enum gsi_is_type_message e_type_msg - Message Type: Regular / Heart Beat
 *----------------------------------------------------------------------------
 *		unsigned int ui_len - message length
 *----------------------------------------------------------------------------
 *		char *s_message		- message content
 *****************************************************************************/
struct gsi_cs_tcp_message
{
	unsigned int ui_port;
	enum gsi_is_type_message e_type_msg;
	unsigned int ui_len;
	char *s_message;
};

/*******************/
/* API Declaration */
/*******************/
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
enum gsi_is_network_return_code gsi_is_network_tcp_reset(struct gsi_net_tcp* p_this);


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
																int i_port);

/********************/
/* Client Functions */
/********************/
/*###########################################################################
	 * Name:		gsi_is_network_tcp_client_init
	 * Description:	Initializes an Instance of struct TCP Client,
	 * 				Connects over TCP
	 * Parameter:   [out] struct gsi_net_tcp *p_this - pointer to structure TCP Client
	 * Parameter:   [in] char* s_tcp_addr - Address to connect to <IPAddress>:<Port>
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_client_init(struct gsi_net_tcp *p_this, char* s_tcp_addr);

/*###########################################################################
	 * Name:		gsi_is_network_tcp_connect
	 * Description: Create a Socket and connect to server.
	 * Parameter:   [in]  struct sockaddr_in *p_serv_addr - address of server (IP Address + Port).
	 * Parameter:   [out] int *p_socket_fd - pointer to socket file descriptor.
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_connect(struct sockaddr_in *p_serv_addr,
													 	   int *p_socket_fd);


/*###########################################################################
	 * Name:		gsi_is_network_tcp_send
	 * Description: Send a single message over an IPv4 TCP socket
	 * 				Generates a new connection for each message.
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP
	 * Parameter:   [in] char *s_msg  - message to send
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR *OR* GSI_NET_RC_CONNECTERR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_send(struct gsi_net_tcp *p_this,
														char *s_msg);


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
enum gsi_is_network_return_code gsi_is_network_tcp_server_init(struct gsi_net_tcp *p_this, unsigned int ui_port);


/*###########################################################################
	 * Name:		gsi_is_network_tcp_server_poll
	 * Description:	Polls if TCP Server has available pending
	 * 			    connections or Data on an established connection,
	 * 			    If pending connections are available, they 1st connection is
	 * 			    accepted.
	 * 			    If data is available on an established connection,
	 * 			    returns GSI_NET_RC_HASDATA.
	 *				POLL allows the TCP server to work in a non-blocking mode
	 *				which allows the server to perform other tasks while
	 *				waiting for either a connection or DATA.
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Return:		Success - GSI_NET_RC_HASDATA *OR* GSI_NET_RC_SUCCESS(empty buffer)
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_server_poll(struct gsi_net_tcp *p_this);


/*###########################################################################
	 * Name:		gsi_is_network_tcp_server_read
	 * Description: Read one message from the TCP Server (Connection FD).
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Parameter:   [out] char *s_msg - buffer to fill with the read message.
	 * Return:		Success 		- GSI_NET_RC_SUCCESS
	 * 				Buffer is Empty	- GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_server_read(struct gsi_net_tcp *p_this,
														 	   char *s_msg);


/*###########################################################################
	 * Name:		gsi_is_network_tcp_server_cleanup
	 * Description: Cleans up the TCP Server.
	 * Parameter:   [in] struct gsi_net_tcp *p_this - pointer to structure TCP Server
	 * Return:		Success - GSI_NET_RC_SUCCESS
	 * 				Failure - GSI_NET_RC_ERROR
#############################################################################*/
enum gsi_is_network_return_code gsi_is_network_tcp_server_cleanup(struct gsi_net_tcp *p_this);


#endif /* GSI_IS_NETWORK_TCP_H_ */
