#include <stdlib.h>

#ifndef CONTROLINFO_H
#define CONTROLINFO_H

typedef struct
{
   size_t filePointer;
   size_t signalSize;
   size_t rxyIndex;
   double result[];
}CONTROLINFO;

#endif /* end of include guard: CONTROLINFO_H */