/*
 * JobPipe_Module.h
 *
 * Created: 08/10/2012 21:38:56
 *  Author: NASSER
 */ 
#ifndef JOBPIPE_MODULE_H_
#define JOBPIPE_MODULE_H_

// Job structure...

#define SHA256_Test_String  "2f33a240bd6785caed5b67b4122079dd9359004ae23c64512c5c2dfbce097b08"

/// ************************** This is our Pipelined job processing system (holder of 2 jobs + 1 process)
#define PIPE_MAX_BUFFER_DEPTH	 20
#define PIPE_JOB_BUFFER_OK		 0
#define PIPE_JOB_BUFFER_FULL	 1
#define PIPE_JOB_BUFFER_EMPTY	 2

unsigned int    __total_jobs_in_buffer;
unsigned int	__total_buf_pipe_jobs_ever_received;

// In process information
volatile char   __inprocess_midstate[32]; // This is the midstate of which we are processing at this moment
volatile char   __inprocess_blockdata[12]; // This is the midstate of which we are processing at this moment

void			init_pipe_job_system(void);
char			JobPipe__available_space(void);
char			JobPipe__pipe_ok_to_push(void);
char			JobPipe__pipe_ok_to_pop(void);
char			JobPipe__pipe_push_job(void* __input_job_info);
char			JobPipe__pipe_pop_job (void* __output_job_info);
void*			JobPipe__pipe_get_buf_job_result(unsigned int iIndex);
unsigned int	JobPipe__pipe_get_buf_job_results_count(void);
void			JobPipe__pipe_set_buf_job_results_count(unsigned int iCount);
void			JobPipe__pipe_flush_buffer(void);
char			JobPipe__pipe_preview_next_job(void* __output_pipe_job_info);
void			JobPipe__was_last_job_loaded_in_engines(void);
void			JobPipe__set_last_job_loaded_in_engines(char iVal);

#endif /* MCU_INITIALIZATION_H_ */