#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include "probConst.h"
#include "FILEINFO.h"
#include "CONTROLINFO.h"

/* General definitions */
# define  WORKTODO       1
# define  NOMOREWORK     0

/* Allusion to internal functions */
static void presentDataFileNames(char *listOfFiles[], unsigned int size)
static void savePartialResults(CONTROLINFO*);
static void printResults(unsigned int, char**);
static void circularCrossCorrelation(double*, double*, CONTROLINFO*);

/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the worker threads and waiting for their termination.
 */
int main(int argc, char const *argv[]) {

  int nProc, rank;
  unsigned int numbFiles = argc - 1;
  filesManager = (FILEINFO*)malloc(sizeof(FILEINFO)*numbFiles);
  CONTROLINFO ci = {0};

  /* get processing configuration */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nProc);

  if (rank == 0) {

    double t0, t1;
    t0 = ((double) clock ()) / CLOCKS_PER_SEC;
    presentDataFileNames(argv + 1, --argc);

    FILE *f;
    FILEINFO *filesManager;
    unsigned int whatToDo, workProc;
    size_t filePos = 1, samples;

    if(argc < 2) {

      printf("Please insert text files to be processed as arguments!");
      whatToDo = NOMOREWORK;
      for (x = 1; x < totProc; x++)
        MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
      MPI_Finalize ();
      exit(EXIT_FAILURE);
        
    }

    /* processing the circular cross correlation between two signals */
    while (filePos <= filesToProcess) {
      for (int x = 1; x < nProc; x++){
        if (filePos > numbFiles) {
          workProc = x;
          break;
        } else if (f == NULL) {
          if((f = fopen (argv[filePos], "rb")) == NULL){
            filePos++;
            perror ("error on file opening for reading");
            whatToDo = NOMOREWORK;
      
            for (x = 1; x < totProc; x++)
              MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
            MPI_Finalize ();
            exit (EXIT_FAILURE);
          }
        }

        i = fread(&samples, sizeof(int), 1, f);

        ci->numbSamples = samples;
        ci->filePosition = filePos;
        ci->processing = true;
        ci->rxyIndex = 0;

        fread(x, sizeof(double), samples, f);
        fread(y, sizeof(double), samples, f);

        if(filesManager[filePosition].read == false){

          double real[samples];
          fread(real, sizeof(double), samples, f);
          filesManager[filePosition].read = true;
          filesManager[filePosition].rxyIndex = 0;
          filesManager[filePosition].filePosition = filePosition;
          filesManager[filePosition].numbSamples = samples;

          for (size_t t = 0; t < samples; t++){
            filesManager[filePosition].expected[t]= real[t];
          }

        }
        
        if (fclose (f) == EOF){ 
          perror ("error on closing file");
          whatToDo = NOMOREWORK;
          for (x = 1; x < totProc; x++)
            MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
          MPI_Finalize ();
          exit (EXIT_FAILURE);
        }
        filePos++;

      }

      MPI_Barrier(MPI_COMM_WORLD);
      for (int i = 0; i < nProc; i++) {
        MPI_Recv (&ci, sizeof(CONTROLINFO), MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        savePartialResults(&ci);
      }
    }

    /* dismiss worker processes */
    whatToDo = NOMOREWORK;
    for (x = 1; x < totProc; x++)
      MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    printf ("\nFinal report\n");
    printResults();
    t1 = ((double) clock ()) / CLOCKS_PER_SEC;
    printf ("\nElapsed time = %.6f s\n", t1 - t0);

  } else { /* worker processes */

    double x[DEFAULT_SIZE_SIGNAL];
    double y[DEFAULT_SIZE_SIGNAL];

    while (true) {
      ci = {0};
      ci.filePosition = -1;
      ci.processing = false;

      MPI_Recv (&whatToDo, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if (whatToDo == NOMOREWORK)
        break;
      MPI_Recv (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv (&x, DEFAULT_SIZE_SIGNAL, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv (&y, DEFAULT_SIZE_SIGNAL, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      circularCrossCorrelation(x, y, &ci);
      MPI_Send (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize ();
  return EXIT_SUCCESS;
}

/**
 *  \brief Insert the names of the files to be processed in an array.
 *
 *  Operation carried out by the main thread.
 *
 *  \param listOfFiles names of files to process
 *  \param size number of text files to be processed
 */
void presentDataFileNames(char *listOfFiles[], unsigned int size){
  numbFiles = size;
  for(int i = 0; i < size; i++)
    filesToProcess[i] = listOfFiles[i];
}

/**
 *  \brief Perform the comparison between two sets of signals
 *
 *  Operation carried out by the workers.
 *
 */
void circularCrossCorrelation(double *x, double *y, CONTROLINFO *ci) {

   size_t i, j;
   size_t n = ci->numbSamples;
   size_t temp = ci->rxyIndex; 

   for(j = 0; j < n; j++){
      ci->result += x[j] * y[(temp+j)%n];
   }
}

/**
 *  \brief Print all the results stored in result data storage.
 *
 *  Operation carried out by the dispatcher.
 *
 */
void printResults(){
  
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
      printf("File %s had %i errors in total.\n", filesToProcess[i], numbErrors);
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
    filePosition++;
    ci->filePosition++;
    ci->processing = false;
  }
}