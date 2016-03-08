#include <mpi.h>

#define TIPPER_RANK 0
#define REPORTERS_PING_TAG 2
#define REPORTERS_MEETING_CRITERION 20

// void masterNode(MPI_Comm reporters, MPI_Comm tippers, MPI_omm editors) {
// 	bool inc_ping;
// 	int data;

// 	MPI_Request tipper_rec, reporter_ping_rec;

// 	bool meeting_convened = false;
// 	int count = 0;
// 	bool data_received, ping_received;

// 	data_received = false;
// 	ping_received = false;

// 	MPI_Irecv(&data, 1, MPI_INT, TIPPER_RANK, MPI_ANY_TAG, tippers, &tipper_rec);
// 	MPI_Irecv(&inc_ping, 1, MPI_BOOL, MPI_ANY_SOURCE, REPORTERS_PING_TAG, reporters, &reporter_ping_rec);

// 	while (!meeting_convened) {

// 		while (!data_received && !ping_received) {
// 			MPI_Iprobe(TIPPER_RANK, MPI_ANY_TAG, tippers, data_received, )
// 		}

// 	}
// }

void leadReporter(MPI_Comm reporters, MPI_Comm tippers, MPI_comm editors) {
	int increment_ping;
	int data;

	MPI_Status tip_status, ping_status;
	MPI_Request tip_request, ping_request;

	int tip_recv, ping_recv;
	int tip_recv_post, ping_recv_post;

	int count = 0;

	int personal_count = 0;
	//assume array of a very large size

	tip_recv_post = 1;
	ping_recv_post = 1;

	tip_recv = 0;
	ping_recv = 0;

	while (1) {
		if (tip_recv_post) {
			MPI_Irecv(&data, 1, MPI_INT, TIPPER_RANK, MPI_ANY_TAG, tippers, &tip_request);
			tip_recv_post = 0;
			tip_recv = 0;
		}

		if (ping_recv_post) {
			MPI_Irecv(&increment_ping, 1, MPI_INT, MPI_ANY_SOURCE, REPORTERS_PING_TAG, reporters, &ping_request);
			ping_recv_post = 0;
			ping_recv = 0;
		}

		MPI_Iprobe(TIPPER_RANK, MPI_ANY_TAG, tippers, &tip_recv, &tip_status);
		MPI_Iprobe(MPI_ANY_SOURCE, REPORTERS_PING_TAG, reporters, &ping_recv, &ping_status);

		if (tip_recv) {
			tip_recv_post = 1;
			array[personal_count] = data;
			personal_count ++;			 
		}

		if (ping_recv) {
			count ++;
			ping_recv_post = 1;

			if (count == REPORTERS_MEETING_CRITERION) {
				break;
			}
		}
	}

	if (!tip_recv) {
		MPI_probe(TIPPER_RANK, MPI_ANY_TAG, tippers, &tip_status);
		array[personal_count] = data;
		personal_count ++;
	}

	//call a meeting!
	
}