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
#include "controlInfo.h"

/** \brief producer threads return status array */
extern int statusWorkers[NUMB_THREADS];

/** \brief names of files to process */
const char* filesToProcess;

/** \brief names of files to process */
controlInfo results[];

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

/**
 *  \brief Initialization of the shared region.
 *
 *  Internal monitor operation.
 */

void initialization (void)
{
  filePointer = bytePointer = 0;                                        /* shared region filepointer and byte pointer are both 0 */
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
  
  int i;
  numbFiles = size;

  for(i = 0; i < size; i++){
    filesToProcess = malloc(strlen(listOfFiles[i]) + 1);
    strcpy(filesToProcess,listOfFiles[i]);
  }

}


/**
 *  \brief Store a value in the data transfer region.
 *
 *  Operation carried out by the producers.
 *
 *  \param prodId producer identification
 *  \param val value to be stored
 */

bool getAPieceOfData (unsigned int workerId, char dataToBeProcessed[], controlInfo *ci)
{
  bool hasData = true;
  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
     { errno = statusWorkers[workerId];                                                            /* save error in errno */
       perror ("error on entering monitor(CF)");
       statusWorkers[workerId] = EXIT_FAILURE;
       pthread_exit (&statusWorkers[workerId]);
     }
  pthread_once (&init, initialization);                                              /* internal data initialization */

  int i;
  file = fopen(files[filePointer], "rb");


  if(bytePointer != 0){
    fseek(file, bytePointer, SEEK_SET);
  }

  if(i = fread(&dataToBeProcessed, K, 1, file) < K) {
    filePointer++;
    bytePointer = 0;
  }
  else
    bytePointer += K;

  while(!isValidStopCharacter(i, dataToBeProcessed)){
    i--;
  }

  if(filePointer == numbFiles - 1 )
    hasData = false;

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessCR)) != 0)                                  /* exit monitor */
     { errno = statusWorkers[workerId];                                                            /* save error in errno */
       perror ("error on exiting monitor(CF)");
       statusWorkers[workerId] = EXIT_FAILURE;
       pthread_exit (&statusWorkers[workerId]);
     }
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

void savePartialResults (unsigned int workerId, controlInfo *ci)
{
  unsigned int val;                                                                               /* retrieved value */

  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
     { errno = statusWorkers[workerId];                                                            /* save error in errno */
       perror ("error on entering monitor(CF)");
       statusWorkers[workerId] = EXIT_FAILURE;
       pthread_exit (&statusWorkers[workerId]);
     }

  val = mem[ri];                                                                   /* retrieve a  value from the FIFO */
  ri = (ri + 1) % K;


  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessCR)) != 0)                                   /* exit monitor */
     { errno = statusWorkers[workerId];                                                             /* save error in errno */
       perror ("error on exiting monitor(CF)");
       statusWorkers[workerId] = EXIT_FAILURE;
       pthread_exit (&statusWorkers[workerId]);
     }

  return val;
}



bool isValidStopCharacter(unsigned int i, char* dataToBeProcessed){

}
