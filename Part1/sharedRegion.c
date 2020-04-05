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

int isValidStopCharacter(char);

/**
 *  \brief Initialization of the shared region.
 *
 *  Internal monitor operation.
 */

void initialization (void)
{
  results = (CONTROLINFO*)calloc(numbFiles, sizeof(CONTROLINFO));
  //CONTROLINFO aux = (CONTROLINFO) {0, 0, 0, {0}};
  //int i;
  //for(i = 0; i < numbFiles; i++)
    //results[i] = aux;
  filePosition = 0;                                        /* shared region filePosition and byte pointer are both 0 */
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

bool getAPieceOfData(unsigned int workerId, unsigned char* dataToBeProcessed, CONTROLINFO *ci)
{
  if(filePosition == numbFiles)
    return false;

  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessF)) != 0)
  { 
    errno = statusWorkers[workerId];
    perror ("error on entering monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
  pthread_once (&init, initialization);

  size_t i, aux;
  
  if(filePointer == NULL)
    filePointer = fopen(filesToProcess[filePosition], "rb");
  
  ci->filePosition = filePosition;
  ci->numbWords = 0;
  ci->maxWordLength = 0;

  i = fread(dataToBeProcessed, sizeof(char), K, filePointer);

  if(i < K) {
    filePosition++;
    fclose(filePointer);
    filePointer = NULL;
  } else {
    aux = i;
    while(isValidStopCharacter(dataToBeProcessed[i-1]) == 0 && i > 0){
      i--;
    }
    if(i == 0)
      i = aux;
    fseek(filePointer, i-K,SEEK_CUR);
  }
  ci->numbBytes = i;

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessF)) != 0)
  {
    errno = statusWorkers[workerId];
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
  results[filePosition].numbBytes += ci->numbBytes;
  results[filePosition].numbWords += ci->numbWords;

  for (size_t i = 0; i < ci->maxWordLength; i++){
    for (size_t j = 0; j < ci->maxWordLength; j++){
      results[filePosition].bidi[i][j] += ci->bidi[i][j];
    }
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

  size_t x, y, i, max_len;

  for (i = 0; i < numbFiles; i++){

    max_len = results[i].maxWordLength;
    
    printf("File name: %s\n", filesToProcess[i]);
    printf("Total number of words: %lu \n", results[i].numbWords);
    printf("Word length\n");

    int numbWords[max_len];

    for (x = 0; x < max_len; x++){
      for (y = 0; y < max_len; y++){
        numbWords[x] += results[i].bidi[y][x];
        if(x == 0)
          ;//printf("\t%i\t", y+1);
      }
    }
    //printf("\n");
    
    for (x = 0; x < max_len+1; x++){
      for (y = 0; y < max_len+1; y++){
        if(x == 0)
          ;//printf("\t %f \t", (double) (numbWords[y]/max_len)*100);
        else
          ;//printf("%i\t%f\t", (double) results[i].bidi[y][x]/numbWords[x] * 100);
      }
      ;//printf("\n");
    }
  }
  
  free(results);
}

int isValidStopCharacter(char character) {
  char separation[15] = { (char)0x20, (char)0x9, (char)0xA, '-', '"', '(', ')', '[', ']', '.', ',', ':', ';', '?', '!' };
  char separation3[4] = { (char)0x9C, (char)0x9D, (char)0x93, (char)0xA6 };
  int x;
  for (x = 0; x < 15; x++)
    if (character == separation[x])
      return 1;
  for (x = 0; x < 4; x++)
    if (character == separation3[x])
      return 3;
  return 0;
}
