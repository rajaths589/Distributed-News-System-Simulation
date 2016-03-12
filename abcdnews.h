#ifndef DEF_ABCD_NEWS_HEADER
#define DEF_ABCD_NEWS_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#define MAX_REPORTER_COLLECTIVE_COUNT 20
#define MAX_REPORTER_SESSION_DURATION 10
#define MAX_EDITOR_COLLECTIVE_COUNT 15
#define MAX_EDITOR_SESSION_DURATION 25

#define NEWS_TAG 7
#define PING_TAG 11
#define EDITOR_FORWARD_TAG 13

extern int informant_rank;
extern int editor_rank;
extern double stime;

struct newsitem {
    unsigned int event;
    int area;
    double time_stamp;
};

typedef struct newsitem newsitem;

int readConfig(const char* configfile, int*** colorMatrix, int* num_editors, int* num_partitions, int world_size);
void informant(MPI_Comm, int*, int, MPI_Datatype);
void reporter(MPI_Comm, MPI_Comm, MPI_Comm, MPI_Datatype);
void editor(MPI_Comm, MPI_Comm, MPI_Datatype);
void printNews(newsitem* nitem);
void printNewsRank(int rank, newsitem* nitem);
int sumArray(int* arr, int len);
int compare_newsitems(const void* newsA, const void* newsB);
void insert(newsitem *queue, newsitem * news_to_insert, int *queue_len);
newsitem* createQueue(int size);

#endif
