/*
 * PipeProcessingKernel.c
 *
 * Created: 03/06/2013 16:36:21
 *  Author: NASSER GHOSEIRI
 */ 
#include "std_defs.h"
#include "MCU_Initialization.h"
#include "JobPipe_Module.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include "USBProtocol_Module.h"
#include "A2D_Module.h"
#include "ASIC_Engine.h"
#include <avr32/io.h>
#include "HostInteractionProtocols.h"
#include "AVR32X/AVR32_Module.h"
#include "AVR32_OptimizedTemplates.h"
#include "PipeProcessingKernel.h"

#include <string.h>
#include <stdio.h>

// Information about the result we're holding
extern buf_job_result_packet __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
extern char __buf_job_results_count;  // Total of results in our __buf_job_results
static char _prev_job_was_from_pipe = FALSE;

char PipeKernel_WasPreviousJobFromPipe(void)
{
	return _prev_job_was_from_pipe;
}

// Global Variables
static job_packet    jpActiveJobBeingProcessed;
static unsigned int  iTotalEnginesFinishedActiveJob = 0;
static unsigned int  iNonceListForActualJob[8] = {0,0,0,0,0,0,0,0};
static unsigned char iNonceListForActualJob_Count = 0;
		
static job_packet	 jpInQueueJob;
static unsigned int  iTotalEnginesTakenInQueueJob = 0;
static unsigned char bInQueueJobExists = FALSE;

