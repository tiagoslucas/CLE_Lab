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
unsigned int *signalsPerFile = NULL;

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
  filePosition = rxyIndex = 0;                                        /* shared region filepointer and byte pointer are both 0 */
  calculatedResults = (CONTROLINFO**)calloc(numbFiles*NUMB_SIGNALS_PER_FILE,sizeof(CONTROLINFO));
  results = (CONTROLINFO**)calloc(numbFiles*NUMB_SIGNALS_PER_FILE,sizeof(CONTROLINFO));
  filePointer = NULL;
  signalsPerFile = malloc(numbFiles * sizeof(int));
  for (size_t i = 0; i < numbFiles; ++i) 
    signalsPerFile[i] = NUMB_SIGNALS_PER_FILE;
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
    rxyIndex++;
    if(!ci->initial){
      if(j > ci->numbSamples){
        x = (double*)realloc(x, sizeof(double)*samples);
        y = (double*)realloc(y, sizeof(double)*samples);
        ci->result = (double*)realloc(ci->result, sizeof(double)*samples);
      }
    }else{
      x = (double*)malloc(sizeof(double)*samples);
      y = (double*)malloc(sizeof(double)*samples);
      ci->result = (double*)malloc(sizeof(double)*samples);
    }

    printf("Number of samples - %i\n", samples);
    fread(x, sizeof(double), samples, filePointer);
    fread(y, sizeof(double), samples, filePointer);
    fread(ci->result, sizeof(double), samples, filePointer);
    
    //printf("values\n");
    //printf("X- %f \n", x[0]);
    //printf("Y- %f\n", &y[0]);

    /*if(rxyIndex > signalsPerFile[filePosition]){ 
      printf("Increasing size of results\n");
      calculatedResults[filePosition] = (CONTROLINFO*)realloc(results[filePosition],rxyIndex*sizeof(CONTROLINFO));
      results[filePosition] = (CONTROLINFO*)realloc(results[filePosition],rxyIndex*sizeof(CONTROLINFO));
      signalsPerFile[filePosition] = rxyIndex;
    }
    for(j = 0; j < samples; j++){
      //printf("bro\n");
      printf("%f\n", results[filePosition][rxyIndex].result[j]);
      //results[filePosition][rxyIndex].result[j] = ci.result;
    }*/

  }else{
    signalsPerFile[filePosition] = rxyIndex;
    rxyIndex = 0;
    filePosition++;
    fclose(filePointer);
  }
  printf("left getdata\n");

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

  size_t i, filePos, samples;
  filePos = ci->filePosition;
  samples = ci->numbSamples;

  for (i = 0; i < samples; i++){
    calculatedResults[filePos][rxyIndex].result[i] = ci->result[i];
    ci->result[i] = 0;
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
  size_t i, j, x, numbErrors, numbSignalsF;

  for (i = 0; i < numbFiles; i++)
  {
    numbSignalsF = signalsPerFile[i];
    numbErrors = 0;
    for (j = 0; j < numbSignalsF; j++)
    {
      for (x = 0; x < results[i][j].numbSamples; x++)
      {
        if (results[i][j].result[x] != calculatedResults[i][j].result[x])
        {
          numbErrors++;
          isResultCorrect = false;
          printf("Erro no ficheiro - %s, no sinal %i.\n", filesToProcess[i], j);
        }
        
      }
      free(results[i][j].result);
      free(calculatedResults[i][j].result);
    }
  if(isResultCorrect)
    printf("Ficheiro %s não teve erros no seu cálculo.\n", filesToProcess[i]);
  else 
    printf("Ficheiro %s teve um total de %i erros no seu cálculo.\n", filesToProcess[i], numbErrors);
  isResultCorrect = true;
  }
  
  free(calculatedResults);
  free(results);
}
