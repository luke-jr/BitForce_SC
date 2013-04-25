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

// Job interleaving
char __interleaved_was_last_job_loaded_into_engines;
char __interleaved_loading_progress_chip;
char __interleaved_loading_progress_engine;
char __interleaved_loading_progress_finished;

// Information about the result we're holding
buf_job_result_packet __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
char __buf_job_results_count;  // Total of results in our __buf_job_results
job_packet PIPE_PROC_BUF[PIPE_MAX_BUFFER_DEPTH];

void init_pipe_job_system()
{
	// Initialize our buffer
	__total_jobs_in_buffer = 0;
	__total_buf_pipe_jobs_ever_received = 0;

	// Put zeros everywhere
	unsigned int tx = 0;
	for (tx = 0; tx < sizeof(__inprocess_midstate); tx++) __inprocess_midstate[tx] = 0;
	for (tx = 0; tx < sizeof(__inprocess_blockdata); tx++) __inprocess_blockdata[tx] = 0;
	for (tx = 0; tx < sizeof(__inprocess_SCQ_chip_processing); tx++) __inprocess_SCQ_chip_processing[tx] = 0;
	
	// Reset results...
	__buf_job_results_count = 0;
	
	// Clear interleaving
	__interleaved_loading_progress_chip = FALSE;
	__interleaved_loading_progress_engine = FALSE;
	__interleaved_loading_progress_finished = FALSE;
	__interleaved_was_last_job_loaded_into_engines = FALSE;
}

void JobPipe__pipe_flush_buffer()
{
	// simply reset its counter;
	#if defined(FLUSH_CLEAR_RESULTS_BUFFER)
		__buf_job_results_count = 0; // NOTE: CHANGED ON REQUEUST, DO NOT LOOSE JOB RESULTS!
	#endif
	__total_jobs_in_buffer	= 0;
	
	// Interleaved reset...
	__interleaved_was_last_job_loaded_into_engines = FALSE;
	__interleaved_loading_progress_finished = FALSE;
	__interleaved_loading_progress_engine = FALSE;
	__interleaved_loading_progress_chip = FALSE;	
}

inline char JobPipe__available_space()
{
	// simply reset its counter;
	return (__total_jobs_in_buffer > PIPE_MAX_BUFFER_DEPTH) ? 0 : (PIPE_MAX_BUFFER_DEPTH - __total_jobs_in_buffer);
}

inline char JobPipe__pipe_ok_to_pop()
{
	return ((__total_jobs_in_buffer > 0) ? 1 : 0);
}

inline char JobPipe__pipe_ok_to_push()
{
	return ((__total_jobs_in_buffer < PIPE_MAX_BUFFER_DEPTH) ? 1 : 0);
}

char JobPipe__pipe_push_job(void* __input_pipe_job_info)
{
	// Is it ok to push a job into stack?
	if (!JobPipe__pipe_ok_to_push()) return PIPE_JOB_BUFFER_FULL;

	// Copy memory block
	memcpy((void*)((char*)(PIPE_PROC_BUF) + (__total_jobs_in_buffer * sizeof(job_packet))),
		   __input_pipe_job_info, sizeof(job_packet));

	// Increase the total jobs available in the stack
	__total_jobs_in_buffer++;

	// Proceed... All is ok!
	return PIPE_JOB_BUFFER_OK;
}

char JobPipe__pipe_pop_job(void* __output_pipe_job_info)
{
	// Is it ok to pop a job from the stack?
	if (!JobPipe__pipe_ok_to_pop()) return PIPE_JOB_BUFFER_EMPTY;

	// Copy memory block (from element 0)
	memcpy(__output_pipe_job_info,
		  (void*)((char*)(PIPE_PROC_BUF) + (0 * sizeof(job_packet))),
		  sizeof(job_packet));

	// Shift all elements back
	char tx = 0;
	for (tx = 1; tx < __total_jobs_in_buffer; tx++)
	{
		memcpy((void*)((char*)(PIPE_PROC_BUF) + ((tx - 1) * sizeof(job_packet))),
			   (void*)((char*)(PIPE_PROC_BUF) + (tx * sizeof(job_packet))),
			   sizeof(job_packet));
	}

	// Reduce total of jobs available in the stack
	__total_jobs_in_buffer--;

	// Proceed... All is ok!
	return PIPE_JOB_BUFFER_OK;
}

char JobPipe__pipe_preview_next_job(void* __output_pipe_job_info)
{
	// If no job is in the buffer, don't do anything...
	if (__total_jobs_in_buffer == 0) return PIPE_JOB_BUFFER_EMPTY;
	
	// Copy memory block (from element 0)
	memcpy(__output_pipe_job_info,
		   (void*)((char*)(PIPE_PROC_BUF)),
		   sizeof(job_packet));

	// Proceed... All is ok!
	return PIPE_JOB_BUFFER_OK;	
}

void* JobPipe__pipe_get_buf_job_result(unsigned int iIndex)
{
	return (void*)&__buf_job_results[iIndex];
}

inline unsigned int JobPipe__pipe_get_buf_job_results_count(void)			 {return __buf_job_results_count;}
inline void JobPipe__pipe_set_buf_job_results_count(unsigned int iCount)	 {__buf_job_results_count = iCount;}
inline void JobPipe__set_was_last_job_loaded_in_engines(char iVal)			 {__interleaved_was_last_job_loaded_into_engines = iVal;}
inline char JobPipe__get_was_last_job_loaded_in_engines()					 {return __interleaved_was_last_job_loaded_into_engines;}
inline void JobPipe__set_interleaved_loading_progress_chip(char iVal)		 {__interleaved_loading_progress_chip = iVal;}
inline char JobPipe__get_interleaved_loading_progress_chip()				 {return __interleaved_loading_progress_chip;}
inline void JobPipe__set_interleaved_loading_progress_engine(char iVal)		 {__interleaved_loading_progress_engine = iVal;}
inline char JobPipe__get_interleaved_loading_progress_engine()				 {return __interleaved_loading_progress_engine;}
inline void JobPipe__set_interleaved_loading_progress_finished(char iVal)	 {__interleaved_loading_progress_finished = iVal;}
inline char JobPipe__get_interleaved_loading_progress_finished()			 {return __interleaved_loading_progress_finished;}