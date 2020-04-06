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
FILEINFO *results;

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
  results = (FILEINFO*)malloc(sizeof(FILEINFO)*numbFiles);
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
  
  if(filePointer == NULL && ci->filePosition != filePosition){
    filePointer = fopen(filesToProcess[filePosition], "rb");
    size_t samples;
    size_t i = fread(&samples, sizeof(int), 1, filePointer);
    printf("samples - %i\n", samples);
    ci->rxyIndex = 0;
    ci->numbSamples = samples;

    FILEINFO *f = (FILEINFO*) malloc( sizeof(FILEINFO) + sizeof(double [ci->numbSamples]));
    fread(x, sizeof(double), samples, filePointer);
    fread(y, sizeof(double), samples, filePointer);
    fread(f->expected, sizeof(double), samples, filePointer);

    fclose(filePointer);

    if(!ci->initial){
      if(samples > ci->numbSamples){
        x = (double*)malloc(sizeof(double)*samples);
        y = (double*)malloc(sizeof(double)*samples);
      }
    }else{
      if(samples > DEFAULT_SIZE_SIGNAL){
        x = (double*)malloc(sizeof(double)*samples);
        y = (double*)malloc(sizeof(double)*samples);
      }
    }
    
    results[filePosition].filePosition = filePosition;
    results[filePosition].numbSamples = ci->numbSamples;
    ci->filePosition = filePosition;
    ci->initial = false;

    printf("X - %f\tY - %f\tR - %f\n", &x[0], &y[0], &f->expected[0]);
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


  results[filePosition].result[ci->rxyIndex] = ci->result;
  results[filePosition].rxyIndex++;

  if(results[ci->filePosition].rxyIndex == results[ci->filePosition].numbSamples)
    filePosition++;
  ci->result = 0;


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
    for (x = 0; x < results[i].numbSamples; x++){
      if (results[i].result[x] != results[i].expected[x]) {
          numbErrors++;
      }
    }
    if(numbErrors==0)
      printf("File %s was calculated correctly.\n", filesToProcess[i]);
    else 
      printf("File %s had %i errors in total.\n", filesToProcess[i], numbErrors);
  }
  
  free(results);
}
