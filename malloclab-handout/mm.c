/*
 Name:Krutika Kamilla
 Andrew ID:kkamilla
 ********************** Malloc implementation ******************
 * This program implements dynamic memory allocator. It uses segregated
 * free list and first fit algorithms to manage and search free blocks.
 *
 * Header & Footer: Free blocks have header and footer. Both are
 * identical. Footer is used during coalescing.It indicates size
 * and allocation status of each block.
 *
 * Allocation: Free block is searched starting from minimum size list.
 * If block is not found then heap is extended.
 *
 * Freeing of block: Block is freed on request by adding to appropriate
 * free list and doing coalescing (combining with other free blocks) to
 * avoid external fragmentation. Headers and footers are also updated.
 
 * Free list Implementation: It uses segregated list approach which
 * uses multiple Linked lists. Lists are used to store
 * different sized blocks. Each list has maximum size limit as
 * defined in LIST#MAX variables (where # is list number). Last list
 * includes all the maximum size blocks.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "contracts.h"

#include "mm.h"
#include "memlib.h"

#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */

/* Extend heap by this amount (bytes) */
#define CHUNKSIZE  (1<<8)  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char*)(bp) - WSIZE)
#define FTRP(bp)    ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and prev blocks */
#define NEXT_BLKP(bp)   ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))
#define PREV_BLKP(bp)   ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))
#define GET_PREV_ALLOC(p)   (GET(p) & 0x2)

/* Given block ptr bp, compute address of next and prev free blocks */
#define NEXT_FREE(bp)   ((char*) ((char*)(bp)))
#define PREV_FREE(bp)   ((char*) ((char*)(bp) + DSIZE))

/* Read and write double word at address p */
#define GETDW(p)      (*(size_t *) (p))
#define PUTDW(p, val) (*(size_t *) (p) = (val))

/* Number of segregated lists */
#define LISTCT  13

/* rounds up to the nearest multiple of 8 */
#define ALIGN(p) (((size_t)(p) + 7) & ~0x7)

static char *heap_listp; /* Pointer used to start lists on heap */
static char *heap_start; /* Pointer used to point to the first block */


void *extend_heap(size_t words);
void *coalesce(void *bp);
void *find_fit(size_t asize);
void place(void *bp, size_t asize);
void addfree(char *bp, size_t asize);
void delfree(char *bp, size_t asize);
void *find_block(size_t listno, size_t asize);

// Create aliases for driver tests
// DO NOT CHANGE THE FOLLOWING!
#ifdef DRIVER
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif
/*
 *  Logging Functions
 *  -----------------
 *  - dbg_printf acts like printf, but will not be run in a release build.
 *  - checkheap acts like mm_checkheap, but prints the line it failed on and
 *    exits if it fails.
 */
#ifndef NDEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#define checkheap(verbose) do {if (mm_checkheap(verbose)) {  \
printf("Checkheap failed on line %d\n", __LINE__);\
exit(-1);  \
}}while(0)
#else
#define dbg_printf(...)
#define checkheap(...)
#endif
/* Helper function declaration */

int in_heap(const void *p) {
    /* Return whether the pointer is in the heap.*/
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

int aligned(const void *p) {
    /* Return whether the pointer is aligned.*/
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * Init: Initializes heap and pointers to free list
 * also creates prologue and epilogue blocks.
 * returns -1 on error, 0 on success.
 */
int mm_init(void) {
    /* Create initial empty heap */
    if ((heap_listp = mem_sbrk(LISTCT*DSIZE)) == NULL)
        return -1;
    /* Segregations lists pointers 
        initialization with null pointers*/
    PUTDW(heap_listp , (size_t) NULL);
    PUTDW(heap_listp + 8, (size_t) NULL);
    PUTDW(heap_listp + 16, (size_t) NULL);
    PUTDW(heap_listp + 24, (size_t) NULL);
    PUTDW(heap_listp + 32, (size_t) NULL);
    PUTDW(heap_listp + 40, (size_t) NULL);
    PUTDW(heap_listp + 48, (size_t) NULL);
    PUTDW(heap_listp + 56, (size_t) NULL);
    PUTDW(heap_listp + 64, (size_t) NULL);
    PUTDW(heap_listp + 72, (size_t) NULL);
    PUTDW(heap_listp + 80, (size_t) NULL);
    PUTDW(heap_listp + 88, (size_t) NULL);
    PUTDW(heap_listp + 96, (size_t) NULL);
    /* Extend heap for Epilogue & Prologue */
    if ((heap_start = mem_sbrk(4*WSIZE)) == NULL)
        return -1;
    PUT(heap_start, 0);
    /* Prologue header */
    PUT(heap_start + (1*WSIZE), PACK(DSIZE, 1));
    /* Prologue footer */
    PUT(heap_start + (2*WSIZE), PACK(DSIZE, 1));
    /* Epilogue header */
    PUT(heap_start + (3*WSIZE), PACK(0, 3));
    
    /* Puting CHUNKSIZE to heap */
    if (extend_heap(CHUNKSIZE) == NULL)
            return -1;
    return 0;
}

/*
 * Find_fit: Search for requested free block through all lists
 */
 void *find_fit(size_t asize) {
    size_t listno; 
    char *bp = NULL; 
    
    /* Searching for first fit through all lists, starting with 
        minimum size list*/
    if (asize <= 24) {
        for (listno = 0; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 48) {
        for (listno = 1; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 72) {
        for (listno = 2; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 96) {
        for (listno = 3; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 120) {
        for (listno = 4; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 480) {
        for (listno = 5; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 960) {
        for (listno = 6; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 1920) {
        for (listno = 7; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 3840) {
        for (listno = 8; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 7680) {
        for (listno = 9; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= 15360) {
        for (listno = 10; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    }else  if (asize <= 30720) {
        for (listno = 11; listno < LISTCT; listno++) {
            if ((bp = find_block(listno, asize)) != NULL)
                return bp;
        }
    } 
    else {
        listno = 12;
        if ((bp = find_block(listno, asize)) != NULL)
            return bp;
    }
    return bp;
}


/*
 * Delfree: Deletes free block from the free list
 */
 void delfree(char *bp, size_t size) {
    /* If free block to be removed is head of list */
    if ((char *) GETDW(PREV_FREE(bp)) == NULL &&
     (char *) GETDW(NEXT_FREE(bp)) != NULL) {
        /* Update head of list */
        if (size <= 24) {
            PUTDW(heap_listp, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 48) {
            PUTDW(heap_listp + 8, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 72) {
            PUTDW(heap_listp + 16, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 96) {
            PUTDW(heap_listp + 24, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 120) {
            PUTDW(heap_listp + 32, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 480) {
            PUTDW(heap_listp + 40, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 960) {
            PUTDW(heap_listp + 48, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 1920) {
            PUTDW(heap_listp + 56, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 3840) {
            PUTDW(heap_listp + 64, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 7680) {
            PUTDW(heap_listp + 72, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        } else if (size <= 15360) {
            PUTDW(heap_listp + 80, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        }else if (size <= 30720) {
            PUTDW(heap_listp + 88, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        }  
        else {
            PUTDW(heap_listp + 96, (size_t) (char *) GETDW(NEXT_FREE(bp)));
        }
        /* Previous block of new head will be NULL */
        PUTDW(PREV_FREE((char *) GETDW(NEXT_FREE(bp))), (size_t) NULL);
        
    /* If free block to be removed is only one in list */
    } else if ((char *) GETDW(PREV_FREE(bp)) == NULL && 
        (char *) GETDW(NEXT_FREE(bp)) == NULL) {
        if (size <= 24) {
            PUTDW(heap_listp , (size_t) NULL);
        } else if (size <= 48) {
            PUTDW(heap_listp + 8, (size_t) NULL);
        } else if (size <= 72) {
            PUTDW(heap_listp + 16, (size_t) NULL);
        } else if (size <= 96) {
            PUTDW(heap_listp + 24, (size_t) NULL);
        } else if (size <= 120) {
            PUTDW(heap_listp + 32, (size_t) NULL);
        } else if (size <= 480) {
            PUTDW(heap_listp + 40, (size_t) NULL);
        } else if (size <= 960) {
            PUTDW(heap_listp + 48, (size_t) NULL);
        } else if (size <= 1920) {
            PUTDW(heap_listp + 56, (size_t) NULL);
        } else if (size <= 3840) {
            PUTDW(heap_listp + 64, (size_t) NULL);
        } else if (size <= 7680) {
            PUTDW(heap_listp + 72, (size_t) NULL);
        } else if (size <= 15360) {
            PUTDW(heap_listp + 80, (size_t) NULL);
        }else if (size <= 30720) {
            PUTDW(heap_listp + 88, (size_t) NULL);
        } 
         else {
            PUTDW(heap_listp + 96, (size_t) NULL);
        }
    
    /* If free block to be removed is only one in list */
    } else if ((char *) GETDW(PREV_FREE(bp)) != NULL &&
     (char *) GETDW(NEXT_FREE(bp)) == NULL) {
        /* Update previous block's next pointer to NULL */
        PUTDW(NEXT_FREE((char *) GETDW(PREV_FREE(bp))), (size_t) NULL);
        
    /* If free block to be removed is in between 
        of two free blocks */
    } else {
        /* Update previous of next free block */
        PUTDW(PREV_FREE((char *) GETDW(NEXT_FREE(bp))), 
            (size_t) (char *) GETDW(PREV_FREE(bp)));
        /* Update next of previous free block */
        PUTDW(NEXT_FREE((char *) GETDW(PREV_FREE(bp))), 
            (size_t) (char *) GETDW(NEXT_FREE(bp)));
    }
}

/*
 * Coalesce: Combines next, previous or both free block
 * with current free block
 */
 void *coalesce(void *bp) {
    /* Size of current, next & previous blocks */
    size_t size_current = GET_SIZE(HDRP(bp));
    size_t size_next = 0;
    size_t size_prev = 0;
    /* Get previous & next block's allocation status */
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    
    /* If prev & next both are allocated no need to combine,
        so return pointer to current block*/
    if (prev_alloc && next_alloc) {
        return bp;
        
    /* If prev is allocated & next is free */
    } else if (!prev_alloc && next_alloc) {
    /* Remove both (current & next) free blocks from list */
        delfree(bp,size_current);
        delfree(PREV_BLKP(bp),GET_SIZE(HDRP(PREV_BLKP(bp))));
    /* Size of new block is addition of
        current and previous block sizes */
        size_current += GET_SIZE(HDRP(PREV_BLKP(bp)));
    /* Update header & footer with new size */
        PUT(HDRP(PREV_BLKP(bp)),PACK(size_current,
            GET_PREV_ALLOC(HDRP(PREV_BLKP(bp)))));
        PUT(FTRP(PREV_BLKP(bp)), GET(HDRP(PREV_BLKP(bp))));
    /* add new free block to list */
        addfree(PREV_BLKP(bp),size_current);
        return PREV_BLKP(bp);
    
    /* If prev is free and next is allocated */
    } else if (prev_alloc && !next_alloc) {
        /* Remove both (current & next) free blocks from list */
        delfree(bp,size_current);
        delfree(NEXT_BLKP(bp),GET_SIZE(HDRP(NEXT_BLKP(bp))));
        /* Size of new block is addition of
        current and next block sizes */
        size_current += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        /* Update header & footer with new size */
        PUT(HDRP(bp),PACK(size_current,prev_alloc));
        PUT(FTRP(bp),PACK(size_current,prev_alloc));
        /* add new free block to list */
        addfree(bp,size_current);
        return bp;
        
    /* If previous & next both are free */
    }  else {
        size_prev = GET_SIZE(HDRP(PREV_BLKP(bp)));
        size_next = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    /* Remove all three (current, previous & next) 
        free blocks from list */
        delfree(bp,size_current);
        delfree(PREV_BLKP(bp),size_prev);
        delfree(NEXT_BLKP(bp),size_next);
    /* Size of new block is addition of
        current, previous and next block sizes */
        size_current += size_prev + size_next;
    /* Update header & footer with new size */
        PUT(HDRP(PREV_BLKP(bp)),PACK(size_current,
            GET_PREV_ALLOC(HDRP(PREV_BLKP(bp)))));
        PUT(FTRP(PREV_BLKP(bp)), GET(HDRP(PREV_BLKP(bp))));
    /* add new free block to list */
        addfree(PREV_BLKP(bp),size_current);
        return PREV_BLKP(bp);
    }
}




/*
 * Function: extend_heap 
 * Extends the heap, adds header and footer to new free block,
 * It also adds block to free list and calls coalesce.
 */
 void *extend_heap(size_t words) {
    char *bp; /* Stores block pointer */
    /* Extend heap by requested size */
    if ((long)(bp = mem_sbrk(words)) < 0)
        return NULL;
    /* Initialize the free block header */
    PUT(HDRP(bp),PACK(words,GET_PREV_ALLOC(HDRP(bp))));
    /* Initialize the free block footer */
    PUT(FTRP(bp),GET(HDRP(bp))); 
    /* add free block to segregated list */
    addfree(bp,words);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * Addfree: Finds the matching list to store the free block
 * then adds the block in front of list.
 */
 void addfree(char *bp, size_t size) {
    char *head; /* Points to head of list */
    char *start; /* Points to addres of head */
    
    /* Find appropriate list based on size */
    if (size <= 24) {
        start = heap_listp;
        head = (char *) GETDW(heap_listp);
    } else if (size <= 48) {
        start = heap_listp + 8;
        head = (char *) GETDW(heap_listp + 8);
    } else if (size <= 72) {
        start = heap_listp + 16;
        head = (char *) GETDW(heap_listp + 16);
    } else if (size <= 96) {
        start = heap_listp + 24;
        head = (char *) GETDW(heap_listp + 24);
    } else if (size <= 120) {
        start = heap_listp + 32;
        head = (char *) GETDW(heap_listp + 32);
    } else if (size <= 480) {
        start = heap_listp + 40;
        head = (char *) GETDW(heap_listp + 40);
    } else if (size <= 960) {
        start = heap_listp + 48;
        head = (char *) GETDW(heap_listp + 48);
    } else if (size <= 1920) {
        start = heap_listp + 56;
        head = (char *) GETDW(heap_listp + 56);
    } else if (size <= 3840) {
        start = heap_listp + 64;
        head = (char *) GETDW(heap_listp + 64);
    } else if (size <= 7680) {
        start = heap_listp + 72;
        head = (char *) GETDW(heap_listp + 72);
    } else if (size <= 15360) {
        start = heap_listp + 80;
        head = (char *) GETDW(heap_listp + 80);
    }else if (size <= 30720) {
        start = heap_listp + 88;
        head = (char *) GETDW(heap_listp + 88);
    }  
    else {
        start = heap_listp + 96;
        head = (char *) GETDW(heap_listp + 96);
    }

    /* If there are previous blocks in the list 
        then it is added to front*/
    if (head != NULL) {
        /* Current block will be set as head */
        PUTDW(start,(size_t) bp);
        PUTDW(PREV_FREE(bp),(size_t) NULL);
        PUTDW(NEXT_FREE(bp),(size_t) head);
        /* Set previous block's previous pointer to current */
        PUTDW(PREV_FREE(head),(size_t) bp);
        
    /* If there are no blocks in the list 
        then this is first free block */
    } else {
        /* Current block will be set as head */
        PUTDW(start,(size_t) bp);
        PUTDW(NEXT_FREE(bp),(size_t) NULL);
        PUTDW(PREV_FREE(bp),(size_t) NULL);
    }
}


/*
 * Find_block: Search for requested free block in a list
 */
 void *find_block(size_t listno, size_t asize) {
    char *bp = NULL;
    switch (listno) {
        case 0:
            bp = (char *) GETDW(heap_listp);
            break;
        case 1:
            bp = (char *) GETDW(heap_listp + 8);
            break;
        case 2:
            bp = (char *) GETDW(heap_listp + 16);
            break;
        case 3:
            bp = (char *) GETDW(heap_listp + 24);
            break;
        case 4:
            bp = (char *) GETDW(heap_listp + 32);
            break;
        case 5:
            bp = (char *) GETDW(heap_listp + 40);
            break;
        case 6:
            bp = (char *) GETDW(heap_listp + 48);
            break;
        case 7:
            bp = (char *) GETDW(heap_listp + 56);
            break;
        case 8:
            bp = (char *) GETDW(heap_listp + 64);
            break;
        case 9:
            bp = (char *) GETDW(heap_listp + 72);
            break;
        case 10:
            bp = (char *) GETDW(heap_listp + 80);
            break;
        case 11:
            bp = (char *) GETDW(heap_listp + 88);
            break;
        case 12:
            bp = (char *) GETDW(heap_listp + 96);
            break;
    }
    /* Traverse through list to find the appropriate fit */
    while (bp != NULL) {
        if (asize <= GET_SIZE(HDRP(bp))) {
                break; 
        }
        bp = (char *) GETDW(NEXT_FREE(bp));
    }
    return bp;
}

/*
 * Place: Allocates free block. 
 * It splits the block if remaining size is greater 
 * than minimum block size.
 */
 void place(void *bp, size_t asize) {
    size_t size_current = GET_SIZE(HDRP(bp));
    /* size of the remaining block */
    size_t size_r = size_current - asize;
    /* Delete free block from list */
    delfree(bp, size_current);
    
    /* Split block if remaining size is
        greater than or equal to 24*/
    if (size_r >= 3*DSIZE) {
        PUT(HDRP(bp),PACK(asize,GET_PREV_ALLOC(HDRP(bp)) | 1));
        PUT(HDRP(NEXT_BLKP(bp)), size_r | 2);
        PUT(FTRP(NEXT_BLKP(bp)), size_r | 2);
    /* Add remaining free block to list */
        addfree(NEXT_BLKP(bp), size_r);
      
    } else {
        PUT(HDRP(bp),PACK(size_current,GET_PREV_ALLOC(HDRP(bp)) | 1));
    /* Update next free block with status of previous's allocation */
        PUT(HDRP(NEXT_BLKP(bp)),GET(HDRP(NEXT_BLKP(bp))) | 2);
        if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))))
            PUT(FTRP(NEXT_BLKP(bp)), GET(HDRP(NEXT_BLKP(bp))));
    }
}


/*
 * malloc: Allocated the block of requested size on heap and
 * returns pointer to the block.
 */
void *malloc (size_t size) {
    size_t asize;
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
    
    if (size <= 0)
        return NULL;
    /* Adjust block size to include alignment */
    if (size <= 2*DSIZE)
        asize = 3*DSIZE;
    else
        asize = (size_t)ALIGN(size + WSIZE);
    
    /* Checking Heap */
    #ifdef MM_CHECKHEAP
        mm_checkheap(0);
    #endif
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp,asize); 
        return bp;
    }
    /* No fit found. Get more memory from heap and place it */
    if (asize > CHUNKSIZE)
        extendsize = asize;
    else 
        extendsize = CHUNKSIZE;
    if ((bp = extend_heap(extendsize)) == NULL)
        return NULL;
    place(bp,asize); 
    return bp;
}

/*
 * free: Frees the block pointed by ptr. Freed block is added to 
 * the appropriate list.        
 * 
 */
void free (void *ptr) {
    if(ptr == NULL) return;
    size_t size = GET_SIZE(HDRP(ptr));
    /* Update header and footer to unallocated */
    PUT(HDRP(ptr), size | GET_PREV_ALLOC(HDRP(ptr)));
    PUT(FTRP(ptr), GET(HDRP(ptr)));
    /* Update next block header*/
    PUT(HDRP(NEXT_BLKP(ptr)),
        (GET_SIZE(HDRP(NEXT_BLKP(ptr))) | GET_ALLOC(HDRP(NEXT_BLKP(ptr)))));
    /* Add free block to list */
    addfree(ptr,size);
    coalesce(ptr);
}

/*
 * realloc: It reallocates previous block with new size.
 * The old data is copied to new location and old block is freed.
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size is 0 then this is free so return NULL. */
    if(size == 0) {
        free(oldptr);
        return 0;
    }
    /* If oldptr is NULL, then this is malloc. */
    if(oldptr == NULL) {
        return malloc(size);
    }
      
    /* Else allocate new space */
      newptr = malloc(size);

    /* If realloc() fails the original block is left as it is  */
      if(!newptr) {
        return 0;
      }

    /* Copy the old data. */
      oldsize = GET_SIZE(HDRP(oldptr)) - WSIZE;
      if(size < oldsize) oldsize = size;
      memcpy(newptr, oldptr, oldsize);

      /* Free the old block. */
      free(oldptr);

      return newptr;
}

/*
 * calloc: Similar to malloc, additionally initializes memory
 * to 0 before returning pointer.
 */
void *calloc (size_t nmemb, size_t size) {
    size_t nobytes = nmemb * size;
    void *ptr = malloc(nobytes);
    memset(ptr, 0, nobytes);
    return ptr;
}

/*
 * mm_checkheap: Check the consistency of heap
 * 1. Check Prologue header
 * 2. Check Prologue footer
 * 3. Check alignment of each block
 * 4. Check that each block exist inside heap
 * 5. Check if header & footer mismatch
 * 6. Check inconsistency in next & previous pointers
 * 7. Check coalesce function
 * 8. Check Epilogue header
 */
int mm_checkheap(int verbose) {
     char *bp;
    /* Checking Prologue header */
    bp = heap_start + (1*WSIZE);
    if ((GET_SIZE(bp) != DSIZE) || !GET_ALLOC(bp))
        printf("Invalid prologue header\n");
    
    /* Checking Prologue footer */  
    bp = heap_start + (2*WSIZE);
    if ((GET_SIZE(bp) != DSIZE) || !GET_ALLOC(bp))
        printf("Invalid prologue footer\n");
    bp = heap_listp + LISTCT*DSIZE + 2*DSIZE;
    while ((GET(HDRP(bp)) != 1) && (GET(HDRP(bp)) != 3)) {
        if (verbose) {
            printf("Block pointer [%p] with size [%d],",bp, GET_SIZE(HDRP(bp)));
            if (!GET_ALLOC(HDRP(bp)))
                printf("It is free\n");
            else
                printf("It is allocated\n");
        }
    /* Checking alignment of each block */
        if (!aligned(bp))
            printf("Block with pointer %p is not aligned\n", bp);
    /* Checking if each block exist 
         inside heap (heap boundries) */
        if (!in_heap(bp)) {
            printf("Block with pointer %p does not exist in heap\n", bp);
        }
        if (!GET_ALLOC(HDRP(bp))) {
    /* Checking if header & footer mismatch */
        if ((GET(HDRP(bp)) != GET(FTRP(bp))))
            printf("Header footer mismatch for block with pointer %p\n", bp);
    /* Checking inconsistencies in next & previous pointers */
        if ((char *) GETDW(NEXT_FREE(bp)) != NULL &&
        (char *) GETDW(PREV_FREE(GETDW(NEXT_FREE(bp)))) != bp)
            printf("Next pointer inconsistent for free block %p\n", bp);
        if ((char *) GETDW(PREV_FREE(bp)) != NULL && 
        (char *) GETDW(NEXT_FREE(GETDW(PREV_FREE(bp)))) != bp)
            printf("Previous pointer inconsistent for free block %p\n", bp);
    /* Checking coalesce function */
        if ((GET(HDRP(NEXT_BLKP(bp))) != 1) && 
            (GET(HDRP(NEXT_BLKP(bp))) != 3) && 
            !GET_ALLOC(HDRP(NEXT_BLKP(bp))))
            printf("No coalescing in pointers %p and %p\n", bp, NEXT_BLKP(bp));
        }
        bp = NEXT_BLKP(bp);
    }   
    /* Checking Epilogue header */
    if (!((GET(HDRP(bp)) == 1) || (GET(HDRP(bp)) == 3)))
        printf("Invalid Epilogue header\n");
    return 0;
}