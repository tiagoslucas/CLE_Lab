/**
 *  \file prob2.c (implementation file)
 *
 *  \brief Problem name: First CLE Project - Problem 2.
 *
 *  Main thread of problem 2.
 *
 *  \author Francisco Gonçalves Tiago Lucas - April 2020
 */

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
static void *process (void *id);

/** \brief Result creation and storage */
void circularCrossCorrelation(double*, double*, CONTROLINFO*);

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

   if(argc < 2)
   {
      printf("Please insert text files to be processed as arguments!");
      exit(EXIT_FAILURE);
   }

   else
   {
      double t0, t1;
      int rc, i;
      
      unsigned int worker_threads[NUMB_THREADS];
      pthread_t threads_id[NUMB_THREADS];


      for (i = 0; i < NUMB_THREADS; i++)
         worker_threads[i] = i;

      t0 = ((double) clock ()) / CLOCKS_PER_SEC;
      presentDataFileNames(argv + 1, --argc);

      for (i = 0; i < NUMB_THREADS; i++)
         if (pthread_create (&threads_id[i], NULL, process, &worker_threads[i]) != 0){ 
            perror ("error on creating worker threads");
            exit (EXIT_FAILURE);
         }
        
      for (i = 0; i < NUMB_THREADS; i++)
         if (pthread_join (threads_id[i], (void *)&status_p) != 0){ 
            perror ("error on joining");
            exit (EXIT_FAILURE);
         }

      
      printf ("\nFinal report\n");
      printResults();

      t1 = ((double) clock ()) / CLOCKS_PER_SEC;
      printf ("\nElapsed time = %.6f s\n", t1 - t0);
      exit (EXIT_SUCCESS);
   }
   
}

static void *process(void *threadId) {

   unsigned int id = *((unsigned int *) threadId);
   double x[DEFAULT_SIZE_SIGNAL];
   double y[DEFAULT_SIZE_SIGNAL];
   CONTROLINFO ci = (CONTROLINFO) {0};
   ci.filePosition = -1;
   ci.processing = false;
   while (getAPieceOfData (id, x, y, &ci))
   { 
      circularCrossCorrelation(x, y, &ci);
      savePartialResults (id, &ci);
   }

   statusWorkers[id] = EXIT_SUCCESS;
   pthread_exit (&statusWorkers[id]);

}

void circularCrossCorrelation(double *x, double *y, CONTROLINFO *ci) {

   size_t i, j;
   size_t n = ci->numbSamples;
   size_t temp = ci->rxyIndex; 

   for(j = 0; j < n; j++){
      ci->result += x[j] * y[(temp+j)%n];
   }
}
