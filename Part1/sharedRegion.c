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
char **filesToProcess;

/** \brief results of processed text */
CONTROLINFO* results;

/** \brief number of files to process */
unsigned int numbFiles;

/** \brief file pointer */
unsigned int filePointer;

/** \brief byte pointer inside file */
unsigned int bytePointer;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

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
  printf("Initialization start\n");
  CONTROLINFO aux = (CONTROLINFO) {0, 0, 0, {0}};
  int i;
  results = (CONTROLINFO*)malloc(sizeof(CONTROLINFO)*numbFiles);
  for(i = 0; i < numbFiles; i++)
    results[i] = aux;

  filePointer = bytePointer = 0;                                        /* shared region filepointer and byte pointer are both 0 */
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
  filesToProcess = listOfFiles;
  printf("Presented filenames\n");
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
  bool hasData = true;
  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
  { 
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on entering monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  pthread_once (&init, initialization);                                              /* internal data initialization */

  printf("Getting piece of data\n");
  size_t i, aux;
  FILE * fp;
  
  fp = fopen(&filesToProcess[filePointer], "r");
  
  printf("Opened file\n");
  ci->filePointer = filePointer;
  ci->numbWords = 0;

  if(bytePointer != 0)
    fseek(fp, bytePointer, SEEK_SET);
  
  i = fread(&dataToBeProcessed, 1, K, fp);
  fclose(fp);
  printf("fread complete\n");
  if(i < K) {
    filePointer++;
    bytePointer = 0;
  }
  else
  {
    aux = i;
    while(!isValidStopCharacter(dataToBeProcessed[i-1]) && i > 0){
      i--;
    }
    if(i == 0)
      i = aux;
    bytePointer += i;
  }
  printf("data processed\n");
  ci->numbBytes = i;

  if(filePointer == (numbFiles - 1) )
    hasData = false;

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessCR)) != 0)                                 /* exit monitor */
  {
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on exiting monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  printf("Getting data complete\n");
return hasData;   
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

  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
  { 
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on entering monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  printf("Saving results start\n");
  size_t filePosition = ci->filePointer;

  CONTROLINFO ciNew = results[filePosition];

  ciNew.numbBytes += ci->numbBytes;
  ciNew.numbWords += ci->numbWords;

  //mudar o 50 para variável de tamanho máx de palavra
  for (size_t i = 0; i < 50; i++){
    for (size_t j = 0; j < 50; j++){
     ciNew.bidi[i][j] += ci->bidi[i][j];
    }
  }

  results[filePosition] = ciNew;
  printf("Saving results end\n");

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessCR)) != 0)                                   /* exit monitor */
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
