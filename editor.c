#include <mpi.h>
#include "abcdnews.h"
#include <stdlib.h>

void editor(MPI_Comm editors_contact, MPI_Comm org_comm, MPI_Datatype news_t) {
	int i, world_rank;

	int my_rank, num_editors, num_reporters;
	MPI_Comm_rank(editors_contact, &my_rank);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(editors_contact, &num_editors);
	MPI_Comm_size(org_comm, &num_reporters);	
	num_reporters -= 1;		// editor is process 0 in org_comm

	int meeting_count = 0;
	double session_start;
	int collective_count;
	int ping_count;

	int incomplete_tip, incomplete_bcast, incomplete_ping;
	int tip_flag, bcast_flag, ping_flag;

	MPI_Request tip_request, ping_request, bcast_request;
	MPI_Status tip_status, ping_status, bcast_status;
	int bcast_buffer, ping_buffer;


	newsitem *queue = NULL;
	newsitem *tip_buffer = NULL;
	newsitem *gather_queue = NULL;
	newsitem *livetelecast_queue = NULL;

	int queue_length;
	int livetelecast_queue_length;

	int gather_recvcounts[num_editors];
	int gather_recvdispls[num_editors];

	incomplete_tip = 0;
	incomplete_bcast = 0;
	tip_flag = 0;
	bcast_flag = 0;

	tip_buffer = createQueue(3*MAX_REPORTER_COLLECTIVE_COUNT);
	queue = createQueue(2*MAX_REPORTER_COLLECTIVE_COUNT*MAX_EDITOR_COLLECTIVE_COUNT);
	gather_queue = createQueue(3*MAX_REPORTER_COLLECTIVE_COUNT*MAX_EDITOR_COLLECTIVE_COUNT);
	livetelecast_queue = createQueue(3*MAX_REPORTER_COLLECTIVE_COUNT*MAX_EDITOR_COLLECTIVE_COUNT);

	while (1) {
		incomplete_bcast = 0;
		collective_count = 0;
		incomplete_ping = 0;

		queue_length = 0;		

		if ((meeting_count % num_editors) == my_rank) {
			//lead reporter

			session_start = MPI_Wtime();
			while (1) {
				if ((MPI_Wtime() - session_start) > MAX_EDITOR_SESSION_DURATION) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, editors_contact, &bcast_request);
					MPI_Wait(&bcast_request, &bcast_status);
					printf("Broadcast complete1! Collective count: %d\n", collective_count);
					break;
				}

				if (collective_count > MAX_EDITOR_COLLECTIVE_COUNT) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, editors_contact, &bcast_request);
					MPI_Wait(&bcast_request, &bcast_status);
					printf("Broadcast complete2! Collective count: %d\n", collective_count);
					break;
				}

				if (incomplete_tip) {
					MPI_Test(&tip_request, &tip_flag, &tip_status);

					if (tip_flag) {
						//printf("Received information %d in process %d\n", tip_buffer, my_rank);
						//insert(queue, &tip_buffer, queue_length);
						int num_messages;
						MPI_Get_count(&tip_status, MPI_INT, &num_messages);
						for (i = 0; i < num_messages; i++)
							insert(queue, &tip_buffer[i], &queue_length);
						printf("Received information in editor %d\n", world_rank);
						collective_count++;
						incomplete_tip = 0;
					}
				}

				if (!incomplete_tip) {
					tip_flag = 0;
					MPI_Irecv(tip_buffer, 3*MAX_REPORTER_COLLECTIVE_COUNT, news_t, MPI_ANY_SOURCE, EDITOR_FORWARD_TAG, org_comm, 
						&tip_request);
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
					MPI_Irecv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, editors_contact, &ping_request);
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
				MPI_Iprobe(MPI_ANY_SOURCE, PING_TAG, editors_contact, &ping_flag, &ping_status);

				if (!ping_flag) {
					break;
				}
				else {
					MPI_Recv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, editors_contact, MPI_STATUS_IGNORE);
					ping_flag = 0;
					collective_count ++;
					incomplete_ping = 0;
				}
			}

//			MPI_Gather(&queue_length, 1, MPI_INT, gather_recvcounts, 1, MPI_INT, my_rank, editors_contact);
			gather_recvdispls[0] = 0;
			for (int i = 1; i < num_editors; i++)
				gather_recvdispls[i] = gather_recvdispls[i-1] + gather_recvcounts[i-1];

			MPI_Gatherv(queue, queue_length, news_t, gather_queue, gather_recvcounts, gather_recvdispls, news_t, my_rank, editors_contact);
			//Need to online sort here! 
			//qsort(gather_queue, sum(gather_recvcounts), sizeof(newsitem), compare_newsitems);
			livetelecast_queue_length = 0;
			for (int i = 0; i < sumArray(gather_recvcounts, num_editors); i++)
				insert(livetelecast_queue, &gather_queue[i], &livetelecast_queue_length);

			for (int i = 0; i < livetelecast_queue_length; i++)
				printNews(&livetelecast_queue[i]);

			meeting_count++;			

		} else {
			//normal reporter
			ping_count = 0;
			bcast_flag = 0;
			MPI_Ibcast(&bcast_buffer, 1, MPI_INT, (meeting_count % num_editors), editors_contact, &bcast_request);
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
						//insert(queue, &tip_buffer, queue_length);
						int num_messages;
						MPI_Get_count(&tip_status, MPI_INT, &num_messages);
						for (i = 0; i < num_messages; i++)
							insert(queue, &tip_buffer[i], &queue_length);

						printf("Received information in editor %d\n", world_rank);
						ping_count++;
						incomplete_tip = 0;
					}
				}

				if (!incomplete_tip) {
					tip_flag = 0;
					MPI_Irecv(&tip_buffer, 3*MAX_REPORTER_COLLECTIVE_COUNT, news_t, MPI_ANY_SOURCE, EDITOR_FORWARD_TAG, org_comm, 
						&tip_request);
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
					MPI_Isend(&ping_buffer, 1, MPI_INT, (meeting_count % num_editors), PING_TAG, editors_contact, &ping_request);
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
				MPI_Send(&ping_buffer, 1, MPI_INT, (meeting_count % num_editors), PING_TAG, editors_contact);
				ping_count --;
			}

			MPI_Gather(&queue_length, 1, MPI_INT, NULL, 1, MPI_INT, (meeting_count % num_editors), editors_contact);
			MPI_Gatherv(queue, queue_length, news_t, NULL, NULL, NULL, news_t, (meeting_count % num_editors), editors_contact);
			meeting_count ++;
		}
	}
}