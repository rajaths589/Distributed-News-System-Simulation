#include <stdlib.h>
#include <time.h>

#include <mpi.h>
#include <omp.h>

#include "abcdnews.h"

#define MAX_NEWS_PER_EVENT 20

void informant(MPI_Comm news_communicator, int* reporters_area_info, int num_areas, MPI_Datatype news_t) {
	srand(time(NULL));

	int num_reporters;
	MPI_Comm_size(news_communicator, &num_reporters);

	// informant is one of the process in the communicator
	num_reporters--;
	int *num_reporters_per_area = (int*) calloc(num_areas, sizeof(int));
	int *area_insert_index = (int*) calloc(num_areas, sizeof(int));
	int **reporters_in_area = (int**) calloc(num_areas, sizeof(int*));
	for (int i = 0; i < num_reporters; i++)
		num_reporters_per_area[reporters_area_info[i]]++;

	for (i = 0; i < num_areas; i++)
		reporters_in_area[i] = (int*) calloc(num_reporters_per_area[i], sizeof(int));

	memcpy(area_insert_index, num_reporters_per_area, num_areas*sizeof(int));

	for (i = 0; i < num_reporters; i++) {
		reporters_in_area[reporters_area_info[i]][area_insert_index[reporters_area_info[i]] - 1] = i;
		area_insert_index[reporters_area_info[i]]--;
	}
	free(area_insert_index);

	unsigned int current_event = 0;
	int news_per_current_event = 0;
	unsigned int current_area;

	#pragma omp parallel num_threads(4)
	{
		int dest;
		while(1) {
			#pragma omp critical
			{
				if (news_per_current_event == 0) {
					current_event++;
					news_per_current_event = rand() % 20;
					current_area = rand() % num_areas;
				} else {
					dest = rand() % num_reporters_per_area[current_area];
					news_per_current_event --;
				}
			}

			newsitem news;
			news.event = current_event;
			news.area = current_area;
			news.time_stamp = MPI_Wtime();

			MPI_Send(&news, 1, news_t, dest+1, NEWS_TAG, news_communicator);
		}
	}
}