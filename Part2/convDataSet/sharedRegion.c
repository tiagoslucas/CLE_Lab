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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "probConst.h"
#include "CONTROLINFO.h"

/** \brief producer threads return status array */
extern int statusWorkers[NUMB_THREADS];

/** \brief names of files to process */
char* filesToProcess[MAX_FILES];

/** \brief array of results read for each file file */
CONTROLINFO** results;

/** \brief array of results calculated for each file file */
CONTROLINFO** calculatedResults;

/** \brief number of files to process */
unsigned int numbFiles;

/** \brief file position in array with all names */
unsigned int filePosition;

/** \brief file pointer */
static FILE* filePointer;

/** \brief Index of element rxy */
unsigned int rxyIndex;

/** \brief New value of signals per file */
unsigned int signalsPerFile;

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
  calculatedResults = (CONTROLINFO**)calloc(numbFiles*NUMB_SIGNALS_PER_FILE,sizeof(CONTROLINFO));
  results = (CONTROLINFO**)calloc(numbFiles*NUMB_SIGNALS_PER_FILE,sizeof(CONTROLINFO));
  filePosition = rxyIndex = 0;                                        /* shared region filepointer and byte pointer are both 0 */
  filePointer = NULL;
  signalsPerFile = NUMB_SIGNALS_PER_FILE;
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

  if(filePointer == NULL)
    filePointer = fopen(filesToProcess[filePosition], "rb");

  size_t samples;
  size_t i = fread(&samples, sizeof(int), 1, filePointer);

  if(i == 1){

    ci->numbSamples = samples;
    ci->rxyIndex = rxyIndex;
    size_t j;
    rxyIndex += 1;

    fread(x, sizeof(double), samples, filePointer);
    fread(y, sizeof(double), samples, filePointer);
    fread(&ci->result, sizeof(double), samples, filePointer);

    printf("j- %i\n", samples);
    printf("X- %f \n", x[0]);
    printf("Y- %f\n", y[0]);
    printf("results- %f\n", ci->result[0]);

    if(rxyIndex > signalsPerFile){
      results[filePosition] = (CONTROLINFO*)realloc(results[filePosition],rxyIndex*sizeof(CONTROLINFO));
      signalsPerFile = rxyIndex;
    }
    for(j = 0; j < samples; j++)
      results[filePosition][rxyIndex].result[j] = ci->result[j];

  }else{
    rxyIndex = 0;
    signalsPerFile = NUMB_SIGNALS_PER_FILE;
    filePosition++;
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

  size_t filePosition = ci->filePosition;
  size_t numbSamples = ci->numbSamples;

  for (size_t i = 0; i < numbSamples; i++){
    calculatedResults[filePosition][rxyIndex].result[i] = ci->result[i];
    //printf(" %i ", calculatedResults[filePosition][rxyIndex].result[i]);
    //printf("\n");
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
  
  bool isResultCorrect = true;
  size_t i, j, x;

  for (i = 0; i < numbFiles; i++)
  {
    for (j = 0; j < NUMB_SIGNALS_PER_FILE; j++)
    {
      for (x = 0; x < results[i][j].numbSamples; x++)
      {
        if (results[i][j].result[x] != calculatedResults[i][j].result[x])
        {
          printf("erro no ficheiro - %i\n", i);
        }
        
      }
      
    }
    
  }
  
  free(calculatedResults);
  free(results);
}
