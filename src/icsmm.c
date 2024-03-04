#include "icsmm.h"
#include "debug.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * This is your implementation of malloc. It acquires uninitialized memory from  
 * ics_inc_brk() that is 16-byte aligned, as needed.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return If successful, the pointer to a valid region of memory of at least the
 * requested size is returned. Otherwise, NULL is returned and errno is set to 
 * ENOMEM - representing failure to allocate space for the request.
 * 
 * If size is 0, then NULL is returned and errno is set to EINVAL - representing
 * an invalid request.
 */
int pages = 0;
void * currentpg;
void * currentBlock;
void * heapStart;

void *ics_malloc(size_t size) {

    
    if (size == 0){
        errno = ENOMEM;
        return NULL;
    }
    ics_bucket * buckets = seg_buckets;
    //initialize memory if there are no pages on the heap
    if (pages == 0){
        //set prologue

        void * page = ics_inc_brk(); //this is the starting address of the page
        heapStart = page;
        pages++;
        currentpg = page;

        ics_footer *prologue = (ics_footer *)page;

        //setting the prologue
        prologue->block_size = 0;
        prologue->requested_size = 0;
        prologue->block_size |= 1;
        prologue->fid = FOOTER_MAGIC;

        // printf("prologue:\n");
        // ics_header_print(prologue);

        //setting the epilogue
        void * epilogueAddress = ics_get_brk()-8;
        ics_header * epilogue = (ics_header*) epilogueAddress;
        epilogue->block_size = 0;
        epilogue->block_size |= 1;
        epilogue->hid = 0x100decafbee5UL;

        //create the first free block and the footer
        void * freeBlockAddy = page + 8;
        ics_free_header * freeBlock = (ics_free_header*)freeBlockAddy;
        freeBlock->header.block_size = 4096 - 16;
        freeBlock->next = NULL;
        freeBlock->prev = NULL;
        
        
    
        
        //footer
        void * footerAddy = epilogueAddress - 8;
        ics_footer * footer = (ics_footer*)footerAddy;
        footer->fid = 0x0a011dabUL;
        footer->block_size = 4096-16;
        footer->requested_size = 0;

        //insert the freeblock into the buckets
        insertBucket(freeBlock, buckets);
        //ics_buckets_print();
    }

    int sizewPadding = size;
    while (sizewPadding % 16 != 0){
        sizewPadding++;
    }

    int blockSize = 16 + sizewPadding;
    while(1){
    //check for a block that satisfies the size
    ics_free_header * block = bucketCheck(blockSize, buckets);
    

    // if (block){
    //     printf("original block:\n");
    //     ics_header_print(block);
    // }

    // printf("after grabbing the free block\n");
    // ics_buckets_print();

    if (block == NULL){ //ask for extra space and coalesce
        //get the block before the epilogue
        ics_footer * lfbPrologue = ics_get_brk() - 16;
        ics_free_header * lfbHeader = (void *)lfbPrologue - lfbPrologue->block_size +8;

        // printf("lfbHEADER: (\n");
        // ics_header_print(lfbHeader);
        // printf(")\n");

        //get remaining space by doing size - block size of that freeblock
        int requiredSpace = minimumBlockSize(size) - lfbHeader->header.block_size;
        //increment pages accordingly
        int space = requiredSpace;
        int pageSize = 0;
        while (requiredSpace > 0){
            ics_inc_brk();
            pages++;
            pageSize += 4096;
            requiredSpace -= 4096;
        }
        int newFreeBlockSize = lfbHeader->header.block_size + pageSize;

        // printf("block size of lfb: %d\n", lfbHeader->header.block_size);
        // printf("Size of new pages: %d\n", pageSize);
        // printf("Block size of the new free block: %d\n", newFreeBlockSize);
        // printf("Pages incremented: %d\n", pages);
        
        freeRemoval(lfbHeader, buckets);
        //set epilogue to be brk - 8
        ics_header * newEpilogue = ics_get_brk() - 8;
        newEpilogue->block_size = 0;
        newEpilogue->block_size |= 1;

        //set footer to brk - 16
        ics_footer * newFooter =  ics_get_brk() - 16;
        newFooter->block_size = newFreeBlockSize;
        newFooter->requested_size = 0;
        newFooter->fid = FOOTER_MAGIC;
        // printf("expected block size: %ld\n",(void *)newEpilogue - (void*)lfbHeader);

        //remove the original block from the bucket list
        

        lfbHeader->header.block_size = newFreeBlockSize;
        // ics_header_print(lfbHeader);

        // printf("Expected address of the footer: %p\n",ics_get_brk()-16);
        // printf("Actual address of footer: %p\n", (void *)newFooter);
        // printf("Address of Brk pointer: %p\n", ics_get_brk());


        // ics_header_print(newFooter);
        //check if the block below the prologue is allocated
        ics_footer * blockBelow = (void *)lfbHeader - 8;

        // printf("Start of the heap: %p\n", heapStart);
        // printf("below:\n");
            //if allocated, dont worry about it


            if (blockBelow->block_size % 2 == 1){
                // printf("No need to coalesce");
                // printf("newfree\n");
                // ics_header_print(lfbHeader);
                insertBucket(lfbHeader, buckets);
                continue;
            }
            //if free, then coalesce
            else{
                // printf("||||||||||COALESCING|||||||||||\n");
                ics_free_header * headerBelow = (void *)lfbHeader - blockBelow->block_size;
                int newsize =  headerBelow->header.block_size +lfbHeader->header.block_size - 16;
                headerBelow->header.block_size = newsize;
                newFooter->block_size = newsize;

                // printf("COALESCED BLOCK BROOOOO:\n");
                // ics_header_print(headerBelow);

                insertBucket(headerBelow, buckets);
                continue;
            }
    }
    int diff = block->header.block_size - blockSize;
    // printf("block size: %d\n", blockSize);
    // if (size == 13 || size == 16){
    //     printf("block chosen:\n");
    //     ics_header_print(block);
    //     if (size == 13){
    //         ics_buckets_print();
    //     }
    // }
    // printf("difference: %d\n", diff);
    if (diff >= 32){
        // subtract the minimum allocated block from the block size of the total block,

            //header and footer of the new allocated block should be that minimum size
            // printf("splitting\n");
            block->header.block_size = blockSize;
            block->header.hid = HEADER_MAGIC;
            block->header.block_size |= 1;


            ics_footer * blockFooter = getFooterAddress(block) -1;//holdupppppppp
            blockFooter->requested_size = size;
            blockFooter->fid = FOOTER_MAGIC;
            blockFooter->block_size = blockSize;
            blockFooter->block_size |= 1;
            // ics_header_print(block);
            void * blockFooterAddy = (void *) blockFooter;
            // printf("Footer address: %p\n", blockFooterAddy);

            //create header and footer for the new free block and set it to the subtraction made earlier
            ics_free_header * freeHeader = blockFooterAddy + 8;
            freeHeader->header.block_size = diff;
            freeHeader->next = NULL;
            freeHeader->prev = NULL;
            freeHeader->header.hid = HEADER_MAGIC;

            ics_footer * freeFooter = (void * )freeHeader + diff - 8;
            freeFooter->fid = FOOTER_MAGIC;
            freeFooter->block_size = diff;
            freeFooter->requested_size = 0;
            // printf("This is the free Block:\n");
            // ics_header_print(freeHeader);
            insertBucket(freeHeader, buckets);
            // printf("buckets after free insert:\n");
            // ics_buckets_print();

            // printf("block allocated: \n");
            // ics_header_print(block);
            // printf("free block:\n");
            // ics_header_print(freeHeader);
            // printf("allocated block:\n");
            // ics_header_print(block);
            return (void*)block+8;
    } 
        block->header.hid = HEADER_MAGIC;
        block->header.block_size |= 1;

        ics_footer * blockFooter = getFooterAddress(block) -1;
        blockFooter->requested_size = size;
        blockFooter->fid = FOOTER_MAGIC;
        blockFooter->block_size |= 1;
        return (void*)block +8;}
        // ics_header_print(block);


  
}

