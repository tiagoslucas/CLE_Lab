/**
 *  \file CONTROLINFO.h (interface file)
 *
 *  \brief Problem name: Problem 1.
 *
 *  File with the data shared accross all threads.
 *
 *  \author Francisco Gonçalves Tiago Lucas - June 2020
 */
 
#ifndef CONTROLINFO_H
#define CONTROLINFO_H

#include <stdlib.h>
#include "probConst.h"

typedef struct
{
   size_t filePosition;
   size_t numbBytes;
   size_t numbWords;
   size_t maxWordLength;
   int bidi[MAX_SIZE_WORD][MAX_SIZE_WORD];
}CONTROLINFO;

#endif /* end of include guard: CONTROLINFO_H */
