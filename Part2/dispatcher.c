/**
 *  \file dispatcher.c (implementation file)
 *
 *  \brief 
 *
 *  
 *
 *  \author 
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
void circularCrossCorrelation(double*, double*, CONTROLINFO*);
void savePartialResults(CONTROLINFO*);
void printResults();

/* Globlal variables */
FILEINFO* filesManager;
unsigned int numbFiles;

/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the worker threads and waiting for their termination.
 */
int main (int argc, char *argv[]){

  int nProc, rank;
  double t0, t1;
  numbFiles = argc - 1;
  CONTROLINFO ci;
  double* x;
  double* y;

  /* get processing configuration */
  MPI_Init (&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nProc);
  printf("PROCESS %d OUT OF %d INITIATED\n", rank, nProc - 1);

  if (rank == 0) {

    t0 = MPI_Wtime();
    FILE *f;
    filesManager = (FILEINFO*)malloc(sizeof(FILEINFO)*numbFiles);;
    ci = (CONTROLINFO) {0};
    unsigned int whatToDo = WORKTODO, workProc, filePos = 1;
    size_t samples;
    double* real;

    if(argc < 2) {

      perror("Please insert binary files to be processed as arguments!");
      whatToDo = NOMOREWORK;
      for (int i = 1; i < nProc; i++)
        MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
      MPI_Finalize ();
      exit(EXIT_FAILURE);
        
    }
    printf("ENTERING WHILE\n");
    /* processing the circular cross correlation between two signals */
    while (filePos <= numbFiles) {
      workProc = 1;
      for (int i = 1; i < nProc; i++, workProc++){
        if (filePos > numbFiles) {
          break;
        } else if (f == NULL) {
          if((f = fopen (argv[filePos], "rb")) == NULL){
            filePos++;
            perror ("error on file opening for reading");
            whatToDo = NOMOREWORK;
      
            for (int i = 1; i < nProc; i++)
              MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
            MPI_Finalize ();
            exit (EXIT_FAILURE);
          }
        }
        printf("FIRST READ\n");
        fread(&samples, sizeof(int), 1, f);
        printf("ALLOCATION %d SAMPLES FOR x AND y\n", samples);
        x = (double *) malloc(sizeof(double) * samples);
        y = (double *) malloc(sizeof(double) * samples);
        real = (double *) malloc(sizeof(double) * samples);

        ci.numbSamples = samples;
        ci.filePosition = filePos - 1;
        ci.processing = true;
        ci.rxyIndex = 0;

        fread(x, sizeof(double), samples, f);
        fread(y, sizeof(double), samples, f);
        fread(real, sizeof(double), samples, f);
        printf("READ %lu SAMPLES TO x AND TO y!\n", samples);

        printf("SENDING DATA TO WORKER %i\n", i);
        MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
        MPI_Send (&samples, sizeof(unsigned int), MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
        MPI_Send (&ci, sizeof (CONTROLINFO), MPI_BYTE, i, 0, MPI_COMM_WORLD);
        MPI_Send (&x, samples, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        MPI_Send (&y, samples, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        printf("SENT DATA TO WORKER %i\n", i);
        
        if (fclose (f) == EOF){ 
          perror ("error on closing file");
          whatToDo = NOMOREWORK;
          for (int i = 1; i < nProc; i++)
            MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
          MPI_Finalize ();
          exit (EXIT_FAILURE);
        }
        filePos++;

      }

      for (int i = 0; i < workProc; i++) {
        MPI_Recv (&ci, sizeof(CONTROLINFO), MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        savePartialResults(&ci);

        if(filesManager[filePos].read == false){

          double real[samples];
          fread(real, sizeof(double), samples, f);
          filesManager[filePos].read = true;
          filesManager[filePos].rxyIndex = 0;
          filesManager[filePos].filePosition = filePos - 1;
          filesManager[filePos].numbSamples = samples;

          for (size_t t = 0; t < samples; t++)
            filesManager[filePos].expected[t] = real[t];
          }
      }
      
    }

    /* dismiss worker processes */
    whatToDo = NOMOREWORK;
    for (int i = 1; i < nProc; i++)
      MPI_Send (&whatToDo, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);

  } else { /* worker processes */
    unsigned int whatToDo, size_signal;

    while (true) {
      ci = (CONTROLINFO) {0};
      MPI_Recv (&whatToDo, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("PROCESS %d RECEIVED whatToDo: %d\n", rank, whatToDo);
      if (whatToDo == NOMOREWORK)
        break;
      MPI_Recv (&size_signal, sizeof (unsigned int), MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      x = (double *) malloc(sizeof(double) * size_signal);
      y = (double *) malloc(sizeof(double) * size_signal);
      MPI_Recv (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv (&x, size_signal, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv (&y, size_signal, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      circularCrossCorrelation(x, y, &ci);
      MPI_Send (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
      printf("PROCESS %d SENT RESULTS\n", rank);
    }
  }

  MPI_Barrier (MPI_COMM_WORLD);
  if (rank == 0) {
    printf("\nFinal report\n");
    printResults(argv+1);
    t1 = MPI_Wtime();
    printf("\nElapsed time = %.6f s\n", t1 - t0);
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
void circularCrossCorrelation(double *x, double *y, CONTROLINFO *ci) {

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
void printResults(char** filesToProcess){
  
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
 *  Operation carried out by the workers.
 *
 *  \param *ci pointer to the shared data structure
 *
 */
void savePartialResults(CONTROLINFO *ci) {
  filesManager[ci->filePosition].result[ci->rxyIndex] = ci->result;
  filesManager[ci->filePosition].rxyIndex++;
  ci->rxyIndex = filesManager[ci->filePosition].rxyIndex;
  ci->result = 0;
  if(filesManager[ci->filePosition].rxyIndex == filesManager[ci->filePosition].numbSamples){
    ci->filePosition++;
    ci->processing = false;
  }
}