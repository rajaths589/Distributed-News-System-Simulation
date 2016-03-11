#include <mpi.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int informant_rank;
int num_reporters;
int max_news_items;
int NEWS_TAG;
int PING_TAG;
int max_session_duration;
int max_collective_count;

void informant(MPI_Comm comm);
void reporter(MPI_Comm comm_world, MPI_Comm rep_comm);

int main() {
	MPI_Init(NULL, NULL);

	int world_rank, world_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	MPI_Comm rep_comm;	

	informant_rank = 0;
	num_reporters = world_size - 1;
	max_news_items = 1000;
	NEWS_TAG = 3;
	PING_TAG = 6;
	max_session_duration = 10;
	max_collective_count = 10;

	if (world_rank == 0) {
		MPI_Comm_split(MPI_COMM_WORLD, 1, 0, &rep_comm);
		informant(MPI_COMM_WORLD);
	} else {
		MPI_Comm_split(MPI_COMM_WORLD, 0, world_rank, &rep_comm);
		reporter(MPI_COMM_WORLD, rep_comm);
	}

	MPI_Finalize();
	return 0;	
}

void informant(MPI_Comm comm) {
	srand(time(NULL));

	int count = 0;

	int rData;
	int rReporter;

	while (count < max_news_items) {
		rData = rand() % 10;
		rReporter = (rand() % num_reporters) + 1;

		MPI_Send(&rData, 1, MPI_INT, rReporter, NEWS_TAG, comm);

		sleep(rand()%3);
	}
}

void reporter(MPI_Comm comm_world, MPI_Comm rep_comm) {
	int my_rank;
	MPI_Comm_rank(rep_comm, &my_rank);

	int meeting_count = 0;
	double session_start;
	int collective_count;
	int ping_count;

	int incomplete_tip, incomplete_bcast, incomplete_ping;
	int tip_flag, bcast_flag, ping_flag;

	MPI_Request tip_request, ping_request, bcast_request;
	MPI_Status tip_status, ping_status, bcast_status;
	int bcast_buffer, tip_buffer, ping_buffer;
	int gather_buffer[num_reporters];

	incomplete_tip = 0;
	incomplete_bcast = 0;
	tip_flag = 0;
	bcast_flag = 0;

	while (1) {

		incomplete_bcast = 0;
		collective_count = 0;
		incomplete_ping = 0;

		if ((meeting_count % num_reporters) == my_rank) {
			//lead reporter

			session_start = MPI_Wtime();
			while (1) {
				if ((MPI_Wtime() - session_start) > max_session_duration) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, rep_comm, &bcast_request);
					MPI_Wait(&bcast_request, &bcast_status);
					printf("Broadcast complete1! Collective count: %d\n", collective_count);
					break;
				}

				if (collective_count > max_collective_count) {
					bcast_buffer = 1;

					// Blocking and nonblocking collective operations do not match.
					MPI_Ibcast(&bcast_buffer, 1, MPI_INT, my_rank, rep_comm, &bcast_request);
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
					MPI_Irecv(&tip_buffer, 1, MPI_INT, informant_rank, NEWS_TAG, comm_world, &tip_request);
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
					MPI_Irecv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, rep_comm, &ping_request);
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
				MPI_Iprobe(MPI_ANY_SOURCE, PING_TAG, rep_comm, &ping_flag, &ping_status);

				if (!ping_flag) {
					break;
				}
				else {
					MPI_Recv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, PING_TAG, rep_comm, MPI_STATUS_IGNORE);
					ping_flag = 0;
					collective_count ++;
					incomplete_ping = 0;
				}
			}

			MPI_Gather(&bcast_buffer, 1, MPI_INT, gather_buffer, 1, MPI_INT, my_rank, rep_comm);
			meeting_count++;
			printf("Gather in process : %d Collective count: %d\n\n", my_rank, collective_count);

		} else {
			//normal reporter
			ping_count = 0;
			bcast_flag = 0;
			MPI_Ibcast(&bcast_buffer, 1, MPI_INT, (meeting_count % num_reporters), rep_comm, &bcast_request);
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
					MPI_Irecv(&tip_buffer, 1, MPI_INT, informant_rank, NEWS_TAG, comm_world, &tip_request);
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
					MPI_Isend(&ping_buffer, 1, MPI_INT, (meeting_count % num_reporters), PING_TAG, rep_comm, &ping_request);
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
				MPI_Send(&ping_buffer, 1, MPI_INT, (meeting_count % num_reporters), PING_TAG, rep_comm);
				ping_count --;
			}

			MPI_Gather(&bcast_buffer, 1, MPI_INT, NULL, 1, MPI_INT, (meeting_count % num_reporters), rep_comm);
			meeting_count ++;
		}
	}
}