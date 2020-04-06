#ifndef CONTROLINFO_H
#define CONTROLINFO_H

#include <stdlib.h>
#include <stdbool.h>
#include "probConst.h"

typedef struct
{
   bool processing;
   size_t filePosition;
   size_t numbSamples;
   size_t rxyIndex;
   double result;
} CONTROLINFO;

#endif /* end of include guard: CONTROLINFO_H */