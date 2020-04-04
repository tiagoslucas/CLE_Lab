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

void circularCrossCorrelation(double*, double*, CONTROLINFO*);

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
      int rc, i;
      
      unsigned int worker_threads[NUMB_THREADS];
      pthread_t threads_id[NUMB_THREADS];


      for (i = 0; i < NUMB_THREADS; i++)
         worker_threads[i] = i;

      t0 = ((double) clock ()) / CLOCKS_PER_SEC;
      presentDataFileNames(argv + 1, --argc);

      //srandom ((unsigned int) getpid());

      for (i = 0; i < NUMB_THREADS; i++)
         if (pthread_create (&threads_id[i], NULL, process, &worker_threads[i]) != 0)                              /* thread producer */
         { 
            perror ("error on creating worker threads");
            exit (EXIT_FAILURE);
         }
      pthread_exit(NULL);
      
      printf ("\nFinal report\n");
      printResults();

      t1 = ((double) clock ()) / CLOCKS_PER_SEC;
      printf ("\nElapsed time = %.6f s\n", t1 - t0);
      exit (EXIT_SUCCESS);
   }
   
}

static void *process(void *threadId) {

   unsigned int id = *((unsigned int *) threadId);
   double *x;
   double *y;
   CONTROLINFO ci = (CONTROLINFO) {0};
   
   while (getAPieceOfData (id, x, y, &ci))
   { 
      //circularCrossCorrelation(x, y, &ci);
      savePartialResults (id, &ci);
   }

   statusWorkers[id] = EXIT_SUCCESS;
   pthread_exit (&statusWorkers[id]);

}

void circularCrossCorrelation(double *x, double *y, CONTROLINFO *ci) {

   int i;
   int length = sizeof(x) / sizeof(x[0]);
   for (i = 0; i < length; i++)
   {
      ci->result[i] = x[i] * y[(2*i)%length];
   }

   //int numbBytestoCirculate = 2;
   //y = (x << numbBytestoCirculate) | (x >> (8 - numbBytestoCirculate));
}