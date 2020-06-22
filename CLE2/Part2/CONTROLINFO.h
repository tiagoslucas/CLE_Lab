/**
 *  \file CONTROLINFO.h (interface file)
 *
 *  \brief Problem name: Problem 2.
 *
 *  File with the data shared accross all threads, referencing signal stream.
 *
 *  \author Francisco Gonçalves Tiago Lucas - June 2020
 */
 
#ifndef CONTROLINFO_H
#define CONTROLINFO_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
   bool processing;
   size_t filePosition;
   size_t numbSamples;
   size_t rxyIndex;
   double result;
} CONTROLINFO;

#endif /* end of include guard: CONTROLINFO_H */