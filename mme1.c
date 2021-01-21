/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team-8",
    /* First member's full name */
    "Sunghyun Choi",
    /* First member's email address */
    "sunghyun0714@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//_________________________________________//
static char* heap_listp;
static char* start;
static char* end;
/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */
#define MAX(x, y) ((x) > (y)? (x) : (y))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define NEXT_PTR(bp) (char *)(GET((char *)(bp) + WSIZE)) - WSIZE
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define PREV_PTR(bp) (char *)(GET((char *)(bp)))
//#define NEXT_PTR(p)  (*(char **)(p + WSIZE))
//#define PREV_PTR(p)  (*(char **)(p))
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
//_________________________________________//

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    printf("Start extend_heap");
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(bp, (char *)PREV_PTR(end)); //save end's PREV_PTR's address at bp
    end = (char*)NEXT_BLKP(bp);
    PUT(HDRP(NEXT_PTR(bp)), PACK(3, 1)); /* New epilogue header */
    PUT(NEXT_PTR(bp), bp); //save bp to epilouge PREV
    PUT(NEXT_PTR(bp)+WSIZE, NULL); // save NULL to epilogue NEXT
    PUT((char*)(PREV_PTR(bp))+WSIZE, (char*)(bp)+WSIZE);//save bp+4 to PREV_PTR+4
    PUT((char*)bp + WSIZE, (char*)(NEXT_BLKP(bp)) + WSIZE);
    printf("Heap increase success");
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    char *bp;
    printf("Start init");
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(8*WSIZE)) == (void *)-1){
        return -1;
    }
    PUT(heap_listp + (0*WSIZE), PACK(2*DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (1*WSIZE), NULL);// points to PREV
    PUT(heap_listp + (2*WSIZE), NULL);// points to NEXT
    PUT(heap_listp + (3*WSIZE), PACK(2*DSIZE, 1)); /* Prologue footer */
    //PUT(heap_listp + (5*WSIZE), PACK(0, 1)); /* Epilogue header */
    /* made a Prologue of size 4 to include Prev, Next Pointer*/
    PUT(heap_listp + (4*WSIZE), PACK(2*DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (5*WSIZE), heap_listp+3*WSIZE);// Epilogue PREV
    PUT(heap_listp + (6*WSIZE), NULL);// Epilogue NEXT
    PUT(heap_listp + (7*WSIZE), PACK(2*DSIZE, 1)); /* Epilogue header */
    
    heap_listp += (2*WSIZE);
    //start = heap_listp + WSIZE;
    //end = heap_listp + (4*WSIZE);
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    printf("Initialize Success");
    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
 void *mm_malloc(size_t size)
{
    printf("Start malloc");
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    printf("Start free");
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void *coalesce(void *bp)
{
    printf("Start coalesce");
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc) { /* Case 1 */
        //find previous free block
        char* next_bp;
        char* prev_bp = bp;
        
        for(;prev_bp != NULL && !(GET_ALLOC(HDRP(prev_bp)));prev_bp = PREV_BLKP(prev_bp)){
        } 
        PUT(bp, prev_bp);
        next_bp = GET(NEXT_PTR((char*)prev_bp+WSIZE));
        PUT((char*)prev_bp + WSIZE, (char*)(bp) + WSIZE);
        PUT((char*)(bp) + WSIZE, (char*)(next_bp) + WSIZE);
        PUT(next_bp, bp);
        return bp;
    }
    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
        PUT(NEXT_PTR(NEXT_BLKP(bp)), bp);
        PUT((char*)PREV_PTR(NEXT_BLKP(bp))+WSIZE, (char*)bp + WSIZE);
        PUT(bp, GET(NEXT_BLKP(bp)));
        PUT((char*)bp+WSIZE, GET((char*)NEXT_BLKP(bp)+WSIZE));
    }
    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT((char*)PREV_BLKP(bp)+WSIZE, GET((char*)NEXT_BLKP(bp)+WSIZE));
        PUT(PREV_PTR(NEXT_BLKP(bp)), PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *find_fit(size_t asize)
{
    printf("Start find_fit");
    /* First-fit search */
    void *bp;
    for (bp = NEXT_PTR(heap_listp); NEXT_PTR(bp)!= NULL; bp = NEXT_PTR(bp)){
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
                printf("Found");
                return bp;
            }
        }
    return NULL; /* No fit */
}

static void place(void *bp, size_t asize)
{
    printf("place");
    char *prev_bp = PREV_PTR(bp);
    char *next_bp = NEXT_PTR(bp);
    size_t csize = GET_SIZE(HDRP(bp));
    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT((char*)(bp), (char*)(prev_bp));
        PUT((char*)(prev_bp)+WSIZE, (char*)(bp) + WSIZE);
        PUT((char*)(bp)+WSIZE, (char*)next_bp + WSIZE);
        PUT((char*)next_bp, bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        PUT((char*)(prev_bp)+WSIZE, (char*)(next_bp)+WSIZE);
        PUT((char*)(next_bp), prev_bp);
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */


void *mm_realloc(void *ptr, size_t size)
{
    printf("Start realloc");
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - WSIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}











