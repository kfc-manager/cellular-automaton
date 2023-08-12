#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "random.h"
#include "md5tool.h"

// start: code taken from Peter Sanders, Ingo Boesnach sequential solution
#define XSIZE 1024

typedef char State;
typedef State Line[XSIZE + 2];

#define randInt(n) ((int)(nextRandomLEcuyer() * n))

static State anneal[10] = {0, 0, 0, 0, 1, 0, 1, 1, 1, 1};

#define transition(a, x, y) \
    (anneal[(a)[(y)-1][(x)-1] + (a)[(y)][(x)-1] + (a)[(y)+1][(x)-1] +\
            (a)[(y)-1][(x)  ] + (a)[(y)][(x)  ] + (a)[(y)+1][(x)  ] +\
            (a)[(y)-1][(x)+1] + (a)[(y)][(x)+1] + (a)[(y)+1][(x)+1]])
// end: code taken from Peter Sanders, Ingo Boesnach sequential solution

static int *workload;
static int *start;
static int *end;

static void initWorkload(int size, int lines) {
    workload = malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++) {
        workload[i] = 0;
    }
    for (int i = 0; i < lines; i++) {
        workload[i % size]++;
    }
}

static void initStart(int size) {
    start = malloc(sizeof(int) * size);
    start[0] = 1;
    for (int i = 1; i < size; i++) {
        start[i] = start[i-1] + workload[i-1];
    }
}

static void initEnd(int size) {
    end = malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++) {
        end[i] = start[i] + workload[i] - 1;
    }
}

static void initConfig(Line *buf, int start, int end, int lines) {  
    int x, y;
    int index = 0;

    initRandomLEcuyer(424243);
    for (y = 1;  y <= lines;  y++) {
        if (y >= start && y <= end) {
            index++;
            for (x = 1; x <= XSIZE; x++) {
                buf[index][x] = randInt(100) >= 50;
            }
            continue;
        } else if (y > end) break;
        for (x = 1;  x <= XSIZE;  x++) {
            randInt(100);
        }
    }
}

static void simulate(Line *buf, Line *next, int lines, int rank, int size) {

    for (int y = 0; y <= lines+1; y++) {
        buf[y][0] = buf[y][XSIZE];
        buf[y][XSIZE+1] = buf[y][1];
    }

    int src, dest;
    MPI_Status status;

    if (rank == 0) {
        dest = size - 1;
    } else {
        dest = rank - 1;
    }
    MPI_Send(buf[1], (XSIZE + 2), MPI_CHAR, dest, 1, MPI_COMM_WORLD);
    if (size - 1 == rank) {
        dest = 0;
    } else {
        dest = rank + 1;
    }
    MPI_Send(buf[lines - 2], (XSIZE + 2), MPI_CHAR, dest, 1, MPI_COMM_WORLD);
    if (size - 1 == rank) {
        src = 0;
    } else {
        src = rank + 1;
    }
    MPI_Recv(buf[lines - 1], (XSIZE + 2), MPI_CHAR, src, 1, MPI_COMM_WORLD, &status);
    if (rank == 0) {
        src = size - 1;
    } else {
        src = rank - 1;
    }
    MPI_Recv(buf[0], (XSIZE + 2), MPI_CHAR, src, 1, MPI_COMM_WORLD, &status);

    for (int y = 1; y < lines - 1; y++) {
        for (int x = 1; x <= XSIZE; x++) {
            next[y][x] = transition(buf, x, y);
        }
    }

    for (int y = 0; y < lines; y++) {
        for (int x = 0; x < (XSIZE+2); x++) {
            buf[y][x] = next[y][x];
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        perror("only two arguments\
         (number of lines and number of itertions) required and allowed");
        return -1;
    }

    int lines = atoi(argv[1]);
    int its = atoi(argv[2]);

    MPI_Init(&argc, &argv);
    MPI_Status status;
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    initWorkload(size, lines);
    initStart(size);
    initEnd(size);

    Line *buf = malloc((workload[rank] + 2) * sizeof(Line));
    Line *next = malloc((workload[rank] + 2) * sizeof(Line));
    initConfig(buf, start[rank], end[rank], lines);

    for (int i = 0; i < its; i++) {
        simulate(buf, next, (workload[rank] + 2), rank, size);
    }

    if (rank < 1) {
        // MASTER
        Line *ca = malloc((lines + 2) * sizeof(Line));
        for (int i = 0; i < size; i++) {
            for (int y = 0; y < workload[i]; y++) {
                for (int x = 1; x < (XSIZE + 1); x++) {
                    ca[y + start[i]][x] = buf[y + 1][x];
                }
            }
            if (i == size - 1) break;
            MPI_Recv(buf, (workload[i+1] + 2) * (XSIZE + 2), MPI_CHAR,\
                i+1, 0, MPI_COMM_WORLD, &status);
        }
        char *hash = getMD5DigestStr(ca[1], sizeof(Line) * (lines));
        printf("hash: %s\n", hash);
        free(hash);
        free(ca);
    } else {
        // WORKER
        MPI_Send(buf, (workload[rank] + 2) * (XSIZE + 2), MPI_CHAR,\
            0, 0, MPI_COMM_WORLD);
    }

    free(buf);
    free(next);
    MPI_Finalize();
    return 0;
}