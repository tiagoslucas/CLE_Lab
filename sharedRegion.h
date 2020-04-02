/**
 *  \file sharedRegion.c (implementation file)
 *
 *  \brief Problem name: Problem 1.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the producers / consumers:
 *     \li putVal
 *     \li getVal.
 *
 *  \author António Rui Borges - March 2019
 */

#ifndef SHAREDREGION_H
#define SHAREDREGION_H

#include "controlInfo.h"
#include <stdbool.h>
/**
 *  \brief Store a value in the data transfer region.
 *
 *  Operation carried out by the producers.
 *
 *  \param prodId producer identification
 *  \param val value to be stored
 */

extern void presentDataFileNames(char *listOfFiles[], unsigned int size);


extern bool getAPieceOfData(unsigned int workerId, char dataToBeProcessed[], controlInfo *ci);

/**
 *  \brief Get a value from the data transfer region.
 *
 *  Operation carried out by the consumers.
 *
 *  \param consId consumer identification
 *
 *  \return value
 */

extern void savePartialResults(unsigned int workerId, controlInfo ci);

/**
 *  \brief Print the results of each file.
 *
 *  Operation carried out by main.
 *
 */

extern void presentResults();

#endif /* SHAREDREGION_H */

