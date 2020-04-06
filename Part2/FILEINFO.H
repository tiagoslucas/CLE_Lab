#ifndef FILEINFO_H
#define FILEINFO_H

#include <stdlib.h>
#include <stdbool.h>
#include "probConst.h"

typedef struct
{
   bool processed;
   size_t filePosition;
   size_t numbSamples;
   double result[DEFAULT_SIZE_SIGNAL];
   double expected[DEFAULT_SIZE_SIGNAL];
} FILEINFO;

#endif /* end of include guard: CONTROLINFO_H */