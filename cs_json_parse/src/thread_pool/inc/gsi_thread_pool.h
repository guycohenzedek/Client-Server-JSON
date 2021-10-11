/**************************************************************************
* Name : gsi_thread_pool.h 
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Thread Pool API
*****************************************************************************/
#ifndef GSI_IS_THREAD_POOL_H_
#define GSI_IS_THREAD_POOL_H_

/* Includes */
#include <pthread.h>

/* Defines and Macros */
#define 	GSI_IS_MAX_THREADS	   5
#define 	GSI_IS_MAX_QUEUE_SIZE 200

/* Typedef */

// pointer to function for threads
typedef void* (*thread_func_t)(void* args);

// structures typedefs
typedef struct gsi_thread_pool gsi_thread_pool_t;
typedef struct gsi_thread_pool_task gsi_thread_pool_task_t;

/* Enums */
/***************************************************************************
 * Name:		thread_pool_destroy_flags
 * Description: Flags for shutdown the thread pool
 ***************************************************************************/
enum gsi_thread_pool_destroy_flags {
    GSI_TP_DESTROY_GRACEFUL  = 1,	// Execute all tasks in Queue and quit
	GSI_TP_DESTROY_IMMIDIATE = 2	// Quit even if there are tasks in Queue
};

/***************************************************************************
 * Name:  		gsi_thread_pool_rc
 * Description: Return Code values for GSI-THREAD-POOL functions
 ***************************************************************************/
enum gsi_thread_pool_rc {
	GSI_TP_RC_SUCCESS = 0,	// Function completed Successfully
	GSI_TP_RC_ERROR   = 1,	// Function completed with Error
	GSI_TP_RC_INVALID = 2	// Function got invalid arguments
};

/* Structures */

/*****************************************************************************
 * Name : gsi_thread_pool_task
 * Used by: struct gsi_thread_pool
 * Members:
 *----------------------------------------------------------------------------
 *		thread_func_t thread_func - Pointer to function that that will perform a task
 *----------------------------------------------------------------------------
 *		void *args - Argument to be passed to the function
 *****************************************************************************/
struct gsi_thread_pool_task
{
    thread_func_t thread_func;
    void* args;
};

/*****************************************************************************
 * Name : gsi_thread_pool
 * Used by: GSI-THREAD-POOL API functions
 * Members:
 *----------------------------------------------------------------------------
 *		pthread_mutex_t lock - Mutex to lock critical sections.
 *----------------------------------------------------------------------------
 *		pthread_cond_t notify - Condition variable to notify worker threads.
 *----------------------------------------------------------------------------
 *		pthread_t *p_threads - Array containing worker threads ID
 *----------------------------------------------------------------------------
 *		gsi_thread_pool_task_t *queue - Array containing the task queue.
 *----------------------------------------------------------------------------
 *		int i_thread_count - Number of threads.
 *----------------------------------------------------------------------------
 *		int i_queue_size - Size of the task queue.
 *----------------------------------------------------------------------------
 *		int i_head - Index of the first element.
 *----------------------------------------------------------------------------
 *		int i_tail - Index of the last element.
 *----------------------------------------------------------------------------
 *		int i_count - Number of pending tasks.
 *----------------------------------------------------------------------------
 *		int i_shutdown - Flag indicating if the pool is shutting down.
 *----------------------------------------------------------------------------
 *		int i_started - Number of started threads.
 *****************************************************************************/
struct gsi_thread_pool
{
	pthread_mutex_t lock;
	pthread_cond_t notify;
	pthread_t* p_threads;
	gsi_thread_pool_task_t* queue;
	int i_thread_count;
	int i_queue_size;
	int i_head;
	int i_tail;
	int i_count;
	int i_shutdown;
	int i_started;
};

/*******************/
/* API Declaration */
/*******************/
/*###########################################################################
	 * Name:   		gsi_is_thread_pool_create
	 * Description: Creates a gsi_thread_pool_t object
	 * Parameter:   [in] int i_thread_count - number of threads in pool
	 * Parameter:   [in] int i_queue_size - size of the queue
	 * Return: 	    Success - pointer to new thread_pool object
	 * 				Failure - NULL
#############################################################################*/
gsi_thread_pool_t *gsi_is_thread_pool_create(int i_thread_count, int i_queue_size);

/*###########################################################################
	 * Name:      	gsi_is_thread_pool_add
	 * Description: Add a new task in the queue of a thread pool.
	 * Parameter:   [in] gsi_thread_pool_t *p_pool - Thread pool to which add the task.
	 * Parameter:   [in] thread_func_t thread_func - Pointer to the function that will perform the task.
	 * Parameter:   [in] void* args - Argument to be passed to the function.
	 * Return: 	    Success - GSI_TP_RC_SUCCESS
	 * 				Failure - GSI_TP_RC_ERROR *OR* GSI_TP_RC_INVALID
#############################################################################*/
enum gsi_thread_pool_rc gsi_is_thread_pool_add(gsi_thread_pool_t* p_pool, thread_func_t thread_func, void* args);

/*###########################################################################
	 * Name:        gsi_is_thread_pool_destroy
	 * Description: Stops and destroys a thread pool.
	 * Parameter:   [in] gsi_thread_pool_t *p_pool - Thread pool to destroy.
	 * Parameter:   [in] int i_flags - Flags for shutdown - 1 default, 2 - graceful
	 * Return: 	    Success - GSI_TP_RC_SUCCESS
	 * 				Failure - GSI_TP_RC_ERROR *OR* GSI_TP_RC_INVALID
#############################################################################*/
enum gsi_thread_pool_rc gsi_is_thread_pool_destroy(gsi_thread_pool_t* p_pool, int i_flags);


#endif /* GSI_THREAD_POOL_H_ */
