#include <mpi.h>
#include "abcdnews.h"

void reporter(MPI_Comm editor_contact, MPI_Comm informant_contact, MPI_Comm collegues_contact, MPI_Datatype news_t) {	
	int my_rank, num_reporters;

	MPI_Comm_rank(collegues_contact, &my_rank);
	MPI_Comm_size(collegues_contact, &num_reporters);

	int meeting_count = 0;
	double session_start;
	int collective_count;
	int ping_count;

	int incomplete_tip, incomplete_bcast, incomplete_ping;
	int tip_flag, bcast_flag, ping_flag;

	MPI_Request tip_request, ping_request, bcast_request;
	MPI_Status tip_status, ping_status, bcast_status;
	int bcast_buffer, ping_buffer;
	newsitem tip_buffer;
	

	newsitem *queue = NULL;
	newsitem *gather_queue = NULL;
	newsitem *forward_to_editor_queue = NULL;

	int queue_length;
	int forward_queue_length;


	int gather_recvcounts[num_reporters];
	int gather_recvdispls[num_reporters];

	incomplete_tip = 0;
	incomplete_bcast = 0;
	tip_flag = 0;
	bcast_flag = 0;

	queue = createQueue(2*MAX_REPORTER_COLLECTIVE_COUNT);
	gather_queue = createQueue(3*MAX_REPORTER_COLLECTIVE_COUNT);
	forward_to_editor_queue = createQueue(3*MAX_REPORTER_COLLECTIVE_COUNT);

	while (1) {

		incomplete_bcast = 0;
		collective_count = 0;
		incomplete_ping = 0;

		queue_length = 0;		

		if ((meeting_count % num_reporters) == my_rank) {
			//lead reporter

			session_start = MPI_Wtime();
			while (1) {
				if ((MPI_Wtime() - session_start) > MAX_REPORTER_SESSION_DURATION) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, collegues_contact, &bcast_request);
					MPI_Wait(&bcast_request, &bcast_status);
					//printf("Broadcast complete1! Collective count: %d\n", collective_count);
					break;
				}

				if (collective_count > MAX_REPORTER_COLLECTIVE_COUNT) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, collegues_contact, &bcast_request);
					MPI_Wait(&bcast_request, &bcast_status);
					//printf("Broadcast complete2! Collective count: %d\n", collective_count);
					break;
				}

				if (incomplete_tip) {
					MPI_Test(&tip_request, &tip_flag, &tip_status);

					if (tip_flag) {
						//printf("Received information %d in process %d\n", tip_buffer, my_rank);
						insert(queue, &tip_buffer, &queue_length);
						collective_count++;
						incomplete_tip = 0;
					}
				}

				if (!incomplete_tip) {
					tip_flag = 0;
					MPI_Irecv(&tip_buffer, 1, news_t, informant_rank, NEWS_TAG, informant_contact, &tip_request);
					incomplete_tip = 1;					
				}

				if (incomplete_ping) {
					MPI_Test(&ping_request, &ping_flag, &ping_status);
					
					if (ping_flag) {
						collective_count++;
						incomplete_ping = 0;
					}
				}

				if (!incomplete_ping) {
					ping_flag = 0;
					MPI_Irecv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, collegues_contact, &ping_request);
					incomplete_ping = 1;
				}
			}

			if (incomplete_ping) {
				MPI_Test(&ping_request, &ping_flag, &ping_status);

				if (ping_flag) {
					collective_count ++;
					incomplete_ping = 0;
				} else {
					MPI_Cancel(&ping_request);
					incomplete_ping = 0;
				}
			}

			// canceling send operation is expensive and not guaranteed
			ping_flag = 0;
			while (1) {
				MPI_Iprobe(MPI_ANY_SOURCE, PING_TAG, collegues_contact, &ping_flag, &ping_status);

				if (!ping_flag) {
					break;
				}
				else {
					MPI_Recv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, collegues_contact, MPI_STATUS_IGNORE);
					ping_flag = 0;
					collective_count ++;
					incomplete_ping = 0;
				}
			}

			MPI_Gatherv(queue, queue_length, news_t, gather_queue, gather_recvcounts, gather_recvdispls, news_t, my_rank, collegues_contact);			
			//Need to online sort here! 
			//qsort(gather_queue, sum(gather_recvcounts), sizeof(newsitem), compare_newsitems);
			forward_queue_length = 0;
			for (int i = 0; i < sumArray(gather_recvcounts, num_reporters); i++)
				insert(forward_to_editor_queue, &gather_queue[i], &forward_queue_length);

			MPI_Send(forward_to_editor_queue, forward_queue_length, news_t, editor_rank, EDITOR_FORWARD_TAG, editor_contact);


			meeting_count++;
			printf("Gather in process : %d Collective count: %d\n\n", my_rank, collective_count);

		} else {
			//normal reporter
			ping_count = 0;
			bcast_flag = 0;
			MPI_Ibcast(&bcast_buffer, 1, MPI_INT, (meeting_count % num_reporters), collegues_contact, &bcast_request);
			incomplete_bcast = 1;
			incomplete_ping = 0;

			while (1) {
				if (incomplete_bcast) {
					MPI_Test(&bcast_request, &bcast_flag, &bcast_status);

					if (bcast_flag) {
						incomplete_bcast = 0;
						break;
					}
				}

				if (incomplete_tip) {
					MPI_Test(&tip_request, &tip_flag, &tip_status);
					if (tip_flag) {

						//printf("Received information %d in process %d\n", tip_buffer, my_rank);
						insert(queue, &tip_buffer, &queue_length);
						ping_count++;
						incomplete_tip = 0;
					}
				}

				if (!incomplete_tip) {
					tip_flag = 0;
					MPI_Irecv(&tip_buffer, 1, news_t, informant_rank, NEWS_TAG, informant_contact, &tip_request);
					incomplete_tip = 1;
				}

				if (incomplete_ping) {
					MPI_Test(&ping_request, &ping_flag, &ping_status);
					if (ping_flag) {
						ping_count--;
						incomplete_ping = 0;
					}
				}

				if (!incomplete_ping && ping_count > 0) {
					MPI_Isend(&ping_buffer, 1, MPI_INT, (meeting_count % num_reporters), PING_TAG, collegues_contact, &ping_request);
					incomplete_ping  = 1;
					ping_flag = 0;
				}
			}

			if (incomplete_ping) {
				MPI_Wait(&ping_request, &ping_status);
				// MPI_Cancel(&ping_request);
				ping_count --;
			}

			while (ping_count > 0) {
				MPI_Send(&ping_buffer, 1, MPI_INT, (meeting_count % num_reporters), PING_TAG, collegues_contact);
				ping_count --;
			}

			MPI_Gatherv(queue, queue_length, news_t, NULL, NULL, NULL, news_t, (meeting_count % num_reporters), collegues_contact);
			meeting_count ++;
		}
	}

	free(queue);
}