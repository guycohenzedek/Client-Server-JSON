/**************************************************************************
* Name : gsi_thread_pool.c
* Author : Guy Cohen Zedek
* Version : 1.0.0
* Description : Thread Pool API implementation.
* 				Every use of thread_pool_create() must also use thread_pool_destroy() !
*****************************************************************************/

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include "gsi_thread_pool.h"

/* Defines and Macros */

/********************************/
/* Static functions declaration */
/********************************/
static void* gsi_is_thread_run(void* args);
static enum gsi_thread_pool_rc gsi_is_thread_pool_free(gsi_thread_pool_t* p_pool);

/*###########################################################################
	 * Name:   		thread_pool_create
	 * Description: Creates a gsi_thread_pool_t object. Must Use thread_pool_destroy() before exit!
	 * Parameter:   [in] int i_thread_count - number of threads in pool
	 * Parameter:   [in] int i_queue_size - size of the queue
	 * Return: 	    Success - pointer to new thread_pool object
	 * 				Failure - NULL
#############################################################################*/
gsi_thread_pool_t* gsi_is_thread_pool_create(int i_thread_count, int i_queue_size)
{
	gsi_thread_pool_t* p_pool = NULL;

	// Check input validation
	if ((0 >= i_thread_count) || (GSI_IS_MAX_THREADS < i_thread_count) ||
		(0 >= i_queue_size)   || (GSI_IS_MAX_QUEUE_SIZE < i_queue_size))
	{
		printf("gsi_is_thread_pool_create: invalid arguments\n");
		return NULL;
	}

	// Allocate new thread pool
	p_pool = (gsi_thread_pool_t*) malloc(sizeof(gsi_thread_pool_t));
	if (NULL == p_pool)
	{
		printf("gsi_is_thread_pool_create: malloc failed\n");
		return NULL;
	}

	// Update fields
	p_pool->i_thread_count = 0;
	p_pool->i_queue_size = i_queue_size;
	p_pool->i_head = 0;
	p_pool->i_tail = 0;
	p_pool->i_count = 0;
	p_pool->i_shutdown = 0;
	p_pool->i_started = 0;

	// Allocate array of threads
	p_pool->p_threads = (pthread_t*)malloc(sizeof(pthread_t) * i_thread_count);
	if (NULL == p_pool->p_threads)
	{
		free(p_pool);
		return NULL;
	}

	// Allocate queue of tasks
	p_pool->queue = (gsi_thread_pool_task_t*)malloc(sizeof(gsi_thread_pool_task_t) * i_queue_size);
	if (NULL == p_pool->queue)
	{
		free(p_pool->p_threads);
		free(p_pool);
		return NULL;
	}

	// Init the mutex
	if (0 != pthread_mutex_init(&(p_pool->lock), NULL))
	{
		free(p_pool->queue);
		free(p_pool->p_threads);
		free(p_pool);
		return NULL;
	}

	// Init the condition variable
	if (0 != pthread_cond_init(&(p_pool->notify), NULL))
	{
		pthread_mutex_destroy(&(p_pool->lock));
		free(p_pool->queue);
		free(p_pool->p_threads);
		free(p_pool);
		return NULL;
	}

	// Start worker threads
	for (int i = 0; i < i_thread_count; ++i)
	{
		if (0 != pthread_create(&(p_pool->p_threads[i]), NULL, gsi_is_thread_run, p_pool))
		{
			gsi_is_thread_pool_destroy(p_pool, GSI_TP_DESTROY_IMMIDIATE);
			return NULL;
		}

		// Add 1 to the number of current working threads
		p_pool->i_thread_count++;
		p_pool->i_started++;
	}

	return p_pool;
}

/*###########################################################################
	 * Name:      	thread_pool_add
	 * Description: Add a new task in the queue of a thread pool.
	 * Parameter:   [in] gsi_thread_pool_t *p_pool - Thread pool to which add the task.
	 * Parameter:   [in] thread_func_t thread_func - Pointer to the function that will perform the task.
	 * Parameter:   [in] void* args - Argument to be passed to the function.
	 * Return: 	    Success - GSI_TP_RC_SUCCESS
	 * 				Failure - GSI_TP_RC_ERROR *OR* GSI_TP_RC_INVALID
#############################################################################*/
enum gsi_thread_pool_rc gsi_is_thread_pool_add(gsi_thread_pool_t* p_pool, thread_func_t thread_func, void* args)
{
	// Check input validation
	if ((NULL == p_pool) || (NULL == thread_func))
	{
		return GSI_TP_RC_INVALID;
	}

	// Try to lock the mutex to own the thread pool
	if (0 != pthread_mutex_lock(&(p_pool->lock)))
	{
		return GSI_TP_RC_ERROR;
	}

	// Check if the queue is full
	if (p_pool->i_count == p_pool->i_queue_size)
	{
		return GSI_TP_RC_ERROR;
	}

	// Check if we are in shutdown
	if (0 < p_pool->i_shutdown)
	{
		return GSI_TP_RC_ERROR;
	}

	// Add task to queue
	p_pool->queue[p_pool->i_tail].thread_func = thread_func;
	p_pool->queue[p_pool->i_tail].args = args;

	// Update the tail index
	p_pool->i_tail = (p_pool->i_tail + 1) % p_pool->i_queue_size;

	// Update the number of pending tasks
	p_pool->i_count++;

	// Notify that there is new work in queue
	if (0 != pthread_cond_broadcast(&(p_pool->notify)))
	{
		return GSI_TP_RC_ERROR;
	}

