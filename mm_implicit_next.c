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
    "jungle",
    /* First member's full name */
    "영지",
    /* First member's email address */
    "youngji.jang804@naver.com",
    /* Second member's full name (leave blank if none) */
    "형규",
    /* Second member's email address (leave blank if none) */
    "도영"
};


#define WSIZE  4
#define DSIZE  8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define PACK(size,alloc) ((size) | (alloc)) /*크기와 할당비트를 통해 해더와 풋터를 저장할 수 있는 값을 리턴*/

#define GET(p) (*(unsigned int*)(p))
#define PUT(p,val) (*(unsigned int*)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1) /* 할당됬는지 마지막 비트만 확인*/

#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE))) //괄호 주의

static char *heap_listp; /* 처음 쓸 큰 가용블럭 힙을 만들어준다.*/
static char *next_listp; //next-fit을 위한 전역변수

/*
* 연결
*/
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    void *temp = bp;

    if(prev_alloc && next_alloc){ /* case 1 */
        next_listp=bp;
        return bp;  
    }
    else if (prev_alloc && !next_alloc){ /* case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0)); //푸터 안바꾸나?
    }
    else if (!prev_alloc && next_alloc){ /* case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));
        bp = PREV_BLKP(bp);
        
    }
    else{ /* case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
        
    }
    next_listp=bp;
    return bp;
}

/*
* extend_heap 새 가용 블록으로 힙 확장하기
*/
static void *extend_heap(size_t words){ //static void 리턴 뭐야
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));

    return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp,0);
    PUT(heap_listp + (1*WSIZE),PACK(DSIZE,1));/* 프롤로그 헤더*/
    PUT(heap_listp + (2*WSIZE),PACK(DSIZE,1));/* 프롤로그 푸터*/
    PUT(heap_listp + (3*WSIZE),PACK(0,1));/* 에필로그 헤더*/
    heap_listp += (2*WSIZE);

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

static void *find_fit(size_t asize){
    
    if (next_listp == NULL){
        next_listp = heap_listp;
    }
    void *bp;
    for(bp = next_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            next_listp = bp;
            return bp;
        }
    }

    for(bp = heap_listp; bp < next_listp; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            next_listp = bp;
            return bp;
        }
    }
    //first-fit
    // for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
    //     if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
    //         return bp;
    //     }
    // }
    return NULL;
// #endif 뭐야넌
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2*DSIZE)){
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize,0));
        PUT(FTRP(bp), PACK(csize-asize,0));
    }
    else{
        PUT(HDRP(bp), PACK(csize,1));
        PUT(FTRP(bp), PACK(csize,1));
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }

    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0)
        return NULL;
    
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if((bp = find_fit(asize)) != NULL){
        place(bp,asize);
        next_listp = bp;
        return bp;
    }

    extendsize = MAX(asize,CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp,asize);
    next_listp = bp;
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) //이미 할당된 포인터 변수, 바꾸고 싶은 크기
{
    void *newptr;
    size_t oldSize;
    
    if (size == 0){
        mm_free(ptr);
        return 0;
    }
    if(ptr == NULL){ //주소가 없으면 새로운데 할당?
        return mm_malloc(size);
    }

    newptr = mm_malloc(size); 
    if (!newptr)
      return 0;

    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    oldSize = GET_SIZE(HDRP(ptr)); //변경된 최소 사이즈
    if (size < oldSize) 
      oldSize = size; //사이즈를 축소하고싶으면 자르기

    memcpy(newptr, ptr, oldSize); //메모리 복사(복사받을,복사할,복사할크기)
    mm_free(ptr); 
    return newptr;
}
