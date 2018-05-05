CC=mpicc
CFLAGS = -O2 -std=c99 -g -Wall -I /usr/X11R6/include
LDLIBS = -L /usr/X11R6/lib -lX11

ALL=gen_lab_mpi

#gen_lab_mpi: gen_lab_mpi.c
#	mpicc $(CFLAGS) gen_lab_mpi.c -o gen_lab_mpi -std=c99

all: $(ALL)

$(ALL): graph.o

clean:
	/bin/rm -f $(ALL) *.o laby.lab
