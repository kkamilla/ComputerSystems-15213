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

/* If you want debugging output, use the following macro.  
 * When you hand in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* To run mm_checkheap uncomment following line */
//#define MM_CHECKHEAP

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */


/* Extend heap by this amount (bytes) */
#define CHUNKSIZE  (168) 

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read and write double word at address p */
#define GETDSIZE(p)      (*(size_t *) (p))
#define PUTDSIZE(p, val) (*(size_t *) (p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)
#define GET_PREV_ALLOC(p)   (GET(p) & 0x2)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char*)(bp) - WSIZE)
#define FTRP(bp)    ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and prev blocks */
#define NEXT_BLKP(bp)   ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))
#define PREV_BLKP(bp)   ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

/* Given block ptr bp, compute address of next and prev free blocks */
#define NEXT_FREE(bp)   ((char*) ((char*)(bp)))
#define PREV_FREE(bp)   ((char*) ((char*)(bp) + DSIZE))

/* For storing in lower 3 bits of header/footer */
#define CURRENT_ALLOC   1
#define PREV_ALLOC      2

/* Total number of segregated lists */
#define LISTCOUNT   12

/* Size limit for each segregated lists */
/* 12th list size is beyond limit of 11th list */
#define LIST1MAX    24
#define LIST2MAX    48
#define LIST3MAX    72
#define LIST4MAX    96
#define LIST5MAX    120
#define LIST6MAX    480
#define LIST7MAX    960
#define LIST8MAX    1920
#define LIST9MAX    3840
#define LIST10MAX   7680
#define LIST11MAX   15360

/* Segregated lists pointer offsets */
#define LIST1   0
#define LIST2   8
#define LIST3   16
#define LIST4   24
#define LIST5   32
#define LIST6   40
#define LIST7   48
#define LIST8   56
#define LIST9   64
#define LIST10  72
#define LIST11  80
#define LIST12  88

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/* Global variables */
static char *heap_listp; /* Pointer to start lists on heap */
static char *heap_start; /* Pointer to first block */

