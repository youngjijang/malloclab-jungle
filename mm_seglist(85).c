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

#define PRED(bp) ((char*)(bp)) //그냥 bp,,?
#define SUCC(bp) ((char*)(bp) + WSIZE) //다음 블록 주소?

//블록 최소 크기인 2**4부터 최대 크기인 2**32를 위한 리스트 29개
#define LIST_NUM 29

static char *heap_listp; /* 처음 쓸 큰 가용블럭 힙을 만들어준다.*/
void *seg_list[LIST_NUM]; //연결리스트 집합  

void delete_block(char* bp);
void add_free_block(char* bp);
int get_seg_list_num(size_t size); //stack에 저장????

/*
* 연결
*/
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){ /* case 1 */
        add_free_block(bp);
        return bp;  
    }
    else if (prev_alloc && !next_alloc){ /* case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        delete_block(NEXT_BLKP(bp));
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0)); //푸터 안바꾸나?
    }
    else if (!prev_alloc && next_alloc){ /* case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        delete_block(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    else{ /* case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
        delete_block(NEXT_BLKP(bp));
        delete_block(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
    }

    add_free_block(bp);
    return bp;
}

/*
* extend_heap 새 가용 블록으로 힙 확장하기
*/
static void *extend_heap(size_t words){ //static void 리턴 뭐야
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; //DSIZE하면 2점-
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
    //각 분리가용 리스트를 초기화
    for(int i = 0; i <LIST_NUM; i++){
        seg_list[i] = NULL;
    }
    
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;

    PUT(heap_listp,0);
    PUT(heap_listp + (1*WSIZE),PACK(DSIZE,1));/* 프롤로그 헤더*/
    PUT(heap_listp + (2*WSIZE),PACK(DSIZE,1));/* 프롤로그 푸터*/
    PUT(heap_listp + (3*WSIZE),PACK(0,1));/* 에필로그 헤더*/
    
    // heap_listp += (4*WSIZE);

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

static void *find_fit(size_t asize){
    void *bp;
    int i = get_seg_list_num(asize); //알맞은 segsize 찾기

    //리스트 내부의 블록들중 가장 작은 블록 할당(best fit)
    void *tmp = NULL;
    while (i < LIST_NUM){
        //first fit
        for(bp = seg_list[i]; bp != NULL; bp = GET(SUCC(bp))){
            if(asize <= GET_SIZE(HDRP(bp))){
                tmp = bp;
            }
        }

        // for(bp = seg_list[i]; bp != NULL; bp = GET(SUCC(bp))){
        //     if(asize <= GET_SIZE(HDRP(bp))){
        //         if (tmp == NULL){
        //             tmp = bp;
        //         }else{
        //             if(GET_SIZE(tmp) > GET_SIZE(HDRP(bp))){
        //                 tmp = bp;
        //             }
        //         } 
        //     }
        // }
        if (tmp != NULL){
            return tmp;
        }    
        i++; //해당 seg_list가 null일 경우 다음 리스트 검색?
    }
    return NULL;
// #endif 뭐야넌
}

static void place(void *bp, size_t asize){ //가용리스트 주소, 할당할 양

    delete_block(bp);

    size_t csize = GET_SIZE(HDRP(bp));

    // PUT(HDRP(bp), PACK(csize,1));
    // PUT(FTRP(bp), PACK(csize,1));

    if((csize - asize) >= (2*DSIZE)){
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize,0));
        PUT(FTRP(bp), PACK(csize-asize,0));

        coalesce(bp);
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
        return bp;
    }

    extendsize = MAX(asize,CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp,asize);
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

void delete_block(char* bp){
     int seg_list_num = get_seg_list_num(GET_SIZE(HDRP(bp)));

    //  // Select segregated list 
    // while ((seg_list_num < LIST_NUM - 1) && (size > 1)) {
    //     size >>= 1;
    //     seg_list_num++;
    // }

    if(GET(PRED(bp)) == NULL){ //내가 리스트 맨앞이다.
        if(GET(SUCC(bp)) == NULL){
            seg_list[seg_list_num] = NULL;
        }else{
            PUT(PRED(GET(SUCC(bp))),NULL); 
            seg_list[seg_list_num] = GET(SUCC(bp));
        }
    }else{
        if(GET(SUCC(bp)) == NULL){
            PUT(SUCC(GET(PRED(bp))),NULL);
        }else{
            PUT(PRED(GET(SUCC(bp))),GET(PRED(bp)));
            PUT(SUCC(GET(PRED(bp))),GET(SUCC(bp)));
        }
    }

    return;
}

void add_free_block(char* bp){ // 해제된 블럭 알맞은 크기 리스트에 넣어주기
    int seg_list_num = get_seg_list_num(GET_SIZE(HDRP(bp)));
    if(seg_list[seg_list_num] == NULL){
        PUT(PRED(bp),NULL);
        PUT(SUCC(bp),NULL);
    }else {
        PUT(PRED(bp),NULL); // 맨앞에 넣어주기
        PUT(SUCC(bp),seg_list[seg_list_num]);
        PUT(PRED(seg_list[seg_list_num]),bp);
    }
    seg_list[seg_list_num] = bp; //맨앞주소로 저장
}

int get_seg_list_num(size_t size){
    int i = -4; //seg_list[0]은 블록의 최소 크기인 2**4를 위한 리스트 
    while (size != 1){
        size = (size >> 1);
        i++;
    }
    return i;
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