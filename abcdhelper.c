#include "abcdnews.h"

newsitem* createQueue(int size)
{
	newsitem* ret_queue = (newsitem *) calloc(size, sizeof(newsitem));
	return ret_queue;
}

void insert(newsitem *queue, newsitem * news_to_insert, int *queue_len)
{
	int i;

	if ((*queue_len) == 0)
	{
		queue[0].area = news_to_insert->area;
		queue[0].event = news_to_insert->event;
		queue[0].time_stamp = news_to_insert->time_stamp;
		(*queue_len)++;
		return;
	}

	//TODO: To ping or not to ping? 
	for(i=0; i<*queue_len; i++) 
	{
		if ( (queue[i].event == news_to_insert->event)) {
			if ( queue[i].time_stamp < news_to_insert->time_stamp ) queue[i].time_stamp = news_to_insert->time_stamp;
			return;
		}
	}

	for(i= (*queue_len)-1; i>=0; i--) {

		//Should Area be checked
		if(queue[i].event > news_to_insert->event) {

			queue[i+1].event = queue[i].event;
			queue[i+1].area = queue[i].area;
			queue[i+1].time_stamp = queue[i].time_stamp;

		} 
		else {
			break;
		}
	}
	queue[i+1].area = news_to_insert->area;
	queue[i+1].event = news_to_insert->event;
	queue[i+1].time_stamp = news_to_insert->time_stamp;

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

void printNews(newsitem* nitem) {
	printf("CurrentTime: %lf Time : %lf Event : %u Area : %u\n", MPI_Wtime()-stime, nitem->time_stamp, nitem->event, nitem->area);
}

void printNewsRank(int rank, newsitem* nitem) {
	printf("Rank : %d Time : %lf Event : %u Area : %u\n", rank, nitem->time_stamp, nitem->event, nitem->area);
}
