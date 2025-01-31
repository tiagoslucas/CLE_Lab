/**
 *  \file sharedRegion.c (implementation file)
 *
 *  \brief Problem name: Problem 1.
 *
 *  \author Francisco Gon�alves and Tiago Lucas - April 2019
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

/** \brief byte pointer inside file */
int *maxWordLEN;

int isValidStopCharacter(char);

/**
 *  \brief Initialization of the results region.
 *
 *  Internal monitor operation.
 */
void initialization (void)
{
  results = (CONTROLINFO*)calloc(numbFiles, sizeof(CONTROLINFO));
  maxWordLEN = calloc(numbFiles, sizeof(int));
  filePosition = 0;                                      /* shared region filePosition is 0 */
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
 *  \brief Get data from the files.
 *
 *  Operation carried out by the worker threads.
 *
 *  \param workerId				identification
 *  \param *dataToBeProcessed	pointer to the array with the data to process.
 *  \param *ci					pointer to the shared data structure.
 *
 */
bool getAPieceOfData(unsigned int workerId, unsigned char *dataToBeProcessed, CONTROLINFO *ci)
{

  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessF)) != 0){                                   /* enter monitor */
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
  size_t i, aux;
  
  if(filePointer == NULL)
    filePointer = fopen(filesToProcess[filePosition], "rb");
  
  ci->filePosition = filePosition;
  i = fread(dataToBeProcessed, 1, K, filePointer);

  if(i < K) {
    filePosition++;
    fclose(filePointer);
    filePointer = NULL;
  }else{
    aux = i;
    while(isValidStopCharacter(dataToBeProcessed[i-1]) == 0 && i > 0){
      i--;
    }
    if(i == 0)
      i = aux;
    fseek(filePointer, i-K,SEEK_CUR);
  }
  ci->numbBytes = i;

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessF)) != 0){                                 /* exit monitor */
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
 *  Operation carried out by the main thread.
 *
 *  \param workerId identification
 *  \param *ci		pointer to the shared data structure
 *
 *  \return value
 */
void savePartialResults(unsigned int workerId, CONTROLINFO *ci)
{                                                                          

  if ((statusWorkers[workerId] = pthread_mutex_lock (&accessR)) != 0){                                   /* enter monitor */
    errno = statusWorkers[workerId];                                                            /* save error in errno */
    perror ("error on entering monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }

  size_t filePosition = ci->filePosition;
  results[filePosition].numbBytes += ci->numbBytes;
  results[filePosition].numbWords += ci->numbWords;
  results[filePosition].maxWordLength = ci->maxWordLength;
  maxWordLEN[filePosition] = ci->maxWordLength;
  ci->numbWords = 0;

  for (size_t i = 0; i < ci->maxWordLength+1; i++){
    for (size_t j = 0; j < ci->maxWordLength; j++){
          results[filePosition].bidi[i][j] += ci->bidi[i][j];
          ci->bidi[i][j] = 0;
    }
  }

  if ((statusWorkers[workerId] = pthread_mutex_unlock (&accessR)) != 0){
    errno = statusWorkers[workerId];
    perror ("error on exiting monitor(CF)");
    statusWorkers[workerId] = EXIT_FAILURE;
    pthread_exit (&statusWorkers[workerId]);
  }
}


/**
 *  \brief Print the results of each file.
 *
 *  Operation carried out by main thread.
 *
 */
void printResults(){

  size_t x, y, i, max_len;

  for (i = 0; i < numbFiles; i++){
    max_len = maxWordLEN[i];
    
    printf("File name: %s\n", filesToProcess[i]);
    printf("Total number of words: %lu \n", results[i].numbWords);
    printf("Word length\n");

    int Words[maxWordLEN[i]];
    printf(" ");
    for (y = 0; y < max_len; y++){
      Words[y] = 0;
      printf("%*d\t", ALIGNMENT, y+1);
      for (x = 0; x <= max_len; x++)
        Words[y] += results[i].bidi[x][y];
    }
    printf("\n\n");

    printf(" ");
    for (x = 0; x < max_len; x++)
      printf("%*d\t", ALIGNMENT, Words[x]);
    
    printf("\n\n");

    printf(" ");
    for (x = 0; x < max_len; x++)
      printf("%*.2f\t", ALIGNMENT, (double) Words[x]/results[i].numbWords*100);

    printf("\n\n");
    
    for (x = 0; x < max_len + 1; x++){
    printf("%i",x);
      for (y = 0; y < max_len; y++){
        if(x > y+1)
          printf("\t");
        else if (Words[y] == 0)
          printf("%*.1f\t", ALIGNMENT, 0);
        else
          printf("%*.1f\t", ALIGNMENT, (double) results[i].bidi[x][y]/Words[y]*100);
          
      }
    printf("\n\n");
    }
  }
  free(results);
}

/**
 *  \brief Validate if a character is a stop character.
 *
 *  Operation carried out by all threads.
 *
 * \param character		character to which the validation is done.
 *
 * \return 				0 if the character provided is not a stop character, otherwise is the number of bytes the character should have. Can be used as true/false.
 *
 */
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
