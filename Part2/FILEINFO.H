#ifndef FILEINFO_H
#define FILEINFO_H

#include <stdlib.h>
#include <stdbool.h>
#include "probConst.h"

typedef struct
{
   size_t filePosition;
   size_t numbSamples;
   size_t rxyIndex;
} FILEINFO;

#endif /* end of include guard: CONTROLINFO_H */
