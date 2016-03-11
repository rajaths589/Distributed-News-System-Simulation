#include <mpi.h>
#include "abcdnews.h"
#include <stdlib.h>

void editor(MPI_Comm editors_comm, MPI_Comm org_comm, MPI_Datatype dt_news) {
	int my_rank, num_editors;
	MPI_Comm_rank(editors_contact, &my_rank);
	MPI_Comm_size(editors_contact, &num_editors);

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
	int gather_buffer[num_editors];


	incomplete_tip = 0;
	incomplete_bcast = 0;
	tip_flag = 0;
	bcast_flag = 0;

	while (1) {

		incomplete_bcast = 0;
		collective_count = 0;
		incomplete_ping = 0;

		if ((meeting_count % num_editors) == my_rank) {
			//lead reporter

			session_start = MPI_Wtime();
			while (1) {
				if ((MPI_Wtime() - session_start) > max_session_duration) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, editor_comm, &bcast_request);
					MPI_Wait(&bcast_request, &bcast_status);
					printf("Broadcast complete1! Collective count: %d\n", collective_count);
					break;
				}

				if (collective_count > max_collective_count) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, editor_comm, &bcast_request);
					MPI_Wait(&bcast_request, &bcast_status);
					printf("Broadcast complete2! Collective count: %d\n", collective_count);
					break;
				}

				if (incomplete_tip) {
					MPI_Test(&tip_request, &tip_flag, &tip_status);

					if (tip_flag) {
						printf("Received information %d in process %d\n", tip_buffer, my_rank);
						collective_count++;
						incomplete_tip = 0;
					}
				}

				if (!incomplete_tip) {
					tip_flag = 0;
					MPI_Irecv(&tip_buffer, 1, news_t, informant_rank, NEWS_TAG, org_comm, &tip_request);
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
					MPI_Irecv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, editor_comm, &ping_request);
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
				MPI_Iprobe(MPI_ANY_SOURCE, PING_TAG, editor_comm, &ping_flag, &ping_status);

				if (!ping_flag) {
					break;
				}
				else {
					MPI_Recv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, editor_comm, MPI_STATUS_IGNORE);
					ping_flag = 0;
					collective_count ++;
					incomplete_ping = 0;
				}
			}

			MPI_Gather(&bcast_buffer, 1, MPI_INT, gather_buffer, 1, MPI_INT, my_rank, editor_comm);
			meeting_count++;
			printf("Gather in process : %d Collective count: %d\n\n", my_rank, collective_count);

		} else {
			//normal reporter
			ping_count = 0;
			bcast_flag = 0;
			MPI_Ibcast(&bcast_buffer, 1, MPI_INT, (meeting_count % num_editors), editor_comm, &bcast_request);
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
						printf("Received information %d in process %d\n", tip_buffer, my_rank);
						ping_count++;
						incomplete_tip = 0;
					}
				}

				if (!incomplete_tip) {
					tip_flag = 0;
					MPI_Irecv(&tip_buffer, 1, news_t, informant_rank, NEWS_TAG, org_comm, &tip_request);
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
					MPI_Isend(&ping_buffer, 1, MPI_INT, (meeting_count % num_editors), PING_TAG, editor_comm, &ping_request);
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
				MPI_Send(&ping_buffer, 1, MPI_INT, (meeting_count % num_editors), PING_TAG, editor_comm);
				ping_count --;
			}

			MPI_Gather(&bcast_buffer, 1, MPI_INT, NULL, 1, MPI_INT, (meeting_count % num_editors), editor_comm);
			meeting_count ++;
		}
	}
}