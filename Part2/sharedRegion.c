/**
 *  \file sharedRegion.c (implementation file)
 *
 *  \brief Problem name: Problem 2.
 *
 *  \author Francisco Gonçalves and Tiago Lucas - April 2019
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

/** \brief array with information for each file file */
FILEINFO *filesManager;

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

/**
 *  \brief Initialization of the shared region.
 *
 *  Internal monitor operation.
 */
void initialization (void)
{
  filePosition = 0;                                        /* shared region filepointer and byte pointer are both 0 */
  filesManager = (FILEINFO*)malloc(sizeof(FILEINFO)*numbFiles);
  filePointer = NULL;
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
 *  \brief Store a value in the data transfer region.
 *
 *  Operation carried out by the worker threads.
 *
 *  \param workerId worker identification
 *  \param *x pointer to the array with first signals of the pair
 *  \param *x pointer to the array with second signals of the pair
 *  \param *ci pointer to the shared data structure
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

  if(!ci->processing){
    
    filePointer = fopen(filesToProcess[filePosition], "rb");
    size_t samples;
    size_t i = fread(&samples, sizeof(int), 1, filePointer);

    ci->numbSamples = samples;
    ci->filePosition = filePosition;
    ci->processing = true;
    ci->rxyIndex = 0;

    fread(x, sizeof(double), samples, filePointer);
    fread(y, sizeof(double), samples, filePointer);

    if(filesManager[filePosition].read == false){

      double real[samples];
      fread(real, sizeof(double), samples, filePointer);
      filesManager[filePosition].read = true;
      filesManager[filePosition].rxyIndex = 0;
      filesManager[filePosition].filePosition = filePosition;
      filesManager[filePosition].numbSamples = samples;

      for (size_t t = 0; t < samples; t++){
        filesManager[filePosition].expected[t]= real[t];
      }

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
 *  \brief Get a value from the data transfer region and save it in result data storage.
 *
 *  Operation carried out by the worker threads.
 *
 *  \param workerId worker identification
 *	\param *ci pointer to the shared data structure
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

  filesManager[ci->filePosition].result[ci->rxyIndex] = ci->result;
  filesManager[ci->filePosition].rxyIndex++;
  ci->rxyIndex = filesManager[ci->filePosition].rxyIndex;
  ci->result = 0;
  if(filesManager[ci->filePosition].rxyIndex == filesManager[ci->filePosition].numbSamples){
    filePosition++;
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

/**
 *  \brief Print all the results stored in result data storage.
 *
 *  Operation carried out by the main thread.
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