	// Unlock the mutex
	if (0 != pthread_mutex_unlock(&p_pool->lock))
	{
		return GSI_TP_RC_ERROR;
	}

	return GSI_TP_RC_SUCCESS;
}

/*###########################################################################
	 * Name:        thread_pool_destroy
	 * Description: Stops and destroys a thread pool. Must be called after use of thread_pool_create()
	 * Parameter:   [in] gsi_thread_pool_t *p_pool - Thread pool to destroy.
	 * Parameter:   [in] int i_flags - Flags for shutdown - 1 default, 2 - graceful
	 * Return: 	    Success - GSI_TP_RC_SUCCESS
	 * 				Failure - GSI_TP_RC_ERROR *OR* GSI_TP_RC_INVALID
#############################################################################*/
enum gsi_thread_pool_rc gsi_is_thread_pool_destroy(gsi_thread_pool_t* p_pool, int i_flags)
{
	// Check input validation
	if (NULL == p_pool)
	{
		return GSI_TP_RC_INVALID;
	}

	// Try to lock the mutex to own the thread pool
	if (0 != pthread_mutex_lock(&(p_pool->lock)))
	{
		return GSI_TP_RC_ERROR;
	}

	// Check if already in shutdown
	if (0 != p_pool->i_shutdown)
	{
		return GSI_TP_RC_ERROR;
	}

	// Update shutdown according to i_flags
	if (GSI_TP_DESTROY_GRACEFUL == i_flags)
	{
		p_pool->i_shutdown = GSI_TP_DESTROY_GRACEFUL;
	}
	else
	{
		p_pool->i_shutdown = GSI_TP_DESTROY_IMMIDIATE;
	}

	// Wake up all threads
	if (0 != pthread_cond_broadcast(&(p_pool->notify)))
	{
		return GSI_TP_RC_ERROR;
	}

	// Unlock the mutex to get threads continue
	if (0 != pthread_mutex_unlock(&(p_pool->lock)))
	{
		return GSI_TP_RC_ERROR;
	}

	// Join all worker thread
	for (int i = 0; i < p_pool->i_thread_count; ++i)
	{
		if (0 != pthread_join(p_pool->p_threads[i], NULL))
		{
			return GSI_TP_RC_ERROR;
		}
	}

	// Free the thread pool
	gsi_is_thread_pool_free(p_pool);

	return GSI_TP_RC_SUCCESS;
}

/***********************************/
/* Static functions implementation */
/***********************************/
/*###########################################################################
	 * Name:		thread_run
	 * Description: That is the main loop of each thread in thread pool
	 * Parameter:   [in] void* args - must be pointer to thread pool object
	 * Return:		NULL
#############################################################################*/
static void* gsi_is_thread_run(void* args)
{
	gsi_thread_pool_t* p_pool = (gsi_thread_pool_t*)args;
	gsi_thread_pool_task_t task;

	// Check input validation
	if (NULL == args)
	{
		return NULL;
	}

	// Main loop
	while (1)
	{
		// Lock the mutex
		if (0 != pthread_mutex_lock(&(p_pool->lock)))
		{
			break;
		}

		// Wait on condition variable until we have tasks, check for spurious wakeups.
        // When returning from pthread_cond_wait(), we own the lock.
		while ((p_pool->i_count == 0) && (!p_pool->i_shutdown))
		{
			pthread_cond_wait(&(p_pool->notify), &(p_pool->lock));
		}

		// Check destroy status
		if ((p_pool->i_shutdown == GSI_TP_DESTROY_IMMIDIATE) ||
		   ((p_pool->i_shutdown == GSI_TP_DESTROY_GRACEFUL) && (p_pool->i_count == 0)))
		{
			break;
		}

		// Get the first task
		task.thread_func = p_pool->queue[p_pool->i_head].thread_func;
		task.args = p_pool->queue[p_pool->i_head].args;

		// Update the head index
		p_pool->i_head = (p_pool->i_head + 1) % p_pool->i_queue_size;

		// Decrease the number of pending tasks
		p_pool->i_count--;

		// Unlock the mutex
		if (0 != pthread_mutex_unlock(&(p_pool->lock)))
		{
			break;
		}

		// Go to work
		(*(task.thread_func))(task.args);
	}

	// Decrease the number of working threads
	p_pool->i_started--;

	// Unlock the mutex
	pthread_mutex_unlock(&(p_pool->lock));

	return NULL;
}

/*###########################################################################
	 * Name:		thread_pool_free
	 * Description: clean up all the allocations of threadpool_create
	 * Parameter:   [in] - gsi_thread_pool_t* p_pool - object to be free
	 * Return:		Success - GSI_TP_RC_SUCCESS
	 * 				Failure - GSI_TP_RC_ERROR *OR* GSI_TP_RC_INVALID
#############################################################################*/
static enum gsi_thread_pool_rc gsi_is_thread_pool_free(gsi_thread_pool_t* p_pool)
{
	// Check input validation
	if (NULL == p_pool)
	{
		return GSI_TP_RC_INVALID;
	}

	// Check if there are still running threads
	if (0 < p_pool->i_started)
	{
		return GSI_TP_RC_ERROR;
	}

	free(p_pool->p_threads);
	free(p_pool->queue);

	if (0 != pthread_mutex_destroy(&(p_pool->lock)))
	{
		return GSI_TP_RC_ERROR;
	}

	if (0 != pthread_cond_destroy(&(p_pool->notify)))
	{
		return GSI_TP_RC_ERROR;
	}

	free(p_pool);

	return GSI_TP_RC_SUCCESS;
}
