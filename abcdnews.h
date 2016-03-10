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