/*
 * Marks a dynamically allocated block as no longer in use and coalesces with 
 * adjacent free blocks. Adds the block to the appropriate bucket according
 * to the block placement policy.
 *
 * @param ptr Address of dynamically allocated memory returned by the function
 * ics_malloc.
 * 
 * @return 0 upon success, -1 if error and set errno accordingly.
 * 
 * If the address of the memory being freed is not valid, this function sets errno
 * to EINVAL. To determine if a ptr is not valid, (i) the header and footer are in
 * the managed  heap space, (ii) check the hid field of the ptr's header for
 * 0x100decafbee5 (iii) check the fid field of the ptr's footer for 0x0a011dab,
 * (iv) check that the block_size in the ptr's header and footer are equal, and (v) 
 * the allocated bit is set in both ptr's header and footer. 
 */
int ics_free(void *ptr) {
    // printf("FREEING\n");
    ics_bucket * buckets = seg_buckets; 
    ics_header * header1 = ptr-8;

    // printf("block to be freed: \n");
    // ics_header_print(header1);

    if (header1->block_size == 0){
        // printf("size of 0\n");
        errno = EINVAL;
        return -1;
    }
    if (header1->block_size % 2 != 1){
        // printf("Not allocated\n");
        errno = ENOMEM;
        return -1;
    }
    ics_free_header * header = (ics_free_header *)header1; 

    
    ics_footer * footer = (void * )header + header->header.block_size - 8-1;
    if (footer->block_size != header->header.block_size){
        // printf("SIZES DONT MATCH \n");
        // printf("Header size: %d\nFooter Size: %d\n", header->header.block_size, footer->block_size);
        // printf("Footer Address: %p\n", (void * )footer);
        // ics_header_print(header);
        errno = EINVAL;
        return -1;
    }
    if (header->header.hid != HEADER_MAGIC || footer->fid != FOOTER_MAGIC){
        // printf("Missing IDs\n");
        errno = EINVAL;
        return -1;
    }
    header->header.block_size &= ~1;

    // printf("VALID block\n");

    footer->block_size &= ~1;
    // printf("after allocated bit: \n");
    // ics_header_print(header);
    //check for coalescing
        //above
        ics_free_header * headerAbove = (void * ) footer + 8;
        if (headerAbove->header.block_size % 2 == 0){
            // printf("Before removal:\n");
            // ics_buckets_print();

            // printf("TO BE REMOVED: \n");
            // ics_header_print(headerAbove);

            freeRemoval(headerAbove, buckets);

            // printf("after removal:\n ");
            // ics_buckets_print();
            // printf("coalescing up\n");

            // ics_header_print(header);

            // printf("block above:\n");
            // ics_header_print(headerAbove);
            //remove above from the bucket
            
            //coalesce
            header->header.block_size = header->header.block_size + headerAbove->header.block_size;

            //get the footer of the header above
            ics_footer * footerAbove =(void*)headerAbove+ headerAbove->header.block_size-8;
            footerAbove->block_size = header->header.block_size;
            footer = footerAbove;
            // printf("After Upward coalesce: \n");
            // ics_header_print(header);

        }

    
        //below
        ics_footer * footerBelow = (void*) header - 8;
        if (footerBelow->block_size %2 == 0 ){
            
            // printf("coalescing down\n");
        
            //remove below from bucket
            ics_free_header * headerBelow = (void *) header - footerBelow->block_size;
            // printf("Heap start: %p\n", heapStart);
            // printf("footer below:\n");
            // ics_header_print(footerBelow);
            // printf("Before removal:\n");
            // ics_buckets_print();
            freeRemoval(headerBelow, buckets);
            // printf("after removal:\n ");
            // ics_buckets_print();


            
            headerBelow->header.block_size = headerBelow->header.block_size + header->header.block_size;
            footer->block_size = headerBelow->header.block_size;
            header = headerBelow;
            // printf("After downward coalesce: \n");
            // ics_header_print(header);
        }

        // printf("Block to be inserted:\n");
        // ics_header_print(header);
        // printf("inserting...\n");
        insertBucket(header, buckets);

    
        return 0;
}


