#include <stdlib.h>

#define MAX_REPORTER_COLLECTIVE_COUNT 20
#define MAX_REPORTER_SESSION_DURATION 10
#define MAX_EDITOR_COLLECTIVE_COUNT 5
#define MAX_EDITOR_SESSION_DURATION 5

extern int max_num_reporters_per_editor;

struct newsitem {
    unsigned int event;
    int area;
    double time_stamp;
};

typedef struct newsitem newsitem;

int readConfig(const char* configfile, int*** colorMatrix, int* num_editors, int* num_partitions, int world_size);
void informant(MPI_Comm, MPI_Comm, int, MPI_Datatype);
void reporter(MPI_Comm, MPI_Comm, MPI_Comm, MPI_Datatype);
void editor(MPI_Comm, MPI_Comm, MPI_Datatype);

newsitem* createQueue(int size)
{
	newsitem* ret_queue = (newsitem *) calloc(size, sizeof(newsitem));
}

void insert(newsitem *queue, newsitem * news_to_insert, int *queue_len)
{
	int i;

	//TODO: To ping or not to ping? 
	for(i=0; i<*queue_len; i++) 
	{
		if( (queue[i].event == news_to_insert->event) && (queue[i].area > news_to_insert->area) ){
			if( queue[i].time_stamp < news_to_insert->time_stamp ) queue[i].time_stamp < news_to_insert->time_stamp;
			return;
		}
	}

	for(i= *queue_len; i>=0; i--) {

		//Should Area be checked
		if(queue[i].event > news_to_insert->event) {

			queue[i+1].event = queue[i].event;
			queue[i+1].area = queue[i].area;
			queue[i+1].time_stamp = queue[i].time_stamp;

		} 
		else {
			queue[i+1].area = news_to_insert->area;
			queue[i+1].event = news_to_insert->event;
			queue[i+1].time_stamp = news_to_insert->time_stamp;
		}
	}

	(*queue_len)++;

}

int compare_newsitems(const void* newsA, const void* newsB) {
	newsitem *nA, *nB;
	nA = (newsitem*) newsA;
	nB = (newsitem*) newsB;

	//descending order -- easier to select unique elements	
	if (nA->event == nB->event) {
		if (nA->time_stamp == nB->time_stamp)	return 0;
		else if (nA->time_stamp > nB->time_stamp) return -1;
		else return 1;
	}
	else if (nA->event > nB->event) return -1;
	else return 1;
}

int sumArray(int* arr, int len) {
	int sum = 0;

	for (int i = 0; i < len; i++)
		sum += arr[i];

	return sum;
}