// Our Activity Map
static unsigned int  sEnginesActivityMap[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 16 chips Max
		
		
// =============================================================
// Buffer Management ===========================================
// =============================================================
#if defined(QUEUE_OPERATE_ONE_JOB_PER_BOARD)

	#if defined(__PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION)
		void PipeKernel_Spin()
		{
			// Declare our variables
			unsigned int iTotalEnginesCount = ASIC_get_processor_count();
			
			DEBUG_TraceTimer0 = MACRO_GetTickCountRet;			
					
			// Were we processing a job from queue before? If not, then check for engines availability.
			// If they are processing, we just exit the function (ZDX is at work or something else)
			// If not, then we have to dispatch a job... (assuming jobs are available in the queue)
			if (_prev_job_was_from_pipe == FALSE)						
			{
				// Is it already processing? If so, just exit...
				if (ASIC_is_processing() == TRUE)
				{
					// Before we exit, if __PROGRESSIV_MONITORING is enabled, we need to decommission bad chips
					#if defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)
					
						// For logging reasons
						char szTempEX[128];
						
						// Ok, we check engines. If anyone has been working extra hours, he has to go... :)
						for (char uzChip = 0; uzChip < TOTAL_CHIPS_INSTALLED; uzChip++)
						{
							if (!CHIP_EXISTS(uzChip)) continue;
							
							for (char uzEngine = 0; uzEngine < 16; uzEngine++)
							{
								#if defined(DO_NOT_USE_ENGINE_ZERO)
									if (uzEngine == 0) continue;
								#endif
								
								if (!IS_PROCESSOR_OK(uzChip, uzEngine)) continue;
								
								// Is it processing?
								int bIsEngineProcessing = ASIC_is_engine_processing(uzChip, uzEngine);
								
								// Is it processing for longer than it should have had?
								if (bIsEngineProcessing)
								{
									if (MACRO_GetTickCountRet - GLOBAL_ENGINE_PROCESSING_START_TIMESTAMP[uzChip][uzEngine] > (2*__ENGINE_PROGRESSIVE_MAXIMUM_BUSY_TIME))
									{
										// Decomission directly
										ASIC_reset_engine(uzChip, uzEngine);
										DECOMMISSION_PROCESSOR(uzChip, uzEngine);
										
										#if defined(__SHOW_DECOMMISSIONED_ENGINES_LOG)
											sprintf(szTempEX,"CHIP %d ENGINE %d DECOMMISSIONED!\n", uzChip, uzEngine);
											strcat(szDecommLog, szTempEX);
										#endif
										
										ASIC_calculate_engines_nonce_range();										
									}
								}
							}
						}
					#endif 
					
					return;
				}					 
				
				// Ok, special case here
				// The ASICs are NOT processing. All we need to do now is to take a job from the queue...
				// Put it in the InQueue. The rest will be done automatically...
				if (JobPipe__pipe_ok_to_pop() == TRUE)
				{
					// We take the job and issue it to engines
					// Then we also see if there is another job in the queue, if so, we will put it in InQueue section
					JobPipe__pipe_pop_job(&jpActiveJobBeingProcessed);
					iTotalEnginesFinishedActiveJob = 0;
					iNonceListForActualJob_Count = 0;
					ASIC_job_issue(&jpActiveJobBeingProcessed, 0x00000000, 0xFFFFFFFF, FALSE, 0);
					
					// Also, do we have any other jobs?
					if (JobPipe__pipe_ok_to_pop())
					{
						JobPipe__pipe_pop_job(&jpInQueueJob);		
						iTotalEnginesTakenInQueueJob = 0;
						bInQueueJobExists = TRUE;
					}
					else
					{
						iTotalEnginesTakenInQueueJob = 0;
						bInQueueJobExists = FALSE;
					}
								
					// Ok so previous job was from the pipe. We just deal with it...
					_prev_job_was_from_pipe = TRUE;
					return;
				}	
				else
				{
					// In this case, there really isn't anything to submit to chips. Just return...
					_prev_job_was_from_pipe = FALSE;
					return;	
				}																	
			}					
			
			DEBUG_TraceTimer1 = MACRO_GetTickCountRet - DEBUG_TraceTimer0;
			
			// Loading job into engines
			for (char iHoverChip = 0; iHoverChip < TOTAL_CHIPS_INSTALLED; iHoverChip++)			
			{
				// We won't take this chip into consideration if it doesn't exist...
				if (!CHIP_EXISTS(iHoverChip)) continue; 
				
				for (char iHoverEngine = 0; iHoverEngine < 16; iHoverEngine++)
				{
					// Should we skip engine 0?
					#if defined(DO_NOT_USE_ENGINE_ZERO)
						if (iHoverEngine == 0) continue;
					#endif 
					
					// Is engine operational?
					if (!IS_PROCESSOR_OK(iHoverChip, iHoverEngine)) continue; 
					
					// What is the status of the actual engine?
					unsigned int iNonceListForThisEngine[8];
					unsigned int iNonceCountForThisEngine = 0;
					int iEngineStatus = ASIC_get_job_status_from_engine(&iNonceListForThisEngine, &iNonceCountForThisEngine, iHoverChip, iHoverEngine);
					
					// Check status
					if ((iEngineStatus == ASIC_JOB_IDLE) || (iEngineStatus == ASIC_JOB_NONCE_NO_NONCE) || (iEngineStatus == ASIC_JOB_NONCE_FOUND))
					{
						// Have we already examined this engine? If so, don't examine it again
						if ( (sEnginesActivityMap[iHoverChip] & (1<<iHoverEngine)) == 0)
						{
							#if defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)
								if (GLOBAL_ENGINE_PROCESSING_FAILURE_SCORES[iHoverChip][iHoverEngine] > 0)
								{
									GLOBAL_ENGINE_PROCESSING_FAILURE_SCORES[iHoverChip][iHoverEngine]--;
								}
							#endif
													
							// Were we processing some job from queue before? If so, it means this engine has finished processing this job
							iTotalEnginesFinishedActiveJob++;
							
							// Mark the engine as examined
							sEnginesActivityMap[iHoverChip] |= (1<<iHoverEngine);
							
							// Are there any results? If so, save them...
							if ((iEngineStatus == ASIC_JOB_NONCE_FOUND))
							{
								// We have the result-nonces in iNonceListForThisEngine
								// Add them to the result for the actual job in process
								for (char iNonceHover = 0; iNonceHover < iNonceCountForThisEngine; iNonceHover++)
								{
									if (iNonceCountForThisEngine >= 8) continue; // We will no longer take nonces, buffer for this engine is full
									iNonceListForActualJob[iNonceListForActualJob_Count] = iNonceListForThisEngine[iNonceHover];
									iNonceListForActualJob_Count += 1;
								}
							}
							
							// Engine is no longer processing so...
							// The only thing we can do is sending it the job-in-queue (that is, if there is a job to be sent)
							if (bInQueueJobExists == TRUE)
							{
								// Read Complete first
								ASIC_ReadComplete(iHoverChip, iHoverEngine);
								
								// We send the job to this engine
								ASIC_job_issue_to_specified_engine (iHoverChip,
																	iHoverEngine,
																	&jpInQueueJob,
																	FALSE,  // = bLoadStaticData
																	TRUE,   // = bResetBeforeSubmission
																	TRUE,   // = bIgniteEngine
																	GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[iHoverChip][iHoverEngine],
																	GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[iHoverChip][iHoverEngine]);
								
								// Total engines taken the queue job increases
								iTotalEnginesTakenInQueueJob++;
							}
						}						
					}
					else if (iEngineStatus == ASIC_JOB_NONCE_PROCESSING)
					{
						#if defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)
							// Since it's processing, we won't bother...
							// However, we need to check how long this engine has been running.
							// If it has been running for longer than it should have (relative to operating frequency)
							// We will reset it.
							// Should this engine fail frequently, we'll completely decommission it...
							if ((MACRO_GetTickCountRet - GLOBAL_ENGINE_PROCESSING_START_TIMESTAMP[iHoverChip][iHoverEngine]) > __ENGINE_PROGRESSIVE_MAXIMUM_BUSY_TIME)
							{
								// First we restart the chip to prevent it from consuming power
								ASIC_reset_engine(iHoverChip, iHoverEngine);
								
								// Check the score, if it's 5 then we reset the engine. If it's 10 then we decommission it
								if (GLOBAL_ENGINE_PROCESSING_FAILURE_SCORES[iHoverChip][iHoverEngine] >= 3)
								{
									// We Decommission the engine while saying that the job was completed here. We also recalculate the nonce ranges
									DECOMMISSION_PROCESSOR(iHoverChip, iHoverEngine);
								
									// Recalculate
									ASIC_calculate_engines_nonce_range();
								
									// We also tell the system that the engine was finished (so the work can proceed)
									iTotalEnginesTakenInQueueJob++;
								}
								else
								{
									// In this case, we will assume that this will not always happen,
									// and probably reseting it every once in a while will resolve the issue
									// Increase the penalty score...
									GLOBAL_ENGINE_PROCESSING_FAILURE_SCORES[iHoverChip][iHoverEngine]++;	
								
									// We also tell the system that the engine was finished (so the work can proceed)
									iTotalEnginesTakenInQueueJob++;							
								}
							}
							else
							{
								// We're not in a bad situation with this engine so just proceed...
								// The engine is presumably still processing
							}
						#else	
							// If no supervision is requested, then just proceed without doing anything special...
						#endif 						
					}
				}				
			}
			
			DEBUG_TraceTimer2 = MACRO_GetTickCountRet - DEBUG_TraceTimer0;
			DEBUG_TraceTimers[DEBUG_TraceTimersIndex] = DEBUG_TraceTimer2;
			DEBUG_TraceTimersIndex += 1;
			if (DEBUG_TraceTimersIndex == 7) DEBUG_TraceTimersIndex = 0;
			
			
			
			
			// Ok, all the engines checked and nonces were collect if necessary (furthermore, next job in queue was issued)
			// Now, if all engines have finished processing the actual job, then it's time to push it to the results buffer
			if (iTotalEnginesFinishedActiveJob >= iTotalEnginesCount)
			{
				//////// -------- Clear our Activity Map
				sEnginesActivityMap[0] = 0; sEnginesActivityMap[4] = 0; sEnginesActivityMap[8] = 0;  sEnginesActivityMap[12] = 0; 
				sEnginesActivityMap[1] = 0; sEnginesActivityMap[5] = 0; sEnginesActivityMap[9] = 0;  sEnginesActivityMap[13] = 0; 
				sEnginesActivityMap[2] = 0; sEnginesActivityMap[6] = 0; sEnginesActivityMap[10] = 0; sEnginesActivityMap[14] = 0; 
				sEnginesActivityMap[3] = 0; sEnginesActivityMap[7] = 0; sEnginesActivityMap[11] = 0; sEnginesActivityMap[15] = 0; 
				
				//////// -------- Time to publish result in buffer, transfer InQueue job to ActiveJob (if it exists)
				// Verify the result counter is correct
				if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
				{
					// Then move all items one-index back (resulting in loss of the job-result at index 0)
					for (char pIndex = 0; pIndex < PIPE_MAX_BUFFER_DEPTH - 1; pIndex += 1) // PIPE_MAX_BUFFER_DEPTH - 1 because we don't touch the last item in queue
					{
						memcpy((void*)__buf_job_results[pIndex].midstate, 	(void*)__buf_job_results[pIndex+1].midstate, 	32);
						memcpy((void*)__buf_job_results[pIndex].block_data, (void*)__buf_job_results[pIndex+1].block_data, 	12);
						memcpy((void*)__buf_job_results[pIndex].nonce_list,	(void*)__buf_job_results[pIndex+1].nonce_list,  8*sizeof(UL32)); // 8 nonces maximum
					}
				}

				// Read the result and put it here...
				char i_result_index_to_put = 0;

				// Set the correct index
				i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ?  (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

				// Copy data...
				memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)jpActiveJobBeingProcessed.midstate, 	 32);
				memcpy((void*)__buf_job_results[i_result_index_to_put].block_data, 	(void*)jpActiveJobBeingProcessed.block_data, 12);
				memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)iNonceListForActualJob,	8*sizeof(int));
				__buf_job_results[i_result_index_to_put].i_nonce_count = iNonceListForActualJob_Count;

				// Increase the result count (if possible)
				if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) __buf_job_results_count++;				
				
				// If __buf_job_result is 4, then sent Mark2
				if (__buf_job_results_count == 4)
				{
					if (iMark2 == 0) iMark2 = MACRO_GetTickCountRet;
				}				
				
				// Update timestamp on the last job result produced
				GLOBAL_LastJobResultProduced = MACRO_GetTickCountRet;
				
				/////// -------- Also, clear the Results buffer, as they are no longer needed
				iNonceListForActualJob_Count = 0;
				iTotalEnginesFinishedActiveJob = 0; // Reset since next job is about to be processed
								
				// Do we have an InQueue Job?
				if (bInQueueJobExists == TRUE)
				{
					// We copy the InQueue job to ActiveJob.
					// Also, we try to see if we can take another job...
					memcpy((void*)&jpActiveJobBeingProcessed, (void*)&jpInQueueJob, sizeof(job_packet));
					iNonceListForActualJob_Count = 0;
					iTotalEnginesFinishedActiveJob = 0;
					
					// Take another job from queue
					if (JobPipe__pipe_ok_to_pop() == TRUE)
					{
						JobPipe__pipe_pop_job(&jpInQueueJob);
						bInQueueJobExists = TRUE;
						iTotalEnginesTakenInQueueJob = 0;
					}
					else
					{
						// No jobs exists for our InQueue
						bInQueueJobExists = FALSE;
						iTotalEnginesTakenInQueueJob = 0;	
					}
					
					_prev_job_was_from_pipe = TRUE; // As we are processing a job already, which is now our Active-Job
				}
				else
				{
					// Ok we're no longer processing jobs from the pipe
					_prev_job_was_from_pipe = FALSE;
				}				
			}

		}		
		
		
	
	#elif defined(__IMMEDIATE_ENGINE_JOB_SUBMISSION)
	
		void PipeKernel_Spin()
		{
			// Our flag which tells us where the previous job
			// was a P2P job processed or not :)

			// Take the job from buffer and put it here...
			// (We take element 0, and push all the arrays back...)
			if (ASIC_is_processing() == TRUE)
			{
				#if defined (__INTERLEAVED_JOB_LOADING)
					// Ok, since ASICs are processing, at this point we can start loading the next job in queue.
					// If the previous job is already loaded, then there isn't much we can do
					if (JobPipe__pipe_ok_to_pop() == FALSE)
					{
						// There is nothing to load, just return...
						return;
					}
		
					// Have we loaded something already and it has finished? If so, nothing to do as well
					if (JobPipe__get_interleaved_loading_progress_finished() == TRUE)
					{
						// Since the job is already loaded, there is nothing to do...
						return;
					}
		
					// Now we reach here, it means there is a job available for loading, which has not been 100% loaded.
					// Try to load one engine at a time...
					job_packet jp;
					char iRes = JobPipe__pipe_preview_next_job(&jp);
		
					// TODO: Take further action
					if (iRes == PIPE_JOB_BUFFER_EMPTY)
					{
						// This is a SERIOUS PROBLEM, it shouldn't be the case... (since we've already executed JobPipe__pipe_ok_to_pop)
					}
	
					// Ok, we have the JOB. We start looping...
					char iChipToProceed = JobPipe__get_interleaved_loading_progress_chip();
					char iEngineToProceed = JobPipe__get_interleaved_loading_progress_engine();
					char iChipHover = 0;
					char iEngineHover = 0;
		
					// Did we perform any writes? If not, it means we're finished
					char bDidWeWriteAnyEngines = FALSE;
					char bEngineWasNotFinishedSoWeAborted = FALSE;
		
					// Ok, we continue until we get the good chip
					for (iChipHover = iChipToProceed; iChipHover < TOTAL_CHIPS_INSTALLED; iChipHover++)		
					{
						// We reach continue to the end...
						if (!CHIP_EXISTS(iChipHover)) continue;
			
						// Hover the engines...
						for (iEngineHover = iEngineToProceed; iEngineHover < 16; iEngineHover++)
						{
							// Is this processor ok?
							if (!IS_PROCESSOR_OK(iChipHover, iEngineHover)) continue;
				
							// Is it engine 0 and we're restricted?
							#if defined(DO_NOT_USE_ENGINE_ZERO)
								if (iEngineHover == 0) continue;
							#endif
				
							// Is this engine finished? If yes then we'll write data to it. IF NOT THEN JUST FORGET IT! We abort this part altogether
							/*
							
							if (ASIC_has_engine_finished_processing(iChipHover, iEngineHover) == FALSE)
							{
								bEngineWasNotFinishedSoWeAborted = TRUE;
								break;
							}
							
							*/
				
							// Ok, now do Job-Issue to this engine on this chip
							ASIC_job_issue_to_specified_engine (iChipHover,
																iEngineHover,
																&jp,
																FALSE,
																FALSE,
																TRUE, 
																GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[iChipHover][iEngineHover],
																GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[iChipHover][iEngineHover]);				

							// Ok, Set the next engine to be processed...
							if (iEngineHover == 15)
							{
								JobPipe__set_interleaved_loading_progress_chip(iChipHover+1);
								JobPipe__set_interleaved_loading_progress_engine(0);
							}
							else
							{
								JobPipe__set_interleaved_loading_progress_chip(iChipHover);
								JobPipe__set_interleaved_loading_progress_engine(iEngineHover+1);					
							}
				
							// Set the flag
							bDidWeWriteAnyEngines = TRUE;
				
							// Exit the loop
							break;			
						}
			
						// Did we perform any writes? If so, we're done
						if (bDidWeWriteAnyEngines == TRUE) break;
			
						// Was the designated engine busy? If so, we exit
						if (bEngineWasNotFinishedSoWeAborted == TRUE) break;
					}
				
					// Did we write any engines? If NOT then we're finished
					if ((bDidWeWriteAnyEngines == FALSE) && (bEngineWasNotFinishedSoWeAborted == FALSE))
					{
						// Set/Reset related settings
						JobPipe__set_interleaved_loading_progress_finished(TRUE);
						JobPipe__set_interleaved_loading_progress_chip(0);
						JobPipe__set_interleaved_loading_progress_engine(0);			
			
					}
					else
					{
						// We're not finished
						JobPipe__set_interleaved_loading_progress_finished(FALSE);			
					}
				
					return; // We won't do anything, since there isn't anything we can do
			
				#else
					// No interleaved job loading, just return...
					return; // We won't do anything, since there isn't anything we can do
				#endif 
			}		 
	
			// Now if we're not processing, we MAY need to save the results if the previous job was from the pipe
			if (_prev_job_was_from_pipe == TRUE)
			{
				// Verify the result counter is correct
				if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
				{
					// Then move all items one-index back (resulting in loss of the job-result at index 0)
					for (char pIndex = 0; pIndex < PIPE_MAX_BUFFER_DEPTH - 1; pIndex += 1) // PIPE_MAX_BUFFER_DEPTH - 1 because we don't touch the last item in queue
					{
						memcpy((void*)__buf_job_results[pIndex].midstate, 	(void*)__buf_job_results[pIndex+1].midstate, 	32);
						memcpy((void*)__buf_job_results[pIndex].block_data, (void*)__buf_job_results[pIndex+1].block_data, 	12);
						memcpy((void*)__buf_job_results[pIndex].nonce_list,	(void*)__buf_job_results[pIndex+1].nonce_list,  8*sizeof(UL32)); // 8 nonces maximum
					}
				}

				// Read the result and put it here...
				unsigned int  i_found_nonce_list[16];
				volatile int i_found_nonce_count;
				char i_result_index_to_put = 0;
				ASIC_get_job_status(i_found_nonce_list, &i_found_nonce_count, FALSE, 0);

				// Set the correct index
				i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ?  (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

				// Copy data...
				memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)__inprocess_midstate, 		32);
				memcpy((void*)__buf_job_results[i_result_index_to_put].block_data, 	(void*)__inprocess_blockdata, 		12);
				memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)	 i_found_nonce_list, 		8*sizeof(int));
				__buf_job_results[i_result_index_to_put].i_nonce_count = i_found_nonce_count;

				// Increase the result count (if possible)
				if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) __buf_job_results_count++;
				
				// Also update the timestamp for the last job result detected
				GLOBAL_LastJobResultProduced = MACRO_GetTickCountRet;
			}
	
			// Ok, now we have recovered any potential results that could've been useful to us
			// Now, Are there any jobs in our pipe system? If so, we need to start processing on that right away and set _prev_job_from_pipe.
			// If not, we just clear _prev_job_from_pipe and exit
			if (JobPipe__pipe_ok_to_pop() == FALSE)
			{
				// Last job was not from pipe...
				_prev_job_was_from_pipe = FALSE;
		
				#if defined(__INTERLEAVED_JOB_LOADING)
					// Reset interleaved settings as well...
					JobPipe__set_interleaved_loading_progress_finished(FALSE);
					JobPipe__set_was_last_job_loaded_in_engines(FALSE);
					JobPipe__set_interleaved_loading_progress_chip(0); // Reset them
					JobPipe__set_interleaved_loading_progress_engine(0); // Reset them
				#endif
			
				// Exit the function. Nothing is left to do here...
				return;
			}

			// Ok so there are jobs that require processing. Suck them in and start processing
			// At this stage, we have know the ASICS have finished processing. We also know that there is a job to be sent
			// to the device for processing. However, we need to make a decision:
			//
			// 1) This job was interleaved-loaded into chips while they were processing. In this case just start the engines
			//
			// 2) No job was interleaved-loaded into chips. In this case, issue a full job. After that, ALSO perform a single interleaved-load action 
			//    if there are any more jobs to be loaded.
			//

			// ***********************************************************
			// We have something to pop, get it...
			#if defined(__INTERLEAVED_JOB_LOADING)
		
				if (JobPipe__get_interleaved_loading_progress_finished() == TRUE)
				{
					// We start the engines...
					char iEngineHover = 0;
					char iChipHover = 0;
		
					// Also POP the previous job, since it's already been fully transferred
					job_packet jpx_dummy;
					JobPipe__pipe_pop_job(&jpx_dummy);
		
					// Before we issue the job, we must put the correct information
					memcpy((void*)__inprocess_midstate, 	(void*)jpx_dummy.midstate, 	 32);
					memcpy((void*)__inprocess_blockdata, 	(void*)jpx_dummy.block_data, 12);
		
					// Ok, now no job is loaded in interleaved mode since the old was is now being processed
					JobPipe__set_interleaved_loading_progress_finished(FALSE);
					JobPipe__set_interleaved_loading_progress_chip(0);
					JobPipe__set_interleaved_loading_progress_engine(0);
				}
				else
				{
					// We have to issue a new job...
					job_packet job_from_pipe;
					if (JobPipe__pipe_pop_job(&job_from_pipe) == PIPE_JOB_BUFFER_EMPTY)
					{
						// This is odd!!! THIS SHOULD NEVER HAPPEN, however we do set things here by default...
						_prev_job_was_from_pipe = FALSE; // Obviously, this must be cleared...
						JobPipe__set_interleaved_loading_progress_finished(FALSE);
						JobPipe__set_interleaved_loading_progress_chip(0);
						JobPipe__set_interleaved_loading_progress_engine(0);
						return;
					}

					// Before we issue the job, we must put the correct information
					memcpy((void*)__inprocess_midstate, 	(void*)job_from_pipe.midstate, 	 32);
					memcpy((void*)__inprocess_blockdata, 	(void*)job_from_pipe.block_data, 12);

					// Send it to processing...
					ASIC_job_issue(&job_from_pipe, 0x0, 0xFFFFFFFF, FALSE, 0);	
				}
			
			#else
				// No interleaved job loading
				// We have to issue a new job...
				job_packet job_from_pipe;
				if (JobPipe__pipe_pop_job(&job_from_pipe) == PIPE_JOB_BUFFER_EMPTY)
				{
					// This is odd!!! THIS SHOULD NEVER HAPPEN, however we do set things here by default...
					_prev_job_was_from_pipe = FALSE; // Obviously, this must be cleared...
					return;
				}

				// Before we issue the job, we must put the correct information
				memcpy((void*)__inprocess_midstate, 	(void*)job_from_pipe.midstate, 	 32);
				memcpy((void*)__inprocess_blockdata, 	(void*)job_from_pipe.block_data, 12);

				// Send it to processing...
				ASIC_job_issue(&job_from_pipe, 0x0, 0xFFFFFFFF, FALSE, 0);		
			#endif 
	
			// The job is coming from the PIPE...
			// Set we simply have to set the flag here
			_prev_job_was_from_pipe = TRUE;
		}	
	
	#endif

	// NOTE: Not Implemented, doesn't make any difference
	char Flush_buffer_into_single_chip(char iChip)
	{
		// FUNCTION NOT IMPLEMENTED HERE IN SINGLE-JOB-PER-BOARD MODE
	}
	
