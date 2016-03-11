#include <mpi.h>
#include "abcdnews.h"
#include <stdlib.h>

void editor(MPI_Comm editors_contact, MPI_Comm org_comm, MPI_Datatype news_t) {
	newsitem *tip_buffer = NULL;
	MPI_Status r_status;
	tip_buffer = createQueue(3*MAX_REPORTER_COLLECTIVE_COUNT);
	int num_messages = 0;

	while (1) {
		MPI_Recv(tip_buffer, 3*MAX_REPORTER_COLLECTIVE_COUNT, news_t, MPI_ANY_SOURCE, EDITOR_FORWARD_TAG, org_comm, &r_status);		
		MPI_Get_count(&r_status, news_t, &num_messages);
		for (int i = 0; i < num_messages; i++)
			printNews(&tip_buffer[i]);
		
//		printf("\n");
	}
}
