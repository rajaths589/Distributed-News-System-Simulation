CC = mpicc
CFLAGS = -g -fopenmp

all : news

news : main.o editor.o reporter.o informant.o abcdhelper.o
	$(CC) $(CFLAGS) main.o editor.o reporter.o informant.o abcdhelper.o

main.o : abcdnews_main.c abcdnews.h
	$(CC) $(CFLAGS) -c abcdnews_main.c -o main.o

editor.o : editor.c abcdnews.h
	$(CC) $(CFLAGS) -c editor.c -o editor.o

reporter.o : reporter.c abcdnews.h
	$(CC) $(CFLAGS) -c reporter.c -o reporter.o

informant.o : informant.c abcdnews.h
	$(CC) $(CFLAGS) -c informant.c -o informant.o

abcdhelper.o : abcdhelper.c abcdnews.h
	$(CC) $(CFLAGS) -c abcdhelper.c -o abcdhelper.o

clean :
	rm -rf *.o