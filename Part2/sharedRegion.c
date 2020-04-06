/**
 *  \file fifo.c (implementation file)
 *
 *  \brief Problem name: Producers / Consumers.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the producers / consumers:
 *     \li putVal
 *     \li getVal.
 *
 *  \author Francisco Gonçalves and Tiago Lucas - March 2019
 */

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h> 

#include "probConst.h"
#include "FILEINFO.h"
#include "CONTROLINFO.h"


/** \brief producer threads return status array */
extern int statusWorkers[NUMB_THREADS];

/** \brief names of files to process */
char* filesToProcess[MAX_FILES];

/** \brief array of results read for each file file */
FILEINFO *filesManager;

double **resultsMatrix;

double **calculatedResultsMatrix;

/** \brief number of files to process */
unsigned int numbFiles;

/** \brief file position in array with all names */
unsigned int filePosition;

/** \brief file pointer */
static FILE* filePointer;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
pthread_mutex_t accessF = PTHREAD_MUTEX_INITIALIZER;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
pthread_mutex_t accessR = PTHREAD_MUTEX_INITIALIZER;

/** \brief flag which warrants that the data transfer region is initialized exactly once */
pthread_once_t init = PTHREAD_ONCE_INIT;

bool isValidStopCharacter(char);

/**
 *  \brief Initialization of the shared region.
 *
 *  Internal monitor operation.
 */

void initialization (void)
{
  filePosition = 0;                                        /* shared region filepointer and byte pointer are both 0 */
  filesManager = (FILEINFO*)malloc(sizeof(FILEINFO)*numbFiles);
  resultsMatrix = malloc(numbFiles*DEFAULT_SIZE_SIGNAL*sizeof(double));
  calculatedResultsMatrix = malloc(numbFiles*DEFAULT_SIZE_SIGNAL*sizeof(double));
  filePointer = NULL;
}


/**
 *  \brief Insert the names of the files to be processed in an array.
 *
 *  Operation carried out by main.
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
 *  \brief Store a value in the data transfer region.
 *
 *  Operation carried out by the producers.
 *
 *  \param prodId producer identification
 *  \param val value to be stored
 */

bool getAPieceOfData(unsigned int workerId, double *x, double *y, CONTROLINFO *ci)
{
  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessF)) != 0)                                   /* enter monitor */
  { 
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on entering monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  pthread_once (&init, initialization);                                              /* internal data initialization */
  
  if(filePosition == numbFiles){
    if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessF)) != 0){                                 /* exit monitor */
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on exiting monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
    }
    return false;
  }

  if(ci->filePosition != filePosition && !ci->processing){
    
    filePointer = fopen(filesToProcess[filePosition], "rb");
    size_t samples;
    size_t i = fread(&samples, sizeof(int), 1, filePointer);

    ci->numbSamples = samples;
    ci->filePosition = filePosition;
    ci->processing = true;
    ci->rxyIndex = 0;
    
    if(samples > ci->numbSamples){
      x = (double*)malloc(sizeof(double)*samples);
      y = (double*)malloc(sizeof(double)*samples);
    }

    FILEINFO f ;
    fread(x, sizeof(double), samples, filePointer);
    fread(y, sizeof(double), samples, filePointer);

    if(filesManager[filePosition].read == false){
      double real[samples];
      fread(real, sizeof(double), samples, filePointer);
      f.read = true;
      f.rxyIndex = 0;
      f.filePosition = filePosition;
      f.numbSamples = ci->numbSamples;
      filesManager[filePosition] = f;   
      printf("value - %f\n", real[0]);
      if(samples > DEFAULT_SIZE_SIGNAL){
        resultsMatrix[filePosition] = (double*)realloc(resultsMatrix[filePosition], samples*sizeof(double));
        calculatedResultsMatrix[filePosition] = (double*)realloc(resultsMatrix[filePosition], samples*sizeof(double));
      }
      for (size_t t = 0; t < samples; t++){
        resultsMatrix[filePosition][t] = real[t];
      }
      filePosition++;

    }
    
    fclose(filePointer);
    
    
  }


  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessF)) != 0)                                 /* exit monitor */
  {
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on exiting monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  
  return true;   
}

/**
 *  \brief Get a value from the data transfer region.
 *
 *  Operation carried out by the consumers.
 *
 *  \param consId consumer identification
 *
 *  \return value
 */

void savePartialResults(unsigned int workerId, CONTROLINFO *ci)
{                                                                          

  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessR)) != 0)                                   /* enter monitor */
  { 
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on entering monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }

  calculatedResultsMatrix[ci->filePosition][ci->rxyIndex] = ci->result;
  filesManager[ci->filePosition].rxyIndex++;
  ci->rxyIndex = filesManager[ci->filePosition].rxyIndex;
  ci->result = 0;
  if(filesManager[ci->filePosition].rxyIndex == filesManager[ci->filePosition].numbSamples){
    ci->filePosition++;
    ci->processing = false;
  }

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessR)) != 0)                                   /* exit monitor */
  { 
    errno = statusWorkers[workerId];                                                             /* save error in errno */
    perror ("error on exiting monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
}

void printResults(){
  
  size_t i, x, numbErrors;

  for (i = 0; i < numbFiles; i++){
    numbErrors = 0;
    for (x = 0; x < filesManager[i].numbSamples; x++){
      if (resultsMatrix[i][x] != calculatedResultsMatrix[i][x]) {
          numbErrors++;
          printf("x- %i,y- %i\nmine-%f\treal %f\n",i,x, resultsMatrix[i][x], calculatedResultsMatrix[i][x]);
      }
    }
    if(numbErrors==0)
      printf("File %s was calculated correctly.\n", filesToProcess[i]);
    else 
      printf("File %s had %i errors in total.\n", filesToProcess[i], numbErrors);
  }
  
  free(resultsMatrix);
  free(calculatedResultsMatrix);
  free(filesManager);
}
