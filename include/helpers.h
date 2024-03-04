#ifndef HELPERS_H
#define HELPERS_H
#include <stdlib.h>
#include <stdint.h>
#include <icsmm.h>
#include <stdbool.h>
int minimumBlockSize(size_t size);
void freeRemoval(ics_free_header *header,ics_bucket * buckets);

void * getFooterAddress(ics_free_header * block);

ics_free_header * firstFitSelection(size_t size, ics_free_header* list, ics_bucket * bucketList, int index);

ics_free_header * bucketCheck(size_t size, ics_bucket * bucketList);

void insertBucket(ics_free_header * header, ics_bucket* bucketList);

#endif

