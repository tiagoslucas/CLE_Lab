/**
 *  \file sharedRegion.c (implementation file)
 *
 *  \brief Problem name: Problem 2.
 *
 *  \author Francisco Gonçalves and Tiago Lucas - April 2019
 */
#ifndef SHAREDREGION_H
#define SHAREDREGION_H

#include "CONTROLINFO.h"
#include <stdbool.h>

/**
 *  \brief Insert the names of the files to be processed in an array.
 *
 *  Operation carried out by the main thread.
 *
 *  \param listOfFiles names of files to process
 *  \param size number of text files to be processed
 */
extern void presentDataFileNames(char *listOfFiles[], unsigned int size);

/**
 *  \brief Store a value in the data transfer region.
 *
 *  Operation carried out by the worker threads.
 *
 *  \param workerId worker identification
 *  \param *x pointer to the array with first signals of the pair
 *  \param *x pointer to the array with second signals of the pair
 *  \param *ci pointer to the shared data structure
 */
extern bool getAPieceOfData(unsigned int workerId, double *x, double *y, CONTROLINFO *ci);

/**
 *  \brief Get a value from the data transfer region and save it in result data storage.
 *
 *  Operation carried out by the worker threads.
 *
 *  \param workerId worker identification
 *	\param *ci pointer to the shared data structure
 *
 *  \return value
 */
extern void savePartialResults(unsigned int workerId, CONTROLINFO *ci);

/**
 *  \brief Print all the results stored in result data storage.
 *
 *  Operation carried out by the main thread.
 *
 */
extern void printResults(void);

#endif /* SHAREDREGION_H */
