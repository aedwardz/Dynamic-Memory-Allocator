#include "helpers.h"
#include "debug.h"
#include "icsmm.h"
int minimumBlockSize(size_t size){
    while(size % 16 != 0){
        size++;
    }
    return 16+size;
}

void freeRemoval(ics_free_header *header,ics_bucket * buckets){
    for (int i = 0; i< 5; i++){
        if (buckets[i].max_size >= header->header.block_size){
                for (ics_free_header * temp = buckets[i].freelist_head; temp != NULL; temp=temp->next){
                    if ((void *)temp == (void *)header){
                        if (header->prev == NULL && header->next == NULL){
                            buckets[i].freelist_head = NULL;
                            header->prev = NULL;
                            header->next = NULL;
                            return;
                        }
                        else if (header->prev == NULL){                            
                            temp = header->next;
                            buckets[i].freelist_head = temp;
                            temp->prev = NULL;
                            header->next = NULL;
                            header->prev = NULL;
                            return;
                        }
                        else if (header->next == NULL){
                            header->prev->next = NULL;
                            header->prev = NULL;
                            return;
                        }
                        else{                            
                            header->prev->next = header->next;
                            header->next->prev = header->prev;
                            header->next = NULL;
                            header->prev = NULL;
                            return;
                        }
                    }

                }
            }

        }
    }



void * getFooterAddress(ics_free_header * block){
    return (void *)block + block->header.block_size - 8;
}

ics_free_header * firstFitSelection(size_t size, ics_free_header* list, ics_bucket * bucketList, int index){
    ics_free_header* prev = NULL; 
    for (ics_free_header * temp = list; temp!= NULL; temp= temp->next){
        if (temp->header.block_size >= size){
            if (prev == NULL){
                // list = NULL;
                bucketList[index].freelist_head = NULL;
                temp->next = NULL;
                temp->prev = NULL;
                return temp;
            }
            else if (temp->next == NULL){
                prev->next = NULL;
                temp->prev = NULL;
                return temp;
            }
            else{
                prev->next = temp->next;
                temp->next->prev = prev;
                temp->next = NULL;
                temp->prev = NULL;
                return temp;
            }
        }
        prev = temp;
    }
    return NULL;
}

/* Helper function definitions go here */
ics_free_header * bucketCheck(size_t size, ics_bucket * bucketList){
    for (int i = 0; i < 5; i++){
        ics_bucket * currentBucket = bucketList;
        if (currentBucket->max_size >= size){
            if (currentBucket->freelist_head == NULL){
                bucketList++;
                continue;
            }
            else{
                if (currentBucket->freelist_head->header.block_size >= size){
                    ics_free_header * blk = currentBucket->freelist_head;
                    currentBucket->freelist_head = blk->next;
                    if(currentBucket->freelist_head !=NULL){currentBucket->freelist_head->prev = NULL;}
                    blk->prev = NULL;
                    blk->next = NULL;
                    return blk;
                    }
                ics_free_header *block = firstFitSelection(size, currentBucket->freelist_head, bucketList,i);
                if (block == NULL){
                    bucketList++;
                    continue;
                }
                return block;
            }
        }

        bucketList++;
    }
    return NULL;
}

void insertBucket(ics_free_header* header, ics_bucket* bucketList){
    for (int i = 0; i <5; i++){
        ics_bucket * currentBucket = bucketList;
      
        if (currentBucket->max_size >= header->header.block_size){
            // printf("%d\n",header->header.block_size );
            if (currentBucket->freelist_head == NULL){
                currentBucket->freelist_head = header;
                // printf("block size inserted: %d\n",currentBucket->freelist_head->header.block_size);
                return;
            }
            else{

                header->next = currentBucket->freelist_head;
                header->next->prev = header;
                header->prev = NULL;
                currentBucket->freelist_head = header;
                return;
            }
        }
        bucketList++;
    }
}