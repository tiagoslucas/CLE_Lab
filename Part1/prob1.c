/**
 *  \file prob1.c (implementation file)
 *
 *  \brief Problem name: First CLE Project - Problem 1.
 *
 *  Main thread of problem 1.
 *
 *  \author Francisco Gonçalves Tiago Lucas - April 2020
 */

#include <pthread.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include "probConst.h"
#include "sharedRegion.h"


/** \brief workerThread life cycle routine */
static void *processText (void *id);

/** \brief Result creation and storage */
void process(unsigned char*, CONTROLINFO*);

/** \brief worker threads return status array */
int statusWorkers[NUMB_THREADS];

/** \brief worker threads response */
int *status_p;

/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the worker threads and waiting for their termination.
 */

int main (int argc, char *argv[]) {

   if(argc < 2) {
      printf("Please insert text files to be processed as arguments!");
      exit(EXIT_FAILURE);
   } else {
        double t0, t1;
        int i;
      
        unsigned int worker_threads[NUMB_THREADS];
        pthread_t threads_id[NUMB_THREADS];
        
        for (i = 0; i < NUMB_THREADS; i++)
            worker_threads[i] = i;

        t0 = ((double) clock ()) / CLOCKS_PER_SEC;
        presentDataFileNames(argv + 1, --argc);

        for (i = 0; i < NUMB_THREADS; i++)
            if (pthread_create (&threads_id[i], NULL, processText, &worker_threads[i]) != 0){ 
                perror ("error on creating worker threads");
                exit (EXIT_FAILURE);
            }
        
        for (i = 0; i < NUMB_THREADS; i++)
            if (pthread_join (threads_id[i], (void *) &status_p) != 0){ 
                perror ("error on joining");
                exit (EXIT_FAILURE);
            }
      
      printResults();

      t1 = ((double) clock ()) / CLOCKS_PER_SEC;
      printf ("\nElapsed time = %.6f s\n", t1 - t0);
      exit (EXIT_SUCCESS);
   }
   
}

static void *processText(void *threadId) {

   unsigned int id = *((unsigned int *) threadId);
   unsigned char dataToBeProcessed[K+1];
   CONTROLINFO ci = {0};
   while (getAPieceOfData (id, dataToBeProcessed, &ci))
   {
        process(dataToBeProcessed, &ci);
        savePartialResults (id, &ci);
   }
   //printf("left - %i\n", id);
   statusWorkers[id] = EXIT_SUCCESS;
   pthread_exit (&statusWorkers[id]);
}

void process(unsigned char *dataToBeProcessed, CONTROLINFO *ci) {
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
        } else if (inWord && (skip == 0 && isValidStopCharacter(cha) == 1) || (inWord && skip == 2 && isValidStopCharacter(cha) == 3) ) {
            ci->bidi[nVowels][nCharacters - 1]++;
            ci->numbWords++;
            if (nCharacters > maxWordLength)
            	maxWordLength = nCharacters;
            //printf("Chars: %lu, Vowels: %lu\n",nCharacters,nVowels);
            nCharacters = 0;
            nVowels = 0;
            inWord = false;
        }
    }
    
    if (maxWordLength > ci->maxWordLength)
		ci->maxWordLength = maxWordLength;
}
