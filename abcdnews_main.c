#include <mpi.h>
#include <stddef.h>
#include "abcdnews.h"
#include <stdio.h>

//process 0 is always the tipper

int informant_rank;
int editor_rank;
double stime;

int main(int argc, char** argv) {
	
	int world_rank, world_size;
	int num_editors, num_partitions, **colorMatrix;

	if (argc != 2) {
		printf("Error in reading command line arguments!\n");
		return 0;
	}

	MPI_Init(NULL, NULL);
	MPI_Barrier(MPI_COMM_WORLD);
	stime = MPI_Wtime();
	
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	if (world_rank == 0)
		fprintf(stderr, "START TIME: %lf\n", MPI_Wtime());	
	
	//News Datatype
	const int dt_num = 3;
	MPI_Datatype dt_type[3] = {MPI_UNSIGNED, MPI_UNSIGNED, MPI_DOUBLE};

	int dt_blocklen[3] = {1,1,1};

	MPI_Datatype dt_news;

	MPI_Aint offset[3];

	offset[0] = offsetof(newsitem, event);
	offset[1] = offsetof(newsitem, area);
	offset[2] = offsetof(newsitem, time_stamp);

	MPI_Type_create_struct(dt_num, dt_blocklen, offset, dt_type, &dt_news);
	MPI_Type_commit(&dt_news);
	

	MPI_Comm editors_comm, org_comm, news_source_comm, editor_partition_comm;

	informant_rank = 0;
	editor_rank = 0;

	if (!readConfig(argv[1], &colorMatrix, &num_editors, &num_partitions, world_size)) {
		printf("Error in reading configuration file! \n");
		return 0;
	}

	if (world_rank == 0) MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, world_rank, &editors_comm);
	else if (world_rank <= num_editors) MPI_Comm_split(MPI_COMM_WORLD, 1, world_rank, &editors_comm);
	else MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, world_rank, &editors_comm);

	int* org_color = (int*) malloc(sizeof(int)*world_size);
	org_color[0] = MPI_UNDEFINED;

	int num_reporters_per_editor;
	int current_reporter_index = num_editors + 1;
	int i, j;

	for (i = 0; i < num_editors; i++) {
		num_reporters_per_editor = 0;
		org_color[i+1] = i;

		for (j = 0; j < num_partitions; j++)
			num_reporters_per_editor += colorMatrix[j][i];

		for (j = 0; j < num_reporters_per_editor; j++) {
			org_color[current_reporter_index] = i;
			current_reporter_index++;
		}
	}
	MPI_Comm_split(MPI_COMM_WORLD, org_color[world_rank], world_rank, &org_comm);

	org_color[0] = 0;
	for (i = 1; i <= num_editors; i++)
		org_color[i] = MPI_UNDEFINED;
	for(i = num_editors + 1; i < world_size; i++)
		org_color[i] = 0;
	MPI_Comm_split(MPI_COMM_WORLD, org_color[world_rank], world_rank, &news_source_comm);

	int *reporters_area = (int*) malloc((world_size - num_editors - 1) * sizeof(int));
	org_color[0] = MPI_UNDEFINED;
	for (i = 1; i <= num_editors; i++)
		org_color[i] = MPI_UNDEFINED;
	current_reporter_index = num_editors + 1;
	int  current_color = 0;

	for (i = 0; i < num_editors; i++) {
		for (j = 0; j < num_partitions; j++) {
			for (int k = 0; k < colorMatrix[j][i]; k++) {
				org_color[current_reporter_index] = current_color;
				reporters_area[current_reporter_index - num_editors - 1] = j;
				current_reporter_index ++;
			}
			current_color ++;
		}
	}
	MPI_Comm_split(MPI_COMM_WORLD, org_color[world_rank], world_rank, &editor_partition_comm);

	int editor_rank;
	if (world_rank == 0) informant(news_source_comm, reporters_area, num_partitions, dt_news);
	else if (world_rank <= num_editors) editor(editors_comm, org_comm, dt_news);
	else reporter(org_comm, news_source_comm, editor_partition_comm, dt_news);

	MPI_Finalize();
	return 0;
}

int readConfig(const char* configfile, int*** colorMatrix, int* num_editors, int* num_partitions, int world_size) {
	int i, j;

	FILE* config_file = fopen(configfile, "r");

	fscanf(config_file, "%d %d", num_editors, num_partitions);

	*colorMatrix = (int**) malloc(*num_partitions*sizeof(int*));
	for (i = 0; i < *num_partitions; i++)
		(*colorMatrix)[i] = (int*) malloc(*num_editors*sizeof(int));

	int num_reporters = 0;

	for (i = 0; i < *num_partitions; i++)
		for (j = 0; j < *num_editors; j++) {
			fscanf(config_file, "%d", &((*colorMatrix)[i][j]));
			num_reporters += (*colorMatrix)[i][j];
		}

	if (world_size != *num_editors + num_reporters + 1)
	{
		for (i = 0; i < *num_partitions; i++)
			free((*colorMatrix)[i]);

		free(*colorMatrix);
		colorMatrix = NULL;
				
		return 0;
	}

	return 1;
}
