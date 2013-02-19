#include "JobPipe_Module.h"
#include "std_defs.h"
/*
 * JobPipe_Module.c
 *
 * Created: 08/10/2012 21:39:04
 *  Author: NASSER
 */ 

////////////////////////////////////////////////////////////////////////////////
/// This is our Pipelined job processing system (holder of 2 jobs + 1 process)
////////////////////////////////////////////////////////////////////////////////

// Information about the result we're holding
buf_job_result_packet  __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
char 		   __buf_job_results_count;  // Total of results in our __buf_job_results

job_packet_p2p PIPE_PROC_BUF[PIPE_MAX_BUFFER_DEPTH];

void init_pipe_job_system()
{
	// Initialize our buffer
	__total_jobs_in_buffer = 0;
	__total_buf_p2p_jobs_ever_received = 0;

	// Put zeros everywhere
	unsigned int tx = 0;

	for (tx = 0; tx < sizeof(__inprocess_midstate); tx++) 	 __inprocess_midstate[tx] = 0;
	for (tx = 0; tx < sizeof(__inprocess_nonce_begin); tx++) __inprocess_nonce_begin[tx] = 0;
	for (tx = 0; tx < sizeof(__inprocess_nonce_end); tx++) 	 __inprocess_nonce_end[tx] = 0;

	// Reset results...
	__buf_job_results_count = 0;
}

void JobPipe__pipe_flush_buffer()
{
	// simply reset its counter;
	__buf_job_results_count = 0;
	__total_jobs_in_buffer = 0;	
}

char JobPipe__pipe_ok_to_pop()
{
	return ((__total_jobs_in_buffer > 0) ? 1 : 0);
}

char JobPipe__pipe_ok_to_push()
{
	return ((__total_jobs_in_buffer < PIPE_MAX_BUFFER_DEPTH) ? 1 : 0);
}

char JobPipe__pipe_push_P2P_job(void* __input_p2p_job_info)
{
	// Is it ok to push a job into stack?
	if (!JobPipe__pipe_ok_to_push()) return PIPE_JOB_BUFFER_FULL;

	// Copy memory block
	memcpy((void*)((char*)(PIPE_PROC_BUF) + (__total_jobs_in_buffer * sizeof(job_packet_p2p))),
	__input_p2p_job_info,
	sizeof(job_packet_p2p));

	// Increase the total jobs available in the stack
	__total_jobs_in_buffer++;

	// Proceed... All is ok!
	return PIPE_JOB_BUFFER_OK;
}

char JobPipe__pipe_pop_P2P_job(void* __output_p2p_job_info)
{
	// Is it ok to pop a job from the stack?
	if (!JobPipe__pipe_ok_to_pop()) return PIPE_JOB_BUFFER_EMPTY;

	// Copy memory block (from element 0)
	memcpy(__output_p2p_job_info,
	(void*)((char*)(PIPE_PROC_BUF) + (0 * sizeof(job_packet_p2p))),
	sizeof(job_packet_p2p));

	// Shift all elements back
	char tx = 0;

	for (tx = 1; tx < __total_jobs_in_buffer; tx++)
	{
		memcpy((void*)((char*)(PIPE_PROC_BUF) + ((tx - 1) * sizeof(job_packet_p2p))),
		(void*)((char*)(PIPE_PROC_BUF) + (tx * sizeof(job_packet_p2p))),
		sizeof(job_packet_p2p));
	}

	// Reduce total of jobs available in the stack
	__total_jobs_in_buffer--;

	// Proceed... All is ok!
	return PIPE_JOB_BUFFER_OK;
}


void* JobPipe__pipe_get_buf_job_result(unsigned int iIndex)
{
	return (void*)&__buf_job_results[iIndex];
}

unsigned int  JobPipe__pipe_get_buf_job_results_count(void)
{
	return __buf_job_results_count;
}

void  JobPipe__pipe_set_buf_job_results_count(unsigned int iCount)
{
	__buf_job_results_count = iCount;
}