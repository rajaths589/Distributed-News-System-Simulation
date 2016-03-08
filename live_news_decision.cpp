#include <mpi.h>

#define TIPPER_RANK 0
#define REPORTERS_PING_TAG 2
#define NEWSTIP_TAG 6

int EVENT_QUEUE_LENGTH = 5;
int MEETING_COLLECTIVE_COUNT = 20;
double MAX_MEETING_SEPERATION = 25;		// expressed in terms of number of MPI_Ticks

// struct newsitem {
//     uint32_t event;
//     enum areas area;
//     struct timeval time;
// };


typedef int newsitem;


//typedef struct newsitem newsitem;

//decide parameters on-the-go
//my_rank in reporters communicator

void ReporterNode(int my_rank) {
	newsitem queue[EVENT_QUEUE_LENGTH];
	newsitem gather_queue[EVENT_QUEUE_LENGTH * NUM_REPORTERS_IN_COMM];

	int gather_count[NUM_REPORTERS_IN_COMM];
	int gather_displ[NUM_REPORTERS_IN_COMM];

	int news_count = 0;

	newsitem tip_buffer[1];
	int ping_buffer;

	int collective_count = 0;
	double session_starttime;
	double session_currenttime;

	int ping_flag, tip_flag;
	MPI_Status ping_status, tip_status, bcast_status;
	MPI_Request ping_request, tip_request;

	MPI_Request bcast_meeting_request;
	int bcast_ping = 1;
	int bcast_recv_flag;
	
	// Flags maintatining state informatioN
	int meeting_count = 0;
	int incomplete_tip_receive = 0;
	int incomplete_ping_receive = 0;
	int ping_count = 0;


	while (1) {
		if ((meeting_count % NUM_REPORTERS_IN_COMM) == my_rank) {
			//lead reporter
			collective_count = 0;
			session_starttime = MPI_Wtime();			
			incomplete_ping_receive = 0;
			tip_flag = 0;
			ping_flag = 0;


			while (1) {
				session_currenttime = MPI_Wtime();

				if (session_currenttime - session_starttime > MAX_MEETING_SEPERATION)
				{
					//call meeting
					MPI_Bcast(&bcast_ping, 1, MPI_INT, 0, MY_RANK, REP_COMM);
					break;
				}

				if (collective_count > NEWS_THRESHOLD) 
				{
					//call meeting
					MPI_Bcast(&bcast_ping, 1, MPI_INT, 0, MY_RANK, REP_COMM);
					break;
				}

				if (incomplete_ping_receive) {
					MPI_Test(&ping_request, &ping_flag, &ping_status);

					if (ping_flag) {
						collective_count++;
						ping_flag = 0;
						
						incomplete_ping_receive = 0;
					}
				}

				if (incomplete_tip_receive) {
					MPI_Test(&tip_request, &tip_flag, &tip_status);

					if (tip_flag) {
//						memcpy(&queue[queue_head], tip_buffer, sizeof(newsitem));
						
						//online insertion sort
						for(i=news_count-1; i>=0; i--)
						{
							if(queue[i] > tip_buffer)
							{
								queue[i+1] = queue[i];
							}
							else 
							{
								queue[i] = tip_buffer;
								break;
							}
						}


						tip_flag = 0;

						news_count++;
						collective_count++;

						incomplete_tip_receive = 0;
					}
				}

				if (!incomplete_ping_receive) {
					MPI_Irecv(&ping_buffer, 1, MPI_INT, MPI_ANY_SOURCE, REPORTERS_PING_TAG, REP_COMM, &ping_request);
					incomplete_ping_receive = 1;
				}

				if (!incomplete_tip_receive) {
					MPI_Irecv(tip_buffer, 1, MPI_INT, TIPPER_RANK, NEWSTIP_TAG, TIP_COMM, &tip_request);				
					incomplete_tip_receive = 1;
				}
			}

			//cancel ping request
			if (incomplete_ping_receive) {
				MPI_Cancel(&ping_request);
				incomplete_ping_receive = 0;
			}

			MPI_Gatherv(queue, news_count, MPI_INT, gather_queue, gather_count, gather_displ, MPI_INT, MY_RANK, REP_COMM);
			//sort the list
			//send to editor
			meeting_count++;

		} else {
			//reporter
			news_count = 0;
			bcast_recv_flag = 0;
			ping_count = 0;

			MPI_Ibcast(&bcast_ping, 1, MPI_INT, 0, LEAD_REP, REP_COMM, &bcast_meeting_request);

			while(1){

				MPI_Test(&bcast_meeting_request, &bcast_recv_flag, &bcast_status);

				if(bcast_recv_flag) break;

				if(incomplete_ping_receive)
				{
					MPI_Test(&ping_request, &ping_flag, &ping_status);
					if(ping_flag)
					{
						incomplete_ping_receive = 0;
						ping_count--;

						if (ping_count > 0) {
							MPI_Isend(&ping_buffer, 1, MPI_INT, LEAD_REP, REPORTERS_PING_TAG, REP_COMM, &ping_request);
							incomplete_ping_receive = 1;	
						}
					}
				}

				if(!incomplete_tip_receive) {
					MPI_Irecv(tip_buffer, 1, MPI_INT, TIPPER_RANK, NEWSTIP_TAG, TIP_COMM, &tip_request);
					incomplete_tip_receive = 1;
				}

				if(incomplete_tip_receive) 
				{
					MPI_Test(&tip_request, &tip_flag, &tip_status);

					if (tip_flag) {
						//memcpy(&queue[queue_head], tip_buffer, sizeof(newsitem));
						
						//online insertion sort
						for(i=news_count-1; i>=0; i--)
						{
							if(queue[i] > tip_buffer)
							{
								queue[i+1] = queue[i];
							}
							else 
							{
								queue[i] = tip_buffer;
								break;
							}
						}

						ping_count++;
						tip_flag = 0;

						news_count ++;

						incomplete_tip_receive = 0;


						MPI_Test(&bcast_meeting_request, &bcast_recv_flag, &bcast_status);

						if(bcast_recv_flag) {
							break;
						}
						else 
						{
							if (ping_count == 0) {
								MPI_Isend(&ping_buffer, 1, MPI_INT, LEAD_REP, REPORTERS_PING_TAG, REP_COMM, &ping_request);
								incomplete_ping_receive = 1;								
							}

						}
					}
				}
			}

			if (incomplete_ping_receive) {
				MPI_Cancel(&ping_request);
				incomplete_ping_receive = 0;
			}

			MPI_Gatherv(queue, news_count, MPI_INT, NULL, NULL, NULL, MPI_INT, LEAD_REP, REP_COMM);
			meeting_count++;
		}
	}
}