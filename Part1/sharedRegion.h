/**
 *  \file sharedRegion.c (implementation file)
 *
 *  \brief Problem name: Problem 1.
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
 *  \brief Get data from the files.
 *
 *  Operation carried out by the worker threads.
 *
 *  \param workerId				identification
 *  \param *dataToBeProcessed	pointer to the array with the data to process.
 *  \param *ci					pointer to the shared data structure.
 *
 */
extern bool getAPieceOfData(unsigned int workerId, unsigned char* dataToBeProcessed, CONTROLINFO* ci);

/**
 *  \brief Get a value from the data transfer region.
 *
 *  Operation carried out by the main thread.
 *
 *  \param workerId identification
 *  \param *ci		pointer to the shared data structure
 *
 *  \return value
 */
extern void savePartialResults(unsigned int workerId, CONTROLINFO *ci);

/**
 *  \brief Print the results of each file.
 *
 *  Operation carried out by main thread.
 *
 */
extern void printResults(void);

/**
 *  \brief Validate if a character is a stop character.
 *
 *  Operation carried out by all threads.
 *
 * \param character		character to which the validation is done.
 *
 * \return 				0 if the character provided is not a stop character, otherwise is the number of bytes the character should have. Can be used as true/false.
 *
 */
extern int isValidStopCharacter(char character);

#endif /* SHAREDREGION_H */
