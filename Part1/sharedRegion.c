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
char *filesToProcess[MAX_FILES];

/** \brief results of processed text */
CONTROLINFO* results;

/** \brief number of files to process */
unsigned int numbFiles;

/** \brief file pointer */
int filePosition;

/** \brief byte pointer inside file */
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
  //CONTROLINFO aux = (CONTROLINFO) {0, 0, 0, {0}};
  //int i;
  results = (CONTROLINFO*)calloc(numbFiles, sizeof(CONTROLINFO));
  //for(i = 0; i < numbFiles; i++)
    //results[i] = aux;

  filePosition = 0;                                        /* shared region filePosition and byte pointer are both 0 */
  printf("Initialization complete\n");
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

bool getAPieceOfData(unsigned int workerId, unsigned char *dataToBeProcessed, CONTROLINFO *ci)
{
  if(filePosition == numbFiles)
    return false;

  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessF)) != 0)                                   /* enter monitor */
  { 
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on entering monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  pthread_once (&init, initialization);                                              /* internal data initialization */

  size_t i, aux;
  
  if(filePointer == NULL)
    filePointer = fopen(filesToProcess[filePosition], "rb");
  
  printf("Opened file\n");
  ci->filePosition = filePosition;
  
  i = fread(&dataToBeProcessed, 1, K, filePointer);
  printf("%i\n", i);
  printf("%s\n", &dataToBeProcessed);

  if(i < K) {
    filePosition++;
    fclose(filePointer);
    filePointer = NULL;
  }else{
    aux = i;
    printf("entering loop\n");
    while(!isValidStopCharacter(dataToBeProcessed[i-1]) && i > 0){
      i--;
    }
    if(i == 0)
      i = aux;
  }

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessF)) != 0)                                 /* exit monitor */
  {
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on exiting monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  
  ci->numbWords = 0;
  ci->numbBytes = i;
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
  results[filePosition].numbBytes += ci->numbBytes;
  results[filePosition].numbWords += ci->numbWords;

  //mudar o 50 para variável de tamanho máx de palavra
  for (size_t i = 0; i < 50; i++){
    for (size_t j = 0; j < 50; j++){
     results[filePosition].bidi[i][j] += ci->bidi[i][j];
    }
  }
  printf("Saving results end\n");

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessR)) != 0)                                   /* exit monitor */
  { 
    errno = statusWorkers[workerId];                                                             /* save error in errno */
    perror ("error on exiting monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
}

void printResults(){

  int i;
  for (i = 0; i < numbFiles; i++){
    printf("File name: %s\n", filesToProcess[i]);
    printf("Total number of words: %ld\n", results[i].numbWords);
    printf("Word length\n");

  }
  
  free(results);
}

bool isValidStopCharacter(char character) {
    char separation[19] = { (char)0x20, (char)0x9, (char)0xA, '-', '"', '(', ')', '[', ']', '.', ',', ':', ';', '?', '!', (char)0x9C, (char)0x9D, (char)0x93, (char)0xA6 };
    for (int x = 0; x < 15; x++)
        if (character == separation[x])
            return true;
    return false;
}
