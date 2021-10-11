/**************************************************************************
* Name : gsi_is_log_api.h 
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : API of MACROS to log messages into files
* 				uses in gsi_is_logger.h API to handle messages
* 				Using: 1. call gsi_is_create_log_file() first!
* 					   2. use any MACRO from API
* 					   3. call gsi_is_close_log() at the end of your program.
*****************************************************************************/
#ifndef GSI_IS_LOG_API_H_
#define GSI_IS_LOG_API_H_

/* Includes */
#include "gsi_is_logger.h"

/* Defines and Macros */
#define NO_LOG  (1)
#define ERROR   (2)
#define WARNING (3)
#define INFO    (4)
#define DEBUG   (5)

#define LOG_FILE gsi_get_saved_file() // return a pointer to opened log file

/* Logging MACROS */
#define LOG_ERROR(msg, args...)\
{\
	gsi_is_write_to_log(LOG_FILE, "%s - Log[ERROR]: in %s line %d: "msg"\n", gsi_is_gen_timestamp(), __FILE__, __LINE__, ##args); \
}

#define LOG_WARNING(msg, args...)\
{\
	gsi_is_write_to_log(LOG_FILE, "%s - Log[WARNNING]: in %s line %d: "msg"\n", gsi_is_gen_timestamp(), __FILE__, __LINE__, ##args); \
}

#define LOG_INFO(msg, args...)\
{\
	gsi_is_write_to_log(LOG_FILE, "%s - Log[INFO]: in %s line %d: "msg"\n", gsi_is_gen_timestamp(), __FILE__, __LINE__, ##args); \
}

#define LOG_DEBUG(msg, args...)\
{\
	gsi_is_write_to_log(LOG_FILE, "%s - Log[DEBUG]: in %s line %d: "msg"\n", gsi_is_gen_timestamp(), __FILE__, __LINE__, ##args); \
}


#if (LOG_LEVEL == NO_LOG)
    #undef LOG_ERROR
    #define LOG_ERROR(msg, args...) {}
    #undef LOG_WARNING
    #define LOG_WARNING(msg, args...) {}
    #undef LOG_INFO
    #define LOG_INFO(msg, args...) {}
    #undef LOG_DEBUG
    #define LOG_DEBUG(msg, args...) {}
#elif (LOG_LEVEL == ERROR)
    #undef LOG_WARNING
    #define LOG_WARNING(msg, args...) {}
    #undef LOG_INFO
    #define LOG_INFO(msg, args...) {}
    #undef LOG_DEBUG
    #define LOG_DEBUG(msg, args...) {}
#elif (LOG_LEVEL == WARNING)
    #undef LOG_INFO
    #define LOG_INFO(msg, args...) {}
    #undef LOG_DEBUG
    #define LOG_DEBUG(msg, args...) {}
#elif (LOG_LEVEL == INFO)
    #undef LOG_DEBUG
    #define LOG_DEBUG(msg, args...) {}
#elif (LOG_LEVEL > 5 || LOG_LEVEL < 1)
    #error "Must Define LOG_LEVEL in command line!"
#endif

#endif /* GSI_IS_LOG_API_H_ */
