/**
 *  \file dispatcher.c (implementation file)
 *
 *  \brief Problem name: Problem 2.
 *
 *  \author Francisco Gonçalves and Tiago Lucas - June 2020
 */

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <mpi.h>

#include "probConst.h"
#include "FILEINFO.h"
#include "CONTROLINFO.h"

/* Allusion to internal functions */
static void circularCrossCorrelation(double*, double*, CONTROLINFO*);
static void savePartialResults(CONTROLINFO*);
static void printResults(unsigned int, char**);

/* Globlal variables */
FILEINFO* filesManager;
unsigned int numbFiles;

/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the worker threads and waiting for their termination.
 */
int main (int argc, char *argv[]){
    int nProc, rank, whatToDo;
    double start, finish;
    numbFiles = argc - 1;
    CONTROLINFO ci = {0};
    double* x;
    double* y;

    /* get processing configuration */
    MPI_Init (&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);

    MPI_Barrier (MPI_COMM_WORLD);
    start = MPI_Wtime();

    if (rank == 0) {

        FILE *f;
        filesManager = (FILEINFO*) calloc(numbFiles, sizeof(FILEINFO));
        unsigned int workProc, aux, samples;
        int filePos = 1;
        double* real;

        if(argc < 2) {
            perror("Please insert binary files to be processed as arguments!");
            whatToDo = NOMOREWORK;
            for (int i = 1; i < nProc; i++)
                MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
            MPI_Finalize ();
            exit(EXIT_FAILURE);    
        }

        /* processing the circular cross correlation between two signals */
        while (filePos <= numbFiles) {
            
            if((f = fopen (argv[filePos], "rb")) == NULL){
                perror ("error on file opening for reading");
                whatToDo = NOMOREWORK;
                for (int i = 1; i < nProc; i++)
                    MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
                MPI_Finalize ();
                exit (EXIT_FAILURE);
            }

            fread(&samples, sizeof(int), 1, f);
            if (filePos == 1) {
                x = (double *) malloc(sizeof(double) * samples);
                y = (double *) malloc(sizeof(double) * samples);
                real = (double *) malloc(sizeof(double) * samples);
            } else {
                x = (double *) realloc(x, sizeof(double) * samples);
                y = (double *) realloc(y, sizeof(double) * samples);
                real = (double *) realloc(real, sizeof(double) * samples);
            }
            ci.numbSamples = samples;
            ci.filePosition = filePos - 1;

            fread(x, sizeof(double), samples, f);
            fread(y, sizeof(double), samples, f);
            fread(real, sizeof(double), samples, f);
            filesManager[filePos - 1].result = (double *) malloc(sizeof(double) * samples);
            filesManager[filePos - 1].expected = (double *) malloc(sizeof(double) * samples);
            memcpy(filesManager[filePos - 1].expected, real, sizeof(double) * samples);
            filesManager[filePos - 1].rxyIndex = 0;
            filesManager[filePos - 1].filePosition = filePos - 1;
            filesManager[filePos - 1].numbSamples = samples;

            if (fclose (f) == EOF){ 
                perror ("error on closing file");
                whatToDo = NOMOREWORK;
                for (int i = 1; i < nProc; i++)
                    MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
                MPI_Finalize ();
                exit (EXIT_FAILURE);
            }

            filePos++;
            aux = 0;
            while (aux < samples){ 
                workProc = 1;
                for (int i = 1; i < nProc && aux < samples; i++, workProc++, aux++){
                    whatToDo = WORKTODO;
                    MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
                    MPI_Send (&samples, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
                    ci.rxyIndex = aux;
                    MPI_Send (&ci, sizeof (CONTROLINFO), MPI_BYTE, i, 0, MPI_COMM_WORLD);
                    MPI_Send (x, samples, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
                    MPI_Send (y, samples, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
                }

                for (int i = 1; i < workProc; i++) {
                    MPI_Recv (&ci, sizeof(CONTROLINFO), MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    savePartialResults(&ci);
                }
            }
        }

        free(x);
        free(y);
        free(real);

        /* dismiss worker processes */
        whatToDo = NOMOREWORK;
        for (int i = 1; i < nProc; i++)
            MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);

    } else { /* worker processes */
        unsigned int size_signal, t = 0;

        while (true) {

            MPI_Recv (&whatToDo, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (whatToDo == NOMOREWORK)
                break;
            MPI_Recv (&size_signal, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (size_signal > t) {
                x = (double *) malloc(sizeof(double) * size_signal);
                y = (double *) malloc(sizeof(double) * size_signal);
                t = size_signal;
            }
            MPI_Recv (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv (x, size_signal, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv (y, size_signal, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            circularCrossCorrelation(x, y, &ci);
            MPI_Send (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        }
        free(x);
        free(y);
    }

    MPI_Barrier (MPI_COMM_WORLD);
    if (rank == 0) {
        printf("\nFinal report\n");
        printResults(numbFiles, argv+1);
        finish = MPI_Wtime();
        printf("\nElapsed time = %.6f s\n", finish - start);
    }
    MPI_Finalize ();
    return EXIT_SUCCESS;
}

/**
 *  \brief Perform the comparison between two sets of signals
 *
 *  Operation carried out by the workers.
 *
 */
static void circularCrossCorrelation(double *x, double *y, CONTROLINFO *ci) {

   size_t i;
   size_t n = ci->numbSamples;
   size_t temp = ci->rxyIndex; 

   for(i = 0; i < n; i++){
      ci->result += x[i] * y[(temp+i)%n];
   }
}

/**
 *  \brief Print all the results stored in result data storage.
 *
 *  Operation carried out by the dispatcher.
 *
 */
static void printResults(unsigned int numbFiles, char** filesToProcess){
  
  size_t i, x, numbErrors;

  for (i = 0; i < numbFiles; i++){
    numbErrors = 0;
    for (x = 0; x < filesManager[i].numbSamples; x++){
      if (filesManager[i].expected[x] != filesManager[i].result[x]) {
          numbErrors++;
      }
    }
    if(numbErrors==0)
      printf("File %s was calculated correctly.\n", filesToProcess[i]);
    else 
      printf("File %s had %lu errors in total.\n", filesToProcess[i], numbErrors);
  }
  
  free(filesManager);
}

/**
 *  \brief Get a value from the data transfer region and save it in result data storage.
 *
 *  Operation carried out by the dispatcher.
 *
 *  \param *ci pointer to the shared data structure
 *
 */
static void savePartialResults(CONTROLINFO *ci) {
  filesManager[ci->filePosition].result[ci->rxyIndex] = ci->result;
  filesManager[ci->filePosition].rxyIndex++;
  ci->rxyIndex = filesManager[ci->filePosition].rxyIndex;
  ci->result = 0;
  if(filesManager[ci->filePosition].rxyIndex == filesManager[ci->filePosition].numbSamples){
    ci->filePosition++;
    ci->processing = false;
  }
}