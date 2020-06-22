/**
 *  \file FILEINFO.h (interface file)
 *
 *  \brief Problem name: Problem 2.
 *
 *  File with the data shared accross all threads, referencing file stream.
 *
 *  \author Francisco Gonçalves Tiago Lucas - June 2020
 */
 
#ifndef FILEINFO_H
#define FILEINFO_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
   bool read;
   size_t filePosition;
   size_t numbSamples;
   size_t rxyIndex;
   double* result;
   double* expected;
} FILEINFO;

#endif /* end of include guard: CONTROLINFO_H */