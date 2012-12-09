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
unsigned int	__total_buf_p2p_jobs_ever_received;

// In process information
char  __inprocess_midstate[32]; // This is the midstate of which we are processing at this moment
char  __inprocess_nonce_begin[4]; // This is the midstate of which we are showing the result in status
char  __inprocess_nonce_end[4]; // This is the midstate of which we are showing the result in status

void init_pipe_job_system(void);
char __pipe_ok_to_push(void);
char __pipe_ok_to_pop(void);
char __pipe_push_P2P_job(void* __input_p2p_job_info);
char __pipe_pop_P2P_job(void* __output_p2p_job_info);
void*				 __pipe_get_buf_job_result(unsigned int iIndex);
unsigned int		 __pipe_get_buf_job_results_count(void);
void				 __pipe_set_buf_job_results_count(unsigned int iCount);
void				 __pipe_flush_buffer(void);

#endif /* MCU_INITIALIZATION_H_ */