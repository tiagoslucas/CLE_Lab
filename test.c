#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <sys/types.h>
using namespace std;

#include "probConst.h"
#include "sharedRegion.h"


/** \brief producer life cycle routine */
static void *processText (void *id);


/** \brief worker threads return status array */
int statusWorker[NUMB_THREADS];

wint_t validChar(wint_t character){
   switch (character)
   {
      case L'á':
      case L'à':
      case L'ã':
      case L'â':
         return L'a';
      case L'Á':
      case L'À':
      case L'Ã':
      case L'Â':
         return L'A';
      case L'é':
      case L'è':
      case L'ẽ':
      case L'ê':
         return L'e';
      case L'É':
      case L'È':
      case L'Ẽ':
      case L'Ê':
         return L'E';
      case L'í':
      case L'ì':
      case L'ĩ':
      case L'î':
         return L'i';
      case L'Í':
      case L'Ì':
      case L'Ĩ':
      case L'Î':
         return L'I';
      case L'ó':
      case L'ò':
      case L'õ':
      case L'ô':
         return L'o';
      case L'Ó':
      case L'Ò':
      case L'Õ':
      case L'Ô':
         return L'O';
      case L'ú':
      case L'ù':
      case L'ũ':
      case L'û':
         return L'u';
      case L'Ú':
      case L'Ù':
      case L'Ũ':
      case L'Û':
         return L'U';
      default:
         return character;
   }
}


void process(wint_t* dataToBeProcessed, unsigned int size){

   char *locale = setlocale(LC_ALL, "");

   size_t wordlength = 0;
   size_t maxwordlength = 0;
   size_t vowels[5] = { 0 };
   size_t *wordlengths = (size_t *) malloc(maxwordlength * sizeof(size_t));
   
   int i;

   for(i = 0; i < size; i++)
   //while ((character = getwc(file)) != WEOF)
   {
      wint_t character = dataToBeProcessed[i];
      character = validChar(character);

      switch (character)
      {
         case L'a':
         case L'A':
            ++vowels[0];
            break;
         case L'e':
         case L'E':
            ++vowels[1];
            break;
         case L'i':
         case L'I':
            ++vowels[2];
            break;
         case L'o':
         case L'O':
            ++vowels[3];
            break;
         case L'u':
         case L'U':
            ++vowels[4];
            break;
         default:
            break;
      }

      if (character >= L'A' && character <= L'Z' || character >= L'a' && character <= L'z')
         ++wordlength;
      else if (wordlength)
      {
         if (wordlength > maxwordlength)
            wordlengths = (size_t *) realloc(wordlengths, (maxwordlength = wordlength) * sizeof(size_t));
            ++wordlengths[wordlength - 1];
            wordlength = 0;
         }
   }

   if (wordlength)
   {
      if (wordlength > maxwordlength)
         wordlengths = (size_t *) realloc(wordlengths, (maxwordlength = wordlength) * sizeof(size_t));
      
      ++wordlengths[wordlength - 1];
      wordlength = 0;
   }
}


void printResults(){
}


/**
 *  \brief Main thread.
 *
 *  Its role is starting the simulation by generating the worker threads and waiting for their termination.
 */

int main (int argc, char** argv) {

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
      presentDataFileNames(argv + 1, argc--);

      //srandom ((unsigned int) getpid());

      for (i = 0; i < NUMB_THREADS; i++)
         if (pthread_create (&threads_id[i], NULL, processText, &worker_threads[i]) != 0)                              /* thread producer */
         { 
            perror ("error on creating worker threads");
            exit (EXIT_FAILURE);
         }
      pthread_exit(NULL);
      
      printf ("\nFinal report\n");
      for (i = 0; i < NUMB_THREADS; i++)
      { 
      
      }
      t1 = ((double) clock ()) / CLOCKS_PER_SEC;
      printf ("\nElapsed time = %.6f s\n", t1 - t0);
      exit (EXIT_SUCCESS);
   }
   
}

static void *processText(void *threadId) {

   unsigned int id = *((unsigned int *) threadId);
   char dataToBeProcessed[K];
   controlInfo ci = {};
   
   while (getAPieceOfData (id, dataToBeProcessed, &ci))
   { 
      /* realizar o seu processamento process(dataToBeProcessed);*/
      savePartialResults (id, ci);
   }

   statusWorker[id] = EXIT_SUCCESS;
   pthread_exit (&statusWorker[id]);

}

