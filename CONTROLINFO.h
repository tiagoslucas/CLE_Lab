#ifndef CONTROLINFO_H
#define CONTROLINFO_H

typedef struct
{
   size_t filePointer;
   size_t numbBytes;
   size_t numbWords;
   int bidi[50][50];
}CONTROLINFO;

#endif /* end of include guard: CONTROLINFO_H */