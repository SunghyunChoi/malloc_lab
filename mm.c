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


#define NEXT_FITx

 /*********************************************************
  * NOTE TO STUDENTS: Before you do anything else, please
  * provide your team information in the following struct.
  ********************************************************/

team_t team = {
    /* Team name */
    "team 8",
    /* First member's full name */
    "Hyungwoo Kim",
    /* First member's email address */
    "gulb1602@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};


/* Basic constants and macros */

#define WSIZE     4                                         /* Word and header/footer size (bytes), size in bytes of a single word, 한 워드 크기 */
#define DSIZE     8                                         /* Double word size (bytes), size in bytes of a double word, 더블 워드 크기 */         

//#define INITSIZE          16                                /* Initial size of free list before first free block added, 초기 힙 크기 */
//#define MINBLOCKSIZE      16                                /* Minmum size for a free block, includes 4 bytes for header/footer                                                            free blocks, 최소 블록 크기 */

#define MINIMUM           24
#define CHUNKSIZE         16



#define MAX(x, y) ((x) > (y)? (x) : (y))                    /* x와 y중 큰 값을 구한다. */

                                                               /* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size)|(alloc))                /* 크기와 allocated 비트를 합친다. 즉, header와 footer로 사용된다. */

/* Read and write a word at address p */
#define GET(p)  (*(unsigned int *)(p))                      /* 주소 p가 가리키는 곳에 있는 값을 읽어온다. */    
#define PUT(p, val)  (*(unsigned int *)(p) = (val))         /* 주소 p에 값 val을 저장한다. */

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                                                /* p포인터가 가리키는 곳의 size를 가져옴(블락사이즈를 가져옴) */
#define GET_ALLOC(p)  (GET(p) & 0x1)                                                /* p포인터가 가리키는 곳의 allocation 정보를 가져옴(1 또는 0) */

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char *)(bp) - WSIZE)                                          /* 블락 포인터에서 왼쪽으로 word size(4 byte) 만큼 이동한 포인터를 반환(블락의 헤더를 가리키는 포인터) */
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp))-DSIZE)                       /* 헤더포인터에서 가져온 사이즈를 블락포인터에 더해준뒤 DSIZE만큼 두칸 땡낌(블락의 풋터를 가리키는 포인터) */

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))            /* 다음 블락의 블락포인터를 반환 */
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))            /* 이전 블락의 블락포인터를 반환 */

/* Given block ptr bp, compute address of next and previous free blocks */
#define PREV_FREE(bp) (*(void **)(bp))                                      /* 블락 포인터 bp의 이전 가용 블록을 계산함 */
#define NEXT_FREE(bp) (*(void **)(bp + WSIZE))                                              /* 블락 포인터 bp의 다음 가용 블록을 계산함 */


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8                                             // memory alignment factor

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Global variables, Private variables representing the heap and free list within the heap */
static char* heap_listp; /* Pointer to first block */

static char* free_listp; /* explicit method, 첫 번째 가용 블록의 포인터로 사용됨 */

#ifdef NEXT_FIT
static char* next_ptr; /* Next fit rover */
#endif

// PROTOTYPES
static void* extend_heap(size_t words);
static void* find_fit(size_t size);
static void* coalesce(void* bp);
static void place(void* bp, size_t asize);
static void removeFreeBlock(void* bp);
// static int mm_check();


/**
 *	@fn		mm_init
 *
 *	@brief                  Initialize the heap with freelist prologue/epilogoue and space for the
 *                          initial free block. (32 bytes total)
 *                          freelist를 이용하여 heap을 초기화한다. 이때 32bytes만큼의 heap을 만들어서
 *                          prologue와 epilogoue 그리고 header, next, prev, footer pointer를 지정해준다.
 *
 *  @param  void
 *
 *	@return	int             initialize: return -1 on error, 0 on success.
 */

 /*
  * mm_init - Initializes the heap like that shown below.
  *
  * FREE BLOCK
  * |------------|------------|------------|-------------|------------|
  * |   PADDING  |   HEADER   |   PREV     |   NEXT      |   FOOTER   |
  * |------------|------------|------------|-------------|------------|
  * ^                         ^
  * heap_listp                bp = free_listp
  */

int mm_init(void)
{

    /* Create the initial empty heap */
    /* 메모리에 초기 empty heap을 요구한다. */
    //if ((heap_listp = mem_sbrk(2*MINIMUM)) == (void*)-1)
    if ((heap_listp = mem_sbrk(3 * DSIZE)) == (void*)-1)

        return -1;

    /* 처음 블록 부분을 초기화 한다. */
    PUT(heap_listp, 0);                                // Padding
    PUT(heap_listp + (1 * WSIZE), PACK(MINIMUM, 1));                  // Free block header
    PUT(heap_listp + (2 * WSIZE), PACK(0, 0));                             // Space for prev pointer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 0));                             // Space for next pointer
    PUT(heap_listp + (4 * WSIZE), PACK(MINIMUM, 1));                  // Free block footer
    /* epilogue 블록을 초기화 한다. */
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));                             // Epilogue header

    // 가용 리스트 포인터를 초기화 한다.
    free_listp = heap_listp + DSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

