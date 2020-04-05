#include <stdlib.h>

#ifndef CONTROLINFO_H
#define CONTROLINFO_H

typedef struct
{
   size_t filePosition;
   size_t numbBytes;
   size_t numbWords;
   size_t maxWordLength;
   int bidi[50][50];
}CONTROLINFO;

#endif /* end of include guard: CONTROLINFO_H */
