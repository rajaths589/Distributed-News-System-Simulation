CC = mpicc
CFLAGS = -g -fopenmp -std=c99

all : news

news_single : main.o single_editor.o reporter.o informant.o abcdhelper.o
	$(CC) $(CFLAGS) main.o single_editor.o reporter.o informant.o abcdhelper.o -o news_single

news : main.o editor.o reporter.o informant.o abcdhelper.o
	$(CC) $(CFLAGS) main.o editor.o reporter.o informant.o abcdhelper.o -o news

main.o : abcdnews_main.c abcdnews.h
	$(CC) $(CFLAGS) -c abcdnews_main.c -o main.o

single_editor.o : single_editor.c abcdnews.h
	$(CC) $(CFLAGS) -c single_editor.c -o single_editor.o

editor.o : editor.c abcdnews.h
	$(CC) $(CFLAGS) -c editor.c -o editor.o

reporter.o : reporter.c abcdnews.h
	$(CC) $(CFLAGS) -c reporter.c -o reporter.o

informant.o : informant.c abcdnews.h
	$(CC) $(CFLAGS) -c informant.c -o informant.o

abcdhelper.o : abcdhelper.c abcdnews.h
	$(CC) $(CFLAGS) -c abcdhelper.c -o abcdhelper.o

clean :
	rm -rf *.o news news_single
