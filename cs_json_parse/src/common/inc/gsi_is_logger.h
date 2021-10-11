/**************************************************************************
* Name : gsi_is_logger.h
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : API functions to handle log messages into files.
* 				Using: 1. gsi_is_create_log_file() - Must be first!
* 					   2. gsi_get_saved_file() - call this on every use of write into log.
* 					   3. gsi_is_write_to_log() - use as much as you want.
* 					   4. gsi_is_close_log() - Must be last!
*****************************************************************************/
#ifndef GSI_IS_LOGGER_H
#define GSI_IS_LOGGER_H

 /* Includes */
#include <stdio.h>

/* Defines and Macros */
#define 	GSI_IS_LOG_MAX_FILE_NAME  256

/* Enums */
/***************************************************************************
 * Name:  gsi_is_log_rc
 * Description: Return Code values for GSI-LOG functions
 ***************************************************************************/
enum gsi_is_log_rc
{
	GSI_LOG_RC_SUCCESS,		// function completed successfully
	GSI_LOG_RC_OPEN_ERROR,  // failed to open the file
	GSI_LOG_RC_CLOSE_ERROR, // failed to close the file
	GSI_LOG_RC_WRITE_ERROR, // failed to write into file
	GSI_LOG_RC_INVALID,		// function got invalid arguments
	GSI_LOG_RC_MEMORY_ERROR,// memory alocation failed
	GSI_LOG_RC_ERROR		// general error
};

/*******************/
/* API Declaration */
/*******************/
/*###########################################################################
	 * Name:		gsi_is_create_log_file
	 * Description: Generate a log file, and return it's file descriptor.
	 *              Name of file is in the following format:
	 *              "log_path/log_name-YYYY-MM-DD-HH.MM.SS.log"
	 * Parameter:   [in] const char *s_log_name - base name for the log file.
	 * Parameter:   [in] const char *s_log_path - path for the log file.
	 * Return:		Success - FILE* - handler for the opened log_file
	 * 				Failure - NULL
#############################################################################*/
FILE* gsi_is_create_log_file(const char *s_log_name, char *s_log_path);


/*###########################################################################
	 * Name:		gsi_get_saved_file
	 * Description: return the FILE* pointer used to write to the log file
	 * Parameter:   [in] None
	 * Return:		FILE* - pointer to file
#############################################################################*/
FILE* gsi_get_saved_file();

/*###########################################################################
	 * Name:		gsi_is_title_to_log
	 * Description: Write title to the log file.
	 * Parameter:   [in] FILE* f_log - Handler to the log file.
	 * Parameter:   [in] const char* s_title - content of the title to be logged
	 * Return:		Success - GSI_LOG_RC_SUCCESS
	 * 				Failure - GSI_LOG_RC_WRITE_ERROR *OR* GSI_LOG_RC_INVALID *OR* GSI_LOG_RC_MEMORY_ERROR
#############################################################################*/
enum gsi_is_log_rc gsi_is_title_to_log(FILE* f_log, const char* s_title);


/*###########################################################################
	 * Name:		gsi_is_write_to_log
	 * Description: Write data to the log file.
	 * Parameter:   [in] FILE* f_log - Handler to the log file.
	 * Parameter:   [in] const char* s_format - Format of the data to be logged,
	 * 									  		using the same format as printf().
	 * Parameter:   [in] ... - List of parameters to be logged, must match s_format.
	 * Return:		Success - GSI_LOG_RC_SUCCESS
	 * 				Failure - GSI_LOG_RC_WRITE_ERROR *OR* GSI_LOG_RC_INVALID *OR* GSI_LOG_RC_MEMORY_ERROR
#############################################################################*/
enum gsi_is_log_rc gsi_is_write_to_log(FILE* f_log, const char* s_format, ...);

/*###########################################################################
	 * Name:		gsi_is_close_log
	 * Description: Close the log file. farther use is undefined.
	 * Parameter:   [in] FILE* f_log - Handler to the log file.
	 * Return:		Success - GSI_LOG_RC_SUCCESS
	 * 				Failure - GSI_LOG_RC_CLOSE_ERROR *OR* GSI_LOG_RC_INVALID
#############################################################################*/
enum gsi_is_log_rc gsi_is_close_log(FILE* f_log);


/*###########################################################################
	 * Name:		gsi_is_gen_timestamp
	 * Description: Generate time stamp as string : "YYYY-MM-DD-HH:MM:SS"
	 * Parameter:   [in] None
	 * Return:		char* - time stamp as string
#############################################################################*/
char* gsi_is_gen_timestamp();

#endif // GSI_IS_LOGGER_H