#elif defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)

	#define SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS	1
	#define SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE		2
	#define SINGLE_CHIP_QUEUE__CHIP_BUSY			3
	
	void PipeKernel_Spin()
	{
		// Run the process for all available chips on board
		char iTotalChips = ASIC_get_chip_count();
		char iTotalTookJob = 0;
		char iTotalNoJobsAvailable = 0;
		char iTotalProcessing = 0;
		char iRetStat = 0;
		char iHover = 0;
	
		for (iHover = 0; iHover < TOTAL_CHIPS_INSTALLED; iHover++)
		{
			if (!CHIP_EXISTS(iHover)) continue;
			iRetStat = Flush_buffer_into_single_chip(iHover);
			iTotalTookJob += (iRetStat == SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS) ? 1 : 0;
			iTotalNoJobsAvailable += (iRetStat == SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE) ? 1 : 0;
			iTotalProcessing += (iRetStat == SINGLE_CHIP_QUEUE__CHIP_BUSY) ? 1 : 0;			
		}		
		
		// If anyone (even only a single chip) took a job, then we can consider that previous job was from pipe
		if (iTotalTookJob > 0)
		{
			// A queue job is being processed...
			_prev_job_was_from_pipe = TRUE;	
		}
		
		// We can determine if the _prev_job_was_from_queue is TRUE or FALSE
		if (iTotalNoJobsAvailable == iTotalChips)
		{
			// Meaning no job was available. We're done now...
			_prev_job_was_from_pipe = FALSE;
		}		
	}	
	
	char Flush_buffer_into_single_chip(char iChip)
	{
		// Our flag which tells us where the previous job
		// was a P2P job processed or not :)
		// Take the job from buffer and put it here...
		// (We take element 0, and push all the arrays back...)
		if (ASIC_is_chip_processing(iChip) == TRUE)
		{
			return SINGLE_CHIP_QUEUE__CHIP_BUSY; // We won't do anything, since there isn't anything we can do
		}

		// Now if we're not processing, we MAY need to save the results if the previous job was from the pipe
		// This is conditioned on whether this chip was processing a single-job or not...
		if ((_prev_job_was_from_pipe == TRUE) && (__inprocess_SCQ_chip_processing[iChip] == TRUE)) 
		{
			// Verify the result counter is correct
			if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
			{
				// Then move all items one-index back (resulting in loss of the job-result at index 0)
				for (char pIndex = 0; pIndex < PIPE_MAX_BUFFER_DEPTH - 1; pIndex += 1) // PIPE_MAX_BUFFER_DEPTH - 1 because we don't touch the last item in queue
				{
					__buf_job_results[pIndex].iProcessingChip = __buf_job_results[pIndex+1].iProcessingChip;
					
					memcpy((void*)__buf_job_results[pIndex].midstate, 	(void*)__buf_job_results[pIndex+1].midstate, 	32);
					memcpy((void*)__buf_job_results[pIndex].block_data, (void*)__buf_job_results[pIndex+1].block_data, 	12);
					memcpy((void*)__buf_job_results[pIndex].nonce_list,	(void*)__buf_job_results[pIndex+1].nonce_list,  8*sizeof(UL32)); // 8 nonces maximum
					__buf_job_results[pIndex].i_nonce_count = __buf_job_results[pIndex+1].i_nonce_count;
				}
			}

			// Read the result and put it here...
			unsigned int  i_found_nonce_list[16];
			volatile int i_found_nonce_count;
			char i_result_index_to_put = 0;
			ASIC_get_job_status(i_found_nonce_list, &i_found_nonce_count, TRUE, iChip);

			// Set the correct index
			i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ? (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

			// Copy data...
			memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)__inprocess_SCQ_midstate[iChip],		32);
			memcpy((void*)__buf_job_results[i_result_index_to_put].block_data, 	(void*)__inprocess_SCQ_blockdata[iChip],	12);
			memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)i_found_nonce_list,		8*sizeof(int));
			__buf_job_results[i_result_index_to_put].iProcessingChip = iChip;
			__buf_job_results[i_result_index_to_put].i_nonce_count = i_found_nonce_count;

			// Increase the result count (if possible)
			if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH)
			{
				 __buf_job_results_count++;
			}				 

			// Update timestamp on the last job result produced
			GLOBAL_LastJobResultProduced = MACRO_GetTickCountRet
			
			// We return, since there is nothing left to do
		}

		// Ok, now we have recovered any potential results that could've been useful to us
		// Now, Are there any jobs in our pipe system? If so, we need to start processing on that right away and set _prev_job_from_pipe.
		// If not, we just clear _prev_job_from_pipe and exit
		if (JobPipe__pipe_ok_to_pop() == FALSE)
		{
			// Exit the function. Nothing is left to do here...
			__inprocess_SCQ_chip_processing[iChip] = FALSE;
			return SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE;
		}

		// We have to issue a new job...
		job_packet job_from_pipe;
		if (JobPipe__pipe_pop_job(&job_from_pipe) == PIPE_JOB_BUFFER_EMPTY)
		{
			// This is odd!!! THIS SHOULD NEVER HAPPEN, however we do set things here by default...
			__inprocess_SCQ_chip_processing[iChip] = FALSE;
			return SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE;
		}

		// Before we issue the job, we must put the correct information
		if (iChip == 7)
		{
			memcpy((void*)__inprocess_SCQ_midstate[iChip], 	(void*)job_from_pipe.midstate, 	 32);
			memcpy((void*)__inprocess_SCQ_blockdata[iChip], (void*)job_from_pipe.block_data, 12);
		}
		else
		{
			memcpy((void*)__inprocess_SCQ_blockdata[iChip], (void*)job_from_pipe.block_data, 12);			
			memcpy((void*)__inprocess_SCQ_midstate[iChip], 	(void*)job_from_pipe.midstate, 	 32);
		}
		
		
		// Send it to processing...
		ASIC_job_issue(&job_from_pipe, 0x0, 0xFFFFFFFF, TRUE, iChip);
		
		// This chip is processing now
		__inprocess_SCQ_chip_processing[iChip] = TRUE;
		
		// The engine took a job
		return SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS;	
	}

#endif

// ======================================================================================
// ======================================================================================
// ======================================================================================
