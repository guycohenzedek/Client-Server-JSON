/**************************************************************************
* Name : gsi_is_logger.c
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : API implementation to handle log messages into files.
* 				Using: 1. gsi_is_create_log_file() - Must be first!
* 					   2. gsi_get_saved_file() - call this on every use of write into log.
* 					   3. gsi_is_write_to_log() - use as much as you want.
* 					   4. gsi_is_close_log() - Must be last!
*****************************************************************************/

/* Includes */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "gsi_thread_pool.h"
#include "gsi_is_logger.h"

/* Defines and Macros */
#define 	GSI_IS_LOG_TRUE			  1
#define 	GSI_IS_LOG_FALSE			  0
#define 	GSI_IS_LOG_THREADS   	  1
#define 	GSI_IS_LOG_QUEUE		  100
#define 	GSI_IS_LOG_DEFAULT_PATH   "/var/log"
#define	 	GSI_IS_LOG_TIME_STAMP_LEN sizeof("YYYY-MM-DD-HH:MM:SS")

#if (LOG_LEVEL == DEBUG)
	#define  	GSI_IS_LOG_MAX_MSG_LEN	  1100  // In debug level there are also writing messages into log so we need huge buffer.
												// base message length: 1024 bytes
												// wrapper content of log message: 50-80 bytes
#else
	#define 	GSI_IS_LOG_MAX_MSG_LEN	  256
#endif

/* Structures */
struct gsi_thread_args
{
	pthread_t thread_id;
	FILE* f_log;
	char s_message[GSI_IS_LOG_MAX_MSG_LEN];
};

/********************************/
/* Static functions declaration */
/********************************/
static void* thread_write_log(void* p_args);

/* Global variables */
gsi_thread_pool_t* g_p_thread_pool = NULL; /* global thread pool pointer */
FILE* g_p_log_file = NULL;				   /* global log file pointer */

/**********************/
/* API implementation */
/**********************/
/*###########################################################################
	 * Name:		gsi_is_create_log_file
	 * Description: Generate a log file, and return it's file descriptor.
	 *              Name of file is in the following format:
	 *              "log_path/log_name-YYYY-MM-DD-HH:MM:SS.log"
	 * Parameter:   [in] const char *s_log_name - base name for the log file.
	 * Parameter:   [in] const char *s_log_path - path for the log file.
	 * 											  if NULL - set it to default path
	 * Return:		Success - FILE* - handler for the opened log_file
	 * 				Failure - NULL
#############################################################################*/
FILE* gsi_is_create_log_file(const char *s_log_name, char *s_log_path)
{
	FILE* f_log = NULL;
	char s_file_name[GSI_IS_LOG_MAX_FILE_NAME];
	int i_is_path_allocated = GSI_IS_LOG_FALSE;

	// Check input validation
	if (NULL == s_log_name)
	{
		printf("gsi_is_create_log_file: invalid argument\n");
		return NULL;
	}

	// Check if path is NULL - set it to default path
	if (NULL == s_log_path)
	{
		s_log_path = malloc(sizeof(GSI_IS_LOG_DEFAULT_PATH));
		if (NULL == s_log_path)
		{
			printf("gsi_is_create_log_file: memory allocation for path failed\n");
			return NULL;
		}

		i_is_path_allocated = GSI_IS_LOG_TRUE;
		strcpy(s_log_path, GSI_IS_LOG_DEFAULT_PATH);
	}

	// Create thread pool from parameters in config file
	g_p_thread_pool = gsi_is_thread_pool_create(GSI_IS_LOG_THREADS, GSI_IS_LOG_QUEUE);
	if (NULL == g_p_thread_pool)
	{
		printf("gsi_is_create_log_file: memory allocation failed\n");
		return NULL;
	}

	// Build the log file name
	sprintf(s_file_name, "%s/%s-%s.log", s_log_path, s_log_name, gsi_is_gen_timestamp());

	// Open file in append mode
	f_log = fopen(s_file_name, "a");
	if (NULL == f_log)
	{
		printf("gsi_is_create_log_file: failed to open log file\n");
		gsi_is_thread_pool_destroy(g_p_thread_pool, GSI_TP_DESTROY_IMMIDIATE);
		return NULL;
	}

	// Check if we allocated the path
	if (GSI_IS_LOG_TRUE == i_is_path_allocated)
	{
		free (s_log_path);
		s_log_path = NULL;
	}

	// Initialize global pointer to log file
	g_p_log_file = f_log;
	return f_log;
}

/*###########################################################################
	 * Name:		gsi_get_saved_file
	 * Description: return the FILE* pointer used to write to the log file
	 * Parameter:   [in] None
	 * Return:		FILE* - pointer to file
#############################################################################*/
FILE* gsi_get_saved_file()
{
	return g_p_log_file;
}

/*###########################################################################
	 * Name:		gsi_is_title_to_log
	 * Description: Write title to the log file.
	 * Parameter:   [in] FILE* f_log - Handler to the log file.
	 * Parameter:   [in] const char* s_title - content of the title to be logged
	 * Return:		Success - GSI_LOG_RC_SUCCESS
	 * 				Failure - GSI_LOG_RC_WRITE_ERROR *OR* GSI_LOG_RC_INVALID *OR* GSI_LOG_RC_MEMORY_ERROR
#############################################################################*/
enum gsi_is_log_rc gsi_is_title_to_log(FILE* f_log, const char* s_title)
{
	// Check input validation
	if ((NULL == f_log) || (NULL == s_title))
	{
		printf("gsi_is_title_to_log: invalid argumets\n");
		return GSI_LOG_RC_INVALID;
	}

	fprintf(f_log,
			"\n+++++++++++++++++++++++++++\n"
			"+\t[%s]   +\n"
			"+++++++++++++++++++++++++++\n",
			  s_title);
	fflush(f_log);

	return GSI_LOG_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_write_to_log
	 * Description: Write data to the log file.
	 * Parameter:   [in] FILE* f_log -  Handler to the log file.
	 * Parameter:   [in] const char* s_format - Format of the data to be logged,
	 * 									  		using the same format as printf().
	 * Parameter:   [in] ... - List of parameters to be logged, must match s_format.
	 * Return:		Success - GSI_LOG_RC_SUCCESS
	 * 				Failure - GSI_LOG_RC_WRITE_ERROR *OR* GSI_LOG_RC_INVALID *OR* GSI_LOG_RC_MEMORY_ERROR
#############################################################################*/
enum gsi_is_log_rc gsi_is_write_to_log(FILE* f_log, const char* s_format, ...)
{
	va_list optional_args;
	struct gsi_thread_args* p_thread_args = NULL;

	// Check input validation
	if ((NULL == f_log) || (NULL == s_format))
	{
		printf("gsi_is_write_to_log: invalid argumets\n");
		return GSI_LOG_RC_INVALID;
	}

	// Allocate memory for thread argument structure. free by running thread in thread_write_log().
	p_thread_args = (struct gsi_thread_args *)malloc(sizeof(struct gsi_thread_args));
	if (NULL == p_thread_args)
	{
		return GSI_LOG_RC_MEMORY_ERROR;
	}

	// Set fields
	p_thread_args->thread_id = pthread_self();
	p_thread_args->f_log = f_log;

	// Reset s_message
	memset(p_thread_args->s_message, 0, sizeof(p_thread_args->s_message));

	// Build message
	va_start(optional_args, s_format);
	vsprintf(p_thread_args->s_message, s_format, optional_args);
	va_end(optional_args);

	// Add message work to Queue
	if (GSI_TP_RC_SUCCESS != gsi_is_thread_pool_add(g_p_thread_pool, thread_write_log, p_thread_args))
	{
		if (NULL != p_thread_args)
		{
			free(p_thread_args);
			p_thread_args = NULL;
		}

		return GSI_LOG_RC_WRITE_ERROR;
	}

	return GSI_LOG_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_close_log
	 * Description: Close the log file. farther use is undefined.
	 * Parameter:   [in] FILE* f_log - Handler to the log file.
	 * Return:		Success - GSI_LOG_RC_SUCCESS
	 * 				Failure - GSI_LOG_RC_CLOSE_ERROR *OR* GSI_LOG_RC_INVALID
#############################################################################*/
enum gsi_is_log_rc gsi_is_close_log(FILE* f_log)
{
	// Check input validation
	if (NULL == f_log)
	{
		return GSI_LOG_RC_INVALID;
	}

	// Destroy thread pool gracefully to write all the messages in the Queue
	gsi_is_thread_pool_destroy(g_p_thread_pool, GSI_TP_DESTROY_GRACEFUL);

	// Close the log file
	if (0 != fclose(f_log))
	{
		return GSI_LOG_RC_CLOSE_ERROR;
	}

	// Reset pointer
	f_log = NULL;

	// Reset global pointer
	g_p_log_file = NULL;

	return GSI_LOG_RC_SUCCESS;
}

/*###########################################################################
	 * Name:		gsi_is_gen_timestamp
	 * Description: Generate time stamp as string : "YYYY-MM-DD-HH:MM:SS"
	 * Parameter:   [in] None
	 * Return:		char* - time stamp as string
#############################################################################*/
char* gsi_is_gen_timestamp()
{
	static char s_time_stamp[GSI_IS_LOG_TIME_STAMP_LEN];

	time_t t = time(NULL);
	struct tm *curr_time = localtime(&t);

	sprintf(s_time_stamp, "%04d-%02d-%02d-%02d:%02d:%02d",
			curr_time->tm_year + 1900,
			curr_time->tm_mon + 1,
			curr_time->tm_mday,
			curr_time->tm_hour,
			curr_time->tm_min,
			curr_time->tm_sec);

	return s_time_stamp;
}

/***********************************/
/* Static functions implementation */
/***********************************/
/*###########################################################################
	 * Name:		thread_write_log
	 * Description: Thread work function to write messages to log
	 * 				Must free the s_message field that allocated by the calling function
	 * Parameter:   [in] void* p_args - pointer to struct gsi_thread_args
	 * Return:		Always NULL
#############################################################################*/
static void* thread_write_log(void* p_args)
{
	struct gsi_thread_args* p_thread_args = (struct gsi_thread_args*)p_args;

	// Check input validation
	if (NULL == p_args)
	{
		printf("p_args is NULL\n");
		return NULL;
	}

	do
	{
		if (NULL == p_thread_args->f_log)
		{
			printf("f_log is NULL\n");
			break;
		}

		if (NULL == p_thread_args->s_message)
		{
			printf("s_message is NULL\n");
			break;
		}

		// Write the message into log file
		fputs(p_thread_args->s_message, p_thread_args->f_log);
		fflush(p_thread_args->f_log);
	}
	while (0);

	free(p_args);
	p_args = NULL;

	return NULL;
}