#ifdef NEXT_FIT
    next_ptr = heap_listp;
#endif
    return 0;
}



/**
 *	@fn		mm_malloc
 *
 *	@brief                  Allocate a block by incrementing the brk pointer.
 *                         Always allocate a block whose size is a multiple of the alignment.
 *
 *                         A block is allocated according to this strategy:
 *                         (1) If a free block of the given size is found, then allocate that free block and return
 *                         a pointer to the payload of that block.
 *                         (2) Otherwise a free block could not be found, so an extension of the heap is necessary.
 *                         Simply extend the heap and place the allocated block in the new free block.
 *
 * @param  size           매개변수 size를 넘겨받아 그 크기만큼 블록을 할당함
 *
 *	@return void*          initialize: return -1 on error, 0 on success.
 */

void* mm_malloc(size_t size)
{
    /* size가 올바르지 않을 때 예외처리 */
    /* size가 음수 값일 때는 무시한다. */
    if (size <= 0)
        return NULL;

    size_t asize;                                           /* Adjusted block size, 조정된 블록 크기를 위한 변수 */
    size_t extendsize;                                      /* Amount to extend heap if no fit, 맞는 fit이 없을 경우 힙을 늘리는 양 */
    char* bp;


    /* block의 크기 결정 */
    /* 오버헤드와 정렬을 위해 필요한 부분을 포함해서 크기를 결정한다. */
    /* 적어도 최소 블록 크기를 갖도록, 매크로로 구현한 MAX를 이용하여 크기를 결정한다. */
    asize = MAX(ALIGN(size) + DSIZE, MINIMUM);

    /* Search the free list for a fit */
    /* 적절한 가용블록을 가용리스트에서 검색, 결정한 크기에 알맞은 블록을 list에서 검색하여 해당 위치에 할당한다. */
    if ((bp = find_fit(asize))) {
        place(bp, asize);
        /* 맞는 블록 찾으면 할당기는 요청한 블록 배치하고, 옵션으로 초과부분을 분할 */
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    /* 힙을 새로운 가용 블록으로 확장, free list에서 적절한 블록을 찾지 못했으면 힙을 늘려서 할당하고, 그곳에 블록을 할당한다. */
    extendsize = MAX(asize, MINIMUM); // 위에 asize에서 체크해주니까 필요없지않을까?

    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* bp)
{
    if (!bp) return;
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * remove_freeblock - Removes the given free block pointed to by bp from the free list.
 *
 * The explicit free list is simply a doubly linked list. This function performs a removal
 * of the block from the doubly linked free list.
 */

 // 가용 리스트에서 블록을 제거한다
static void removeFreeBlock(void* bp)
{
    if (bp) {
        /* 이전 가용 블록이 있다면, 그 블록의 next를 현재의 다음 가용 블록을 가리키도록 한다. */
        if (PREV_FREE(bp)) {
            NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
        }
        /* 그렇지 않으면, 가용 리스트의 처음(free_listp)이 다음 가용 블록을 가리키도록 한다. */
        else
            free_listp = NEXT_FREE(bp);

        /* 다음 블록의 prev는 bp의 prev가 가리키는 곳을 가리키도록 한다. */
        if (NEXT_FREE(bp) != NULL)
            PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
    }

}


static void* find_fit(size_t size)
{


#ifdef NEXT_FIT 
    /* Next fit search */
    char* bp = next_ptr;

    /* Search from the next_ptr to the end of list */
    for (; GET_SIZE(HDRP(next_ptr)) > 0; next_ptr = NEXT_BLKP(next_ptr))
        if (!GET_ALLOC(HDRP(next_ptr)) && (asize <= GET_SIZE(HDRP(next_ptr))))
            return next_ptr;

    /* search from start of list to old ptr(= heap_listp) */
    for (next_ptr = heap_listp; next_ptr < bp; next_ptr = NEXT_BLKP(next_ptr))
        if (!GET_ALLOC(HDRP(next_ptr)) && (asize <= GET_SIZE(HDRP(next_ptr))))
            return next_ptr;

    return NULL;  /* no fit found */


   ///* Next-fit search (성현이형 코드)*/
   // void* bp;
   // for (bp = next_ptr; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
   //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
   //         //lowers the performance
   //         //next_start = bp;
   //         return bp;
   //     }
   // }
   // for (bp = heap_listp; bp != next_ptr; bp = NEXT_BLKP(bp)) {
   //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
   //         //lowers the performance
   //         //next_start = bp;
   //         return bp;
   //     }
   // }
   // return NULL; /* No fit */
#else 
    /* $begin mmfirstfit */
        /* First fit search */
    void* bp;

    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREE(bp)) {
        if (size <= (size_t)GET_SIZE(HDRP(bp))) {
            return bp;
        }
    }
    return NULL; /* No fit */
/* $end mmfirstfit */
#endif
}




static void place(void* bp, size_t asize)
{
    size_t fsize = GET_SIZE(HDRP(bp)); // free size

    if ((fsize - asize) >= MINIMUM) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        removeFreeBlock(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(fsize - asize, 0));
        PUT(FTRP(bp), PACK(fsize - asize, 0));
        coalesce(bp);
    }
    else {
        PUT(HDRP(bp), PACK(fsize, 1));
        PUT(FTRP(bp), PACK(fsize, 1));
        removeFreeBlock(bp);
    }
}


