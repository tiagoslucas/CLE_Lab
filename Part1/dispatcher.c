/**
 *  \file binSortListNames.c (implementation file)
 *
 *  \brief Sorting of a list of names stored in a file.
 *
 *  It sorts the list using a binary sorting algorithm which allows the sorting of parts of the original list by
 *  different processes of a group in a hierarchical way.
 *  The list of names must be supplied by the user.
 *  MPI implementation.
 *
 *  \author António Rui Borges - April 2020
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "probConst.h"
#include "CONTROLINFO.h"

/* General definitions */

# define  WORKTODO       1
# define  NOMOREWORK     0

/** \brief results of processed text */
CONTROLINFO *results;

/** \brief byte pointer inside file */
int *maxWordLEN;

/* Allusion to internal functions */
static void savePartialResults(CONTROLINFO*);
static int isValidStopCharacter(char);
static void printResults(unsigned int, char**);
static void processText(unsigned char*, CONTROLINFO*);

/**
 *  \brief Main function.
 *
 *  Instantiation of the processing configuration.
 *
 *  \param argc number of words of the command line
 *  \param argv list of words of the command line
 *
 *  \return status of operation
 */

int main (int argc, char *argv[]){
  int rank,                                                              /* number of processes in the group */
  totProc;                                                               /* group size */
  double start, finish;
  unsigned int numbFiles = argc - 1;                                     /* number of files to process*/
  
  /* get processing configuration */

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &totProc);

  MPI_Barrier (MPI_COMM_WORLD);
  start = MPI_Wtime();

  /* processing */

  if (rank == 0){ 
          /* dispatcher process
           it is the first process of the group */

    FILE *f;                               /* pointer to the text stream associated with the file name */
    unsigned int whatToDo;                 /* command */
    unsigned int aux,                     /* counting variables */
                 workProc,
                 i,
                 x;
    CONTROLINFO ci = {0};                      /* data transfer variable */
    unsigned char dataToBeProcessed[K+1];
    size_t filePos = 1;
    results = (CONTROLINFO*) malloc(numbFiles * sizeof(CONTROLINFO));
    maxWordLEN = (int *) malloc(numbFiles * sizeof(int));

    /* check running parameters and load list of names into memory */

    if (argc < 2){ 
      perror("Please insert text files to be processed as arguments!");
      whatToDo = NOMOREWORK;
      for (x = 1; x < totProc; x++)
        MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
      MPI_Finalize ();
      return EXIT_FAILURE;
    }
    while(filePos <= numbFiles){
      workProc = 1;
      for (x = 1; x < totProc; x++, workProc++){
        
        if(filePos > numbFiles){
          break;
        }
          
        if(f == NULL) {
          if((f = fopen (argv[filePos], "rb")) == NULL){
            filePos++;
            perror ("error on file opening for reading");
            whatToDo = NOMOREWORK;
            for (x = 1; x < totProc; x++)
              MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
            MPI_Finalize ();
            exit (EXIT_FAILURE);
          }
        }

        ci.filePosition = filePos;
        i = fread(dataToBeProcessed, 1, K, f);

        if(i < K) {
          filePos++;
      
          if (fclose (f) == EOF){ 
            perror ("error on closing file");
            whatToDo = NOMOREWORK;
            for (x = 1; x < totProc; x++)
              MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
            MPI_Finalize ();
            exit (EXIT_FAILURE);
          }

          f = NULL;

        } else {
          aux = i;
          while(isValidStopCharacter(dataToBeProcessed[i-1]) == 0 && i > 0){
            i--;
          }
          if(i == 0)
            i = aux;
          fseek(f, i-K,SEEK_CUR);
        }
        ci.numbBytes = i;
     
      /* distribute sorting task */
        printf("START SEND TO WORKER %i\n", x);
        whatToDo = WORKTODO;
        MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
        MPI_Send (&ci, sizeof (CONTROLINFO), MPI_BYTE, x, 0, MPI_COMM_WORLD);
        MPI_Send (dataToBeProcessed, K+1, MPI_CHAR, x, 0, MPI_COMM_WORLD);
        memset(dataToBeProcessed, 0, K+1);
      }

      for (x = 1; x < workProc; x++) {
        printf("WAITING RESULTS FROM WORKER %d\n", x);
        MPI_Recv (&ci, sizeof(CONTROLINFO), MPI_CHAR, x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        savePartialResults(&ci);
      }
      
    }
    
    /* dismiss worker processes */

    whatToDo = NOMOREWORK;
    for (x = 1; x < totProc; x++)
      MPI_Send (&whatToDo, 1, MPI_UNSIGNED, x, 0, MPI_COMM_WORLD);
    

  } else { 
    /* worker processes
    the remainder processes of the group */
    unsigned int whatToDo;                                                   /* command */
    CONTROLINFO ci;                                                          /* data transfer variable */
    unsigned char dataToBeProcessed[K+1];                                    /* text to process */

    while (true){
      printf("STARTED WORKER %d.\n",rank);
      MPI_Recv (&whatToDo, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("whatToDo: %i\n", whatToDo);
      if (whatToDo == NOMOREWORK)
        break;
      printf("totProc: %i\n", totProc);
      MPI_Recv (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv (&dataToBeProcessed, K+1, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      processText(dataToBeProcessed, &ci);
      printf("SENDING INFO\n");
      MPI_Send (&ci, sizeof (CONTROLINFO), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
      printf("RESULTS SENT\n");
    }
  }

  MPI_Barrier (MPI_COMM_WORLD);
  finish = MPI_Wtime();
  if(rank == 0) {
    printf("Execution time: %f seconds\n", finish - start);
    //printResults(numbFiles, argv+1);
    free(results);
    free(maxWordLEN);
  }
  MPI_Finalize ();
  return EXIT_SUCCESS;
}



/**
 *  \brief Get a value from the data transfer region.
 *
 *  Operation carried out by the main thread.
 *
 *  \param *ci    pointer to the shared data structure
 *
 *  \return value
 */
void savePartialResults(CONTROLINFO *ci){                                                                          

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

}

/**
 *  \brief Validate if a character is a stop character.
 *
 *  Operation carried out by dispatcher process.
 *
 * \param character   character to which the validation is done.
 *
 * \return        0 if the character provided is not a stop character, otherwise is the number of bytes the character should have. Can be used as true/false.
 *
 */
static int isValidStopCharacter(char character) {
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


/**
 *  \brief Print the results of each file.
 *
 *  Operation carried out by dispatcher process.
 *
 */

static void printResults(unsigned int numbFiles, char *filesToProcess[]){

  size_t x, y, i, max_len;

  for (i = 0; i < numbFiles; i++){
    max_len = maxWordLEN[i];
    
    printf("File name: %s\n", filesToProcess[i]);
    printf("Total number of words: %lu \n", results[i].numbWords);
    printf("Word length\n");

    int Words[max_len];
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
}


/**
 *  \brief Process text function.
 *
 *  Processes text sent by dispatcher and calculates the required statistics.
 *
 *  \param dataToBeProcessed chunk of text data being processed
 *  \param ci structure where calculated statistics are saved
 */

static void processText(unsigned char *dataToBeProcessed, CONTROLINFO *ci) {
    char cha;
    bool inWord = false;
    int skip, nVowels = 0, nCharacters = 0, maxWordLength = 0, length = ci->numbBytes;
    
    for (int i = 0; i < length; i++) {
      skip = 0;
        if((char)dataToBeProcessed[i] == (char)0xC3) {
          skip = 1;
            i += 1;
        } else if((char)dataToBeProcessed[i] == (char)0xE2) {
          skip = 2;
            i += 2;
    }
        cha = dataToBeProcessed[i];

    if (cha >= 48 && cha <= 57) {
      inWord = true;
            nCharacters++;
        } else if (skip == 0 && cha >= 65 && cha <= 90) {
            inWord = true;
            nCharacters++;
          if (cha == 65 || cha == 69 || cha == 73 || cha == 79 || cha == 85)
            nVowels++;
        } else if (skip == 0 && cha >= 97 && cha <= 122) {
            inWord = true;
            nCharacters++;
          if (cha == 97 || cha == 101 || cha == 105 || cha == 111 || cha == 117)
            nVowels++;
        } else if (skip == 0 && cha == '_') {
            inWord = true;
            nCharacters++;
    } else if ((skip == 0 && cha == (char)0x27) || (skip == 2 && cha == (char)0x98) || (skip == 2 && cha == 0x99)) {
            ;
    } else if (skip == 1) {
      if (cha == (char)0xA7 || cha == (char)0x87) {
              inWord = true;
              nCharacters++;
          } else if (cha == (char)0xA1 || cha == (char)0xA0 || cha == (char)0xA2 || cha == (char)0xA3 || cha == (char)0x81 || cha == (char)0x80 || cha == (char)0x82 || cha == (char)0x83) {
              inWord = true;
              nCharacters++;
              nVowels++;
          } else if (cha == (char)0xA9 || cha == (char)0xA8 || cha == (char)0xAA || cha == (char)0x89 || cha == (char)0x88 || cha == (char)0x8A) {
              inWord = true;
              nCharacters++;
              nVowels++;
          } else if (cha == (char)0xAD || cha == (char)0xAC || cha == (char)0x8D || cha == (char)0x8C) {
              inWord = true;
              nCharacters++;
              nVowels++;
          } else if (cha == (char)0xB3 || cha == (char)0xB2 || cha == (char)0xB4 || cha == (char)0xB5 || cha == (char)0x93 || cha == (char)0x92 || cha == (char)0x94 || cha == (char)0x95) {
          inWord = true;
              nCharacters++;
              nVowels++;
          } else if (cha == (char)0xBA || cha == (char)0xB9 || cha == (char)0x9A || cha == (char)0x99) {
              inWord = true;
              nCharacters++;
              nVowels++;
        }
        } else if ((inWord && (skip == 0 && isValidStopCharacter(cha) == 1)) || (inWord && skip == 2 && isValidStopCharacter(cha) == 3) ) {
            ci->bidi[nVowels][nCharacters - 1]++;
            ci->numbWords++;
            if (nCharacters > maxWordLength)
              maxWordLength = nCharacters;
            nCharacters = 0;
            nVowels = 0;
            inWord = false;
        }
    }
    
    if (maxWordLength > ci->maxWordLength)
      ci->maxWordLength = maxWordLength;
}