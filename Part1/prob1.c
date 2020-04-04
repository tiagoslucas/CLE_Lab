#include <pthread.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
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

void process(unsigned char*, CONTROLINFO*);

/** \brief worker threads return status array */
int statusWorkers[NUMB_THREADS];


/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the worker threads and waiting for their termination.
 */

int main (int argc, char *argv[]) {

   if(argc < 2)
   {
      printf("Please insert text files to be processed as arguments!");
      exit(EXIT_FAILURE);
   }

   else
   {
      double t0, t1;
      int i;
      
      unsigned int worker_threads[NUMB_THREADS];
      pthread_t threads_id[NUMB_THREADS];


      for (i = 0; i < NUMB_THREADS; i++)
         worker_threads[i] = i;

      t0 = ((double) clock ()) / CLOCKS_PER_SEC;

      presentDataFileNames(argv + 1, --argc);
      //srandom ((unsigned int) getpid());

      for (i = 0; i < NUMB_THREADS; i++)
         if (pthread_create (&threads_id[i], NULL, processText, &worker_threads[i]) != 0)                              /* thread producer */
         { 
            perror ("error on creating worker threads");
            exit (EXIT_FAILURE);
         }
      pthread_exit(NULL);
      
      printf ("\nFinal report\n");
      //printResults();

      t1 = ((double) clock ()) / CLOCKS_PER_SEC;
      printf ("\nElapsed time = %.6f s\n", t1 - t0);
      exit (EXIT_SUCCESS);
   }
   
}

static void *processText(void *threadId) {

   unsigned int id = *((unsigned int *) threadId);
   unsigned char dataToBeProcessed[K+1];
   CONTROLINFO ci = (CONTROLINFO) {0, 0, 0, {0}};
   
   while (getAPieceOfData (id, dataToBeProcessed, &ci))
   { 
      process(dataToBeProcessed, &ci);
      savePartialResults (id, &ci);
   }

   statusWorkers[id] = EXIT_SUCCESS;
   pthread_exit (&statusWorkers[id]);

}

void process(unsigned char* dataToBeProccessed, CONTROLINFO *ci) {
    char cha;
    int counter = 0, size = 0, length = ci->numbBytes;

    for (int i = 0; i < length; i++) {
        if(dataToBeProccessed[i] == (char)0xC3)
            i++;
        else if(dataToBeProccessed[i] == (char)0xE2)
            i += 2;

        if (i >= length)
            return;
        
        cha = dataToBeProccessed[i];

        if (cha >= 65 && cha <= 90) {
            size++;
        } else if (cha >= 97 && cha <= 122) {
            size++;
        } else if (cha == (char)0xA7 || cha == (char)0x87) {
            size++;
        } else if (cha == (char)0xA1 || cha == (char)0xA0 || cha == (char)0xA2 || cha == (char)0xA3 || cha == (char)0x81 || cha == (char)0x80 || cha == (char)0x82 || cha == (char)0x83) {
            counter++;
            size++;
        } else if (cha == (char)0xA9 || cha == (char)0xA8 || cha == (char)0xAA || cha == (char)0x89 || cha == (char)0x88 || cha == (char)0x8A) {
            counter++;
            size++;
        } else if (cha == (char)0xAD || cha == (char)0xAC || cha == (char)0x8D || cha == (char)0x8C) {
            counter++;
            size++;
        } else if (cha == (char)0xB3 || cha == (char)0xB2 || cha == (char)0xB4 || cha == (char)0xB5 || cha == (char)0x93 || cha == (char)0x92 || cha == (char)0x94 || cha == (char)0x95){
            counter++;
            size++;
        } else if (cha == (char)0xBA || cha == (char)0xB9 || cha == (char)0x94 || cha == (char)0x99) {
            counter++;
            size++;
        }

        if (isValidStopCharacter(cha)) {
            ci->bidi[size - 1][counter]++;
            ci->numbWords++;
            counter = 0;
            size = 0;
        }
    }
}