static void* coalesce(void* bp)
{

    /* 이전 블록과 다음 블록이 할당되어 있는지 알아보는 변수들 */
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* 이전 블록, 다음 블록 할당인 경우 */
    // 아무 것도 하지 않음. 단지 해당 블록을 가용리스트 맨 앞에 삽입

    /* 이전 블록 할당, 다음 블록 가용인 경우 */
    if (prev_alloc && !next_alloc) {                              /* Case 1 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        removeFreeBlock(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {                        /* Case 2 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        removeFreeBlock(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && !next_alloc) {                        /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        removeFreeBlock(PREV_BLKP(bp));
        removeFreeBlock(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    NEXT_FREE(bp) = free_listp;
    PREV_FREE(free_listp) = bp;
    PREV_FREE(bp) = NULL;
    free_listp = bp;

    return bp;
}

/**
 *	@fn		extend_heap
 *
 *	@brief                  가용 블록의 힙을 늘리고, 그것의 블록 포인터를 반환한다.
 *
 *  @param  words           words 크기의 사이즈만큼 힙을 늘려준다.
 *
 *	@return	static void*
 */

static void* extend_heap(size_t words)
{
    char* bp;                   /* 힙을 확장한 후에 새로운 블록 포인터 */
    size_t asize;               /* 힙 메모리에 size를 요구 */

    /* Allocate an even number of words to mainitain alignment */
    /* 정렬을 유지하기 위해 words의 수를 짝수로 만든다. */
    asize = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if (asize < MINIMUM)
        asize = MINIMUM;

    if ((long)(bp = mem_sbrk(asize)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    /* 가용 블록의 header와 footer, epilogue를 초기화 한다. */
    PUT(HDRP(bp), PACK(asize, 0));                                       /* Free block header */
    PUT(FTRP(bp), PACK(asize, 0));                                       /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));                               /* New epilogue header */

    /* Coalesce if the previous block was free */
    /* 이전 블록이 가용 블록이었다면 합병을 하고, 가용리스트에 블록을 추가한다. */
    return coalesce(bp);
}








/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size)
{

    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size <= 0) {
        mm_free(ptr);
        return NULL;
    }



    size_t asize = MAX(ALIGN(size) + DSIZE, MINIMUM);
    size_t current_size = GET_SIZE(HDRP(ptr));

    void* bp;
    char* next = HDRP(NEXT_BLKP(ptr));
    size_t newsize = current_size + GET_SIZE(next);


    if (asize == current_size)
        return ptr;

    if (asize <= current_size) {
        if (asize > MINIMUM && (current_size - asize) > MINIMUM) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            bp = NEXT_BLKP(ptr);
            PUT(HDRP(bp), PACK(current_size - asize, 1));
            PUT(FTRP(bp), PACK(current_size - asize, 1));
            mm_free(bp);
            return ptr;
        }

        bp = mm_malloc(asize);
        memcpy(bp, ptr, asize);
        mm_free(ptr);

        return bp;
    }

    else {
        if (!GET_ALLOC(next) && newsize >= asize) {
            removeFreeBlock(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            bp = NEXT_BLKP(ptr);
            PUT(HDRP(bp), PACK(newsize - asize, 1));
            PUT(FTRP(bp), PACK(newsize - asize, 1));
            mm_free(bp);
            return ptr;
        }

        bp = mm_malloc(asize);
        memcpy(bp, ptr, current_size);
        mm_free(ptr);
        return bp;
    }

}

////consistency checker

//static int mm_check() {

//  // Is every block in the free list marked as free?
//  void *next;
//  for (next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREE(next)) {
//    if (GET_ALLOC(HDRP(next))) {
//      printf("Consistency error: block %p in free list but marked allocated!", next);
//      return 1;
//    }
//  }

//  // Are there any contiguous free blocks that escaped coalescing?
//  for (next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREE(next)) {

//    char *prev = PREV_FREE(HDRP(next));
//      if(prev != NULL && HDRP(next) - FTRP(prev) == DSIZE) {
//        printf("Consistency error: block %p missed coalescing!", next);
//        return 1;
//      }
//  }

//  // Do the pointers in the free list point to valid free blocks?
//  for (next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREE(next)) {

//    if(next < mem_heap_lo() || next > mem_heap_hi()) {
//      printf("Consistency error: free block %p invalid", next);
//      return 1;
//    }
//  }

//  // Do the pointers in a heap block point to a valid heap address?
//  for (next = heap_listp; NEXT_BLKP(next) != NULL; next = NEXT_BLKP(next)) {

//    if(next < mem_heap_lo() || next > mem_heap_hi()) {
//      printf("Consistency error: block %p outside designated heap space", next);
//      return 1;
//    }
//  }

//  return 0;
//}