/* Helper function declaration */
static void *extend_heap(size_t words);
static void addfreetolist(char *bp, size_t size);
static void delfreefromlist(char *bp, size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void *find_block(size_t listid, size_t asize);
static void place(void *bp, size_t asize);

/* Helper function definitions */

/*
 * Function: extend_heap 
 * Extends the heap, adds header and footer to new free block,
 * adds block to free list and calls coalesce
 */
static void *extend_heap(size_t words) {
    char *bp; /* Stores block pointer */
    /* Extend heap by requested size */
    if ((long)(bp = mem_sbrk(words)) < 0)
        return NULL;
    
    /* Initialize free block header/footer and the epilogue header */
    /* Free block header */
    PUT(HDRP(bp),PACK(words,GET_PREV_ALLOC(HDRP(bp))));
    /* Free block footer */
    PUT(FTRP(bp),GET(HDRP(bp))); 
    /* add free block to segregated list */
    addfreetolist(bp,words);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * Addfreetolist: Finds the list to store the free block
 * and adds the block in front of list
 */
static void addfreetolist(char *bp, size_t size) {
    char *head; /* Points to head of list */
    char *start; /* Points to addres of head */
    
    /* Find appropriate list based on size */
    if (size <= LIST1MAX) {
        start = heap_listp + LIST1;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST2MAX) {
        start = heap_listp + LIST2;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST3MAX) {
        start = heap_listp + LIST3;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST4MAX) {
        start = heap_listp + LIST4;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST5MAX) {
        start = heap_listp + LIST5;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST6MAX) {
        start = heap_listp + LIST6;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST7MAX) {
        start = heap_listp + LIST7;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST8MAX) {
        start = heap_listp + LIST8;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST9MAX) {
        start = heap_listp + LIST9;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST10MAX) {
        start = heap_listp + LIST10;
        head = (char *) GETDSIZE(start);
    } else if (size <= LIST11MAX) {
        start = heap_listp + LIST11;
        head = (char *) GETDSIZE(start);
    } else {
        start = heap_listp + LIST12;
        head = (char *) GETDSIZE(start);
    }

    /* If there are previous blocks in the list 
        added to front*/
    if (head != NULL) {
        /* Current block will be set as head */
        PUTDSIZE(start,(size_t) bp);
        /* Being new head set previous block to NULL */
        PUTDSIZE(PREV_FREE(bp),(size_t) NULL);
        /* Being new head set next block to previous head */
        PUTDSIZE(NEXT_FREE(bp),(size_t) head);
        /* Set previous block's previous pointer to current */
        PUTDSIZE(PREV_FREE(head),(size_t) bp);
        
    /* If there are no blocks in the list 
        this is first free block */
    } else {
        /* Current block will be set as head */
        PUTDSIZE(start,(size_t) bp);
        /* As it is first bloct, set previous 
            and next free blocks to NULL */
        PUTDSIZE(NEXT_FREE(bp),(size_t) NULL);
        PUTDSIZE(PREV_FREE(bp),(size_t) NULL);
    }
}

/*
 * Delfreefromlist: Deletes free block from the free list
 */
static void delfreefromlist(char *bp, size_t size) {
    /* prev and next free block addresses */
    char *next = (char *) GETDSIZE(NEXT_FREE(bp));
    char *prev = (char *) GETDSIZE(PREV_FREE(bp));
    
    /* If free block to be removed is head of list */
    if (prev == NULL && next != NULL) {
        /* Update head of list (next block will be head) */
        if (size <= LIST1MAX) {
            PUTDSIZE(heap_listp + LIST1, (size_t) next);
        } else if (size <= LIST2MAX) {
            PUTDSIZE(heap_listp + LIST2, (size_t) next);
        } else if (size <= LIST3MAX) {
            PUTDSIZE(heap_listp + LIST3, (size_t) next);
        } else if (size <= LIST4MAX) {
            PUTDSIZE(heap_listp + LIST4, (size_t) next);
        } else if (size <= LIST5MAX) {
            PUTDSIZE(heap_listp + LIST5, (size_t) next);
        } else if (size <= LIST6MAX) {
            PUTDSIZE(heap_listp + LIST6, (size_t) next);
        } else if (size <= LIST7MAX) {
            PUTDSIZE(heap_listp + LIST7, (size_t) next);
        } else if (size <= LIST8MAX) {
            PUTDSIZE(heap_listp + LIST8, (size_t) next);
        } else if (size <= LIST9MAX) {
            PUTDSIZE(heap_listp + LIST9, (size_t) next);
        } else if (size <= LIST10MAX) {
            PUTDSIZE(heap_listp + LIST10, (size_t) next);
        } else if (size <= LIST11MAX) {
            PUTDSIZE(heap_listp + LIST11, (size_t) next);
        } else {
            PUTDSIZE(heap_listp + LIST12, (size_t) next);
        }
        /* Previous block of new head will be NULL */
        PUTDSIZE(PREV_FREE(next), (size_t) NULL);
        
    /* If free block to be removed is only one in list */
    } else if (prev == NULL && next == NULL) {
        /* Set head of list to NULL */
        if (size <= LIST1MAX) {
            PUTDSIZE(heap_listp + LIST1, (size_t) NULL);
        } else if (size <= LIST2MAX) {
            PUTDSIZE(heap_listp + LIST2, (size_t) NULL);
        } else if (size <= LIST3MAX) {
            PUTDSIZE(heap_listp + LIST3, (size_t) NULL);
        } else if (size <= LIST4MAX) {
            PUTDSIZE(heap_listp + LIST4, (size_t) NULL);
        } else if (size <= LIST5MAX) {
            PUTDSIZE(heap_listp + LIST5, (size_t) NULL);
        } else if (size <= LIST6MAX) {
            PUTDSIZE(heap_listp + LIST6, (size_t) NULL);
        } else if (size <= LIST7MAX) {
            PUTDSIZE(heap_listp + LIST7, (size_t) NULL);
        } else if (size <= LIST8MAX) {
            PUTDSIZE(heap_listp + LIST8, (size_t) NULL);
        } else if (size <= LIST9MAX) {
            PUTDSIZE(heap_listp + LIST9, (size_t) NULL);
        } else if (size <= LIST10MAX) {
            PUTDSIZE(heap_listp + LIST10, (size_t) NULL);
        } else if (size <= LIST11MAX) {
            PUTDSIZE(heap_listp + LIST11, (size_t) NULL);
        } else {
            PUTDSIZE(heap_listp + LIST12, (size_t) NULL);
        }
    
    /* If free block to be removed is only one in list */
    } else if (prev != NULL && next == NULL) {
        /* Update previous block's next pointer to NULL */
        PUTDSIZE(NEXT_FREE(prev), (size_t) NULL);
        
    /* If free block to be removed is in between 
        of two free blocks */
    } else {
        /* Update previous of next free block */
        PUTDSIZE(PREV_FREE(next), (size_t) prev);
        /* Update next of previous free block */
        PUTDSIZE(NEXT_FREE(prev), (size_t) next);
    }
}

/*
 * Coalesce: Combines next, previous or both free blocks 
 * with current free block
 */
static void *coalesce(void *bp) {
    /* Get previous & next block's allocation status */
    size_t prev_status = GET_PREV_ALLOC(HDRP(bp));
    size_t next_status = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    
    /* Size of current, next & previous blocks */
    size_t size_current = GET_SIZE(HDRP(bp));
    size_t size_next = 0;
    size_t size_prev = 0;
    
    char *prev_head; /* Previous header */
    /* Next & previous block pointers */
    char *prev;
    char *next;
    
    /* If prev & next both are allocated no need to combine,
        so return pointer to current block*/
    if (prev_status && next_status) {
        return bp;
        
    /* If prev is allocated & next is free */
    } else if (prev_status && !next_status) {
        next = NEXT_BLKP(bp);
        /* Remove both (current & next) free blocks from list */
        delfreefromlist(bp,size_current);
        delfreefromlist(next,GET_SIZE(HDRP(next)));
        /* Size of new block is addition of
        current and next block sizes */
        size_current += GET_SIZE(HDRP(next));
        /* Update header & footer with new size */
        PUT(HDRP(bp),PACK(size_current,prev_status));
        PUT(FTRP(bp),PACK(size_current,prev_status));
        /* add new free block to list */
        addfreetolist(bp,size_current);
        /* Return current pointer */
        return bp;
        
    /* If prev is free and next is allocated */
    } else if (!prev_status && next_status) {
        prev = PREV_BLKP(bp);
        prev_head = HDRP(prev);
        size_prev = GET_SIZE(prev_head);
    /* Remove both (current & next) free blocks from list */
        delfreefromlist(bp,size_current);
        delfreefromlist(prev,size_prev);
    /* Size of new block is addition of
        current and previous block sizes */
        size_current += size_prev;
    /* Update header & footer with new size */
        PUT(prev_head,PACK(size_current,GET_PREV_ALLOC(prev_head)));
        PUT(FTRP(prev), GET(prev_head));
    /* add new free block to list */
        addfreetolist(prev,size_current);
    /* Return previous pointer */
        return prev;
    
    /* If previous & next both are free */
    } else {
        prev = PREV_BLKP(bp);
        prev_head = HDRP(prev);
        next = NEXT_BLKP(bp);
        size_prev = GET_SIZE(prev_head);
        size_next = GET_SIZE(HDRP(next));
    /* Remove all three (current, previous & next) 
        free blocks from list */
        delfreefromlist(bp,size_current);
        delfreefromlist(prev,size_prev);
        delfreefromlist(next,size_next);
    /* Size of new block is addition of
        current, previous and next block sizes */
        size_current += size_prev + size_next;
    /* Update header & footer with new size */
        PUT(prev_head,PACK(size_current,GET_PREV_ALLOC(prev_head)));
        PUT(FTRP(prev), GET(prev_head));
    /* add new free block to list */
        addfreetolist(prev,size_current);
    /* Return previous pointer */
        return prev;
    }
}

/*
 * Find_fit: Search fit for requested free block through all lists
 */
static void *find_fit(size_t asize) {
    size_t listid; /* List ID */
    char *bp = NULL; /* Block Pointer */
    
    /* Searched for fit through all lists, starting with 
        minimum size list*/
    if (asize <= LIST1MAX) {
        for (listid = 0; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST2MAX) {
        for (listid = 1; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST3MAX) {
        for (listid = 2; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST4MAX) {
        for (listid = 3; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST5MAX) {
        for (listid = 4; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST6MAX) {
        for (listid = 5; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST7MAX) {
        for (listid = 6; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST8MAX) {
        for (listid = 7; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST9MAX) {
        for (listid = 8; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST10MAX) {
        for (listid = 9; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else  if (asize <= LIST11MAX) {
        for (listid = 10; listid < LISTCOUNT; listid++) {
            if ((bp = find_block(listid, asize)) != NULL)
                return bp;
        }
    } else {
        listid = 11;
        if ((bp = find_block(listid, asize)) != NULL)
            return bp;
    }
    return bp;
}

/*
 * Find_block: Search fit for requested free block in perticular list
 */
static void *find_block(size_t listid, size_t asize) {
    char *bp = NULL;
    /* Get head block pointer for list */
    switch (listid) {
        case 0:
            bp = (char *) GETDSIZE(heap_listp + LIST1);
            break;
        case 1:
            bp = (char *) GETDSIZE(heap_listp + LIST2);
            break;
        case 2:
            bp = (char *) GETDSIZE(heap_listp + LIST3);
            break;
        case 3:
            bp = (char *) GETDSIZE(heap_listp + LIST4);
            break;
        case 4:
            bp = (char *) GETDSIZE(heap_listp + LIST5);
            break;
        case 5:
            bp = (char *) GETDSIZE(heap_listp + LIST6);
            break;
        case 6:
            bp = (char *) GETDSIZE(heap_listp + LIST7);
            break;
        case 7:
            bp = (char *) GETDSIZE(heap_listp + LIST8);
            break;
        case 8:
            bp = (char *) GETDSIZE(heap_listp + LIST9);
            break;
        case 9:
            bp = (char *) GETDSIZE(heap_listp + LIST10);
            break;
        case 10:
            bp = (char *) GETDSIZE(heap_listp + LIST11);
            break;
        case 11:
            bp = (char *) GETDSIZE(heap_listp + LIST12);
            break;
    }
    /* Traverse through list to find fit */
    while (bp != NULL) {
        if (asize <= GET_SIZE(HDRP(bp))) {
                break; /* Break & return as first fit is found */
        }
        bp = (char *) GETDSIZE(NEXT_FREE(bp));
    }
    return bp;
}

/*
 * Place: Allocates free block. 
 * Splits block if remaining size is greater 
 * than minimum block size.
 */
static void place(void *bp, size_t asize) {
    /* Current free block size */
    size_t size_current = GET_SIZE(HDRP(bp));
    /* Remaining free block size */
    size_t size_remain = size_current - asize;
    /* Next block */
    char *next = NEXT_BLKP(bp);
    
    /* Delete free block from list */
    delfreefromlist(bp, size_current);
    
    /* Split block if remaining size is
        greater than or equal to BLOCKSIZE_MIN */
    if (size_remain >= 24) {
    /* Update new header */
        PUT(HDRP(bp),PACK(asize,\
            GET_PREV_ALLOC(HDRP(bp)) | CURRENT_ALLOC));
    /* Update next block's address */
        next = NEXT_BLKP(bp);
    /* Update next free block with status of previous's allocation */
        PUT(HDRP(next), size_remain | PREV_ALLOC);
        PUT(FTRP(next), size_remain | PREV_ALLOC);

    /* Add remaining free block to list */
        addfreetolist(next, size_remain);

    /* Assign entire free block if remaining size
        is less than BLOCKSIZE_MIN*/       
    } else {
    /* Update new header */
        PUT(HDRP(bp),PACK(size_current,\
                    GET_PREV_ALLOC(HDRP(bp)) | CURRENT_ALLOC));
    /* Update next free block with status of previous's allocation */
        PUT(HDRP(next),GET(HDRP(next)) | PREV_ALLOC);
    /* Footer is updated only if next block is free */
        if (!GET_ALLOC(HDRP(next)))
            PUT(FTRP(next), GET(HDRP(next)));
    }
}

/* Primary function definitions */

/*
 * Initialize: Initializes heap, pointers to free lists
 * also creates prologue and epilogue blocks.
 * returns -1 on error, 0 on success.
 */
int mm_init(void) {
    /* Create initial empty heap */
    if ((heap_listp = mem_sbrk(LISTCOUNT*DSIZE)) == NULL)
        return -1;
    /* Segregations lists pointers 
        initialization with null pointers*/
    PUTDSIZE(heap_listp + LIST1, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST2, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST3, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST4, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST5, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST6, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST7, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST8, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST9, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST10, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST11, (size_t) NULL);
    PUTDSIZE(heap_listp + LIST12, (size_t) NULL);
    
    /* Extend heap for Epilogue & Prologue */
    if ((heap_start = mem_sbrk(4*WSIZE)) == NULL)
        return -1;
    /* Alignment padding */
    PUT(heap_start, 0);
    /* Prologue header */
    PUT(heap_start + (1*WSIZE), PACK(DSIZE, 1));
    /* Prologue footer */
    PUT(heap_start + (2*WSIZE), PACK(DSIZE, 1));
    /* Epilogue header */
    PUT(heap_start + (3*WSIZE), PACK(0, 3));
    
    /* Assiging CHUNKSIZE bytes to heap */
    if (extend_heap(CHUNKSIZE) == NULL)
            return -1;
    return 0;
}

/*
 * malloc: Allocated the block of requested size on heap and
 * returns pointer to the block.
 */
void *malloc (size_t size) {
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
    
    /* Ignore spurious requests */
    if (size <= 0)
        return NULL;
    /* Adjust block size to include overhead and 
        alignment requirements */
    if (size <= 2*DSIZE)
        asize = 24;
    else
        asize = (size_t)ALIGN(size + WSIZE);
    
    /* Checking Heap */
    #ifdef MM_CHECKHEAP
        mm_checkheap(0);
    #endif
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp,asize);        /* Assignment */
        return bp;
    }
    /* No fit found. Get more memory and place the block */
    if (asize > CHUNKSIZE)
        extendsize = asize;
    else 
        extendsize = CHUNKSIZE;
    if ((bp = extend_heap(extendsize)) == NULL)
        return NULL;
    place(bp,asize);                /* Assignment */
    return bp;
}

/*
 * free: Frees the block pointed by ptr. Freed block is added to 
 * appropriate list.        
 * 
 */
void free (void *ptr) {
    /* Return if null pointer */
    if(ptr == NULL) return;
    /* Get next head and size of ptr */
    char *next_head = HDRP(NEXT_BLKP(ptr));
    size_t size = GET_SIZE(HDRP(ptr));
    /* Update header and footer to unallocated */
    PUT(HDRP(ptr), size | GET_PREV_ALLOC(HDRP(ptr)));
    PUT(FTRP(ptr), GET(HDRP(ptr)));
    /* Update next block header*/
    PUT(next_head,GET_SIZE(next_head) | GET_ALLOC(next_head));
    /* Add free block to list */
    addfreetolist(ptr,size);
    coalesce(ptr);
}

/*
 * realloc: It reallocates previous block with new size.
 * Old data is copied to new location and old block is freed.
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        free(oldptr);
        return 0;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL) {
        return malloc(size);
    }
      
    /* Else allocate new space */
      newptr = malloc(size);

    /* If realloc() fails the original block is left untouched  */
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
    size_t bytes = nmemb * size;
    void *newptr = malloc(bytes);
    memset(newptr, 0, bytes);
    return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap: Check the consistency of heap
 * Following checks are implemented:
 * 1. Check that all the free blocks fall withing appropriate ranges
 * 2. Check Prologue header
 * 3. Check Prologue footer
 * 4. Check alignment of each block
 * 5. Check that each block exist inside heap (heap boundries)
 * 6. Check if header & footer mismatch
 * 7. Check inconsistencies for next & previous pointers
 * 8. Check coalescing
 * 9. Compare counts of free blocks when traversing 
 *      through heap & segregated free lists
 * 10. Check Epilogue header
 */
int mm_checkheap(int verbose) {
    size_t listid; /* List ID */
    char *listhead; /* Head of the list */
    /* Minimum and maximum block sizes for list */
    unsigned min, max;
    unsigned freecount1=0, freecount2=0; /* To count free blocks */
    char *bp; /* block pointer */
    
    /* 1. Check that all the free blocks fall 
            within appropriate ranges */
    for (listid = 0; listid < LISTCOUNT; listid++) {
        switch (listid) {
            case 0:
                listhead = (char *) GETDSIZE(heap_listp + LIST1);
                min = 0;
                max = LIST1MAX;
                break;
            case 1:
                listhead = (char *) GETDSIZE(heap_listp + LIST2);
                min = LIST1MAX;
                max = LIST2MAX;
                break;
            case 2:
                listhead = (char *) GETDSIZE(heap_listp + LIST3);
                min = LIST2MAX;
                max = LIST3MAX;
                break;
            case 3:
                listhead = (char *) GETDSIZE(heap_listp + LIST4);
                min = LIST3MAX;
                max = LIST4MAX;
                break;
            case 4:
                listhead = (char *) GETDSIZE(heap_listp + LIST5);
                min = LIST4MAX;
                max = LIST5MAX;
                break;
            case 5:
                listhead = (char *) GETDSIZE(heap_listp + LIST6);
                min = LIST5MAX;
                max = LIST6MAX;
                break;
            case 6:
                listhead = (char *) GETDSIZE(heap_listp + LIST7);
                min = LIST6MAX;
                max = LIST7MAX;
                break;
            case 7:
                listhead = (char *) GETDSIZE(heap_listp + LIST8);
                min = LIST7MAX;
                max = LIST8MAX;
                break;
            case 8:
                listhead = (char *) GETDSIZE(heap_listp + LIST9);
                min = LIST8MAX;
                max = LIST9MAX;
                break;
            case 9:
                listhead = (char *) GETDSIZE(heap_listp + LIST10);
                min = LIST9MAX;
                max = LIST10MAX;
                break;
            case 10:
                listhead = (char *) GETDSIZE(heap_listp + LIST11);
                min = LIST10MAX;
                max = LIST11MAX;
                break;
            case 11:
                listhead = (char *) GETDSIZE(heap_listp + LIST12);
                min = LIST11MAX;
                max = ~0;
                break;
        }
        while (listhead != NULL) {
            if (!(GET_SIZE(HDRP(listhead)) > min && \
                GET_SIZE(HDRP(listhead)) <= max)) {
                printf("Free Block with pointer %p is \
                    not in the appropriate list\n", listhead);
            }
            if (!GET_ALLOC(HDRP(listhead))) {
                freecount1++; /* count free blocks */
            }
            listhead = (char *) GETDSIZE(NEXT_FREE(listhead));
        }
    }
    /* 2. Check Prologue header */
    bp = heap_start + (1*WSIZE);
    if ((GET_SIZE(bp) != DSIZE) || !GET_ALLOC(bp))
        printf("Invalid prologue header for heap\n");
    
    /* 3. Check Prologue footer */  
    bp = heap_start + (2*WSIZE);
    if ((GET_SIZE(bp) != DSIZE) || !GET_ALLOC(bp))
        printf("Invalid prologue footer for heap\n");
    /* Block pointer for first block */
    bp = heap_listp + LISTCOUNT*DSIZE + 2*DSIZE;
    /* Loop to traverse through heap */
    while ((GET(HDRP(bp)) != 1) && (GET(HDRP(bp)) != 3)) {
        if (verbose) {
            printf("Block pointer [%p] , size [%d],", \
                    bp, GET_SIZE(HDRP(bp)));
            if (!GET_ALLOC(HDRP(bp)))
                printf("status[free]\n");
            else
                printf("status[allocated]\n");
        }
    /* 4. Check alignment of each block */
        if (!aligned(bp))
            printf("Block with pointer %p is not aligned\n", bp);
    /* 5. Check that each block exist 
            inside heap (heap boundries) */
        if (!in_heap(bp)) {
            printf("Block with pointer %p does not\
                    exist in heap\n", bp);
        }
    /* For free blocks */
        if (!GET_ALLOC(HDRP(bp))) {
            freecount2++; /* count free blocks */
    /* 6. Check if header & footer mismatch */
        if ((GET(HDRP(bp)) != GET(FTRP(bp))))
            printf("Header and Footer mismatch for free block \
                    with pointer %p\n", bp);
    /* 7. Check inconsistencies for next & previous pointers */
        if ((char *) GETDSIZE(NEXT_FREE(bp)) != NULL && \
        (char *) GETDSIZE(PREV_FREE(GETDSIZE(NEXT_FREE(bp)))) != bp)
            printf("Next pointer inconsistency for \
                Free block with pointer %p\n", bp);
        if ((char *) GETDSIZE(PREV_FREE(bp)) != NULL && \
        (char *) GETDSIZE(NEXT_FREE(GETDSIZE(PREV_FREE(bp)))) != bp)
            printf("Previous pointer inconsistency for \
                Free block with pointer %p\n", bp);

    /* 8. Check coalescing */
        if ((GET(HDRP(NEXT_BLKP(bp))) != 1) && \
            (GET(HDRP(NEXT_BLKP(bp))) != 3) && \
            !GET_ALLOC(HDRP(NEXT_BLKP(bp))))
            printf("No coalcing for free blocks with \
                    pointers %p and %p\n", bp, NEXT_BLKP(bp));
        }
        bp = NEXT_BLKP(bp);
    }   
    /* 9. Compare counts of free blocks when traversing 
            through heap & segregated free lists */
    if (verbose)
        printf("freecount1, freecount2 : %u, %u\n", \
        freecount1, freecount2);
    if (freecount1 != freecount2) {
        printf("Free blocks count do not match when \
        traversing through heap or all segregated free lists\n");
        printf("Free count traversing through heap:%u\n",freecount1);
        printf("Free count traversing through free lists:%u\n",\
                freecount2);
    }
    /* 10. Check Epilogue header */
    if (!((GET(HDRP(bp)) == 1) || (GET(HDRP(bp)) == 3)))
        printf("Invalid Epilogue header for heap\n");
   return 0; 
}