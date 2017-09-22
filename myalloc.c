/******************************************************************************
myalloc.c
homemade malloc substitute

Author: Victor Bodell
Started on: 09/20/2017
First working version: TBA

******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <errno.h> /* For error handling, including EXIT_FAILURE */
#include <unistd.h> /* For syscall declarations */

/* DATA STRUCTURE FOR MEMORY CHUNKS
size: contains sizeof memory-map (exluding sizeof chunk)
isfree: is chunk of memory free to use?
memptr: pointer to actual memory space
next: pointer to next chunk header */
struct chunk{
  uintptr_t chunksize;
  char isfree;
  uintptr_t memptr;
  struct chunk *next;
};

#define ALIGNER 16

/* Roundup to next multiple of 16 if size of chunk is not already */
static int roundup = ALIGNER -
        ((sizeof(struct chunk) % ALIGNER) ?
              sizeof(struct chunk) % ALIGNER : ALIGNER);
/*Define size of chunk header*/
#define HEADERSIZE (sizeof(struct chunk) + roundup)

//declare memory buffer, HOW TO initialize??
static struct chunk *MEM;

/*Lowest break*/
static uintptr_t BREAK = 0;

/*Upper Limit before we need sbrk*/
static uintptr_t UPPERLIM;

/* A big piece is a CAKE, which we turn into chunks (Pizza-slice?)*/
#define CAKESIZE 1024

/*Define sbrk(2) error-value*/
#define SBRKERR (struct chunk*)-1

/*To improve readability*/
#define TRUE 1
#define FALSE 0



void *more_memory_please(struct chunk *node, size_t sizerequest){
  struct chunk *newbreak;
  if((newbreak = (struct chunk*) sbrk(CAKESIZE)) == SBRKERR){
    /*sbrk(2) failed*/
    errno = ENOMEM;
    return NULL;
  }

  /*If First BREAK has not been set, set it*/
  if(!BREAK)
    BREAK = (uintptr_t) newbreak;

  UPPERLIM = (uintptr_t) newbreak + CAKESIZE;
  /*current address for user memory & header (only offset is header)*/
  uintptr_t current_user_memory = (uintptr_t) newbreak+HEADERSIZE;
  uintptr_t current_header = (uintptr_t) newbreak;

  /*Correctly initialize starting chunk to correct place in memory*/
  newbreak->chunksize = sizerequest,
  newbreak->isfree = FALSE,
  newbreak->memptr = current_user_memory;


  /*If node is NULL, there is no root & memory has not been initialized*/
  if(node == NULL){
    MEM = newbreak;
  }
  else{ /*We have traversed linked list already*/
    node->next = newbreak;
  }

  /* node used for traverse*/
  struct chunk *temp = newbreak;

  /*set memory pointers to next position*/
  uintptr_t chunk_tot_sz = sizerequest + HEADERSIZE;
  current_user_memory += chunk_tot_sz;
  current_header += chunk_tot_sz;

  int i;
  /*HOW TO ASSIGN CHUNKS?? IMPROVE FORMULA!!*/
  for(i = 1; (current_user_memory+ALIGNER*i+HEADERSIZE) < UPPERLIM; i++){
    i = (i%129); /*Maximum chunksize will be 128*16=2048 bytes*/
    int thissize = ALIGNER*i;
    chunk_tot_sz = HEADERSIZE + thissize;

    /*Set address for next, and move to it for initialization*/
    temp->next = (struct chunk*) current_header;
    temp = temp->next;
    /*move to next header*/
    current_header += chunk_tot_sz;

    /*Give chunk memorysize, set free=TRUE, pointer=current_user_memory+header+sizerequest, */
    temp->chunksize = thissize,
    temp->isfree = TRUE,
    temp->memptr = current_user_memory;

    /*Increment current_user_memory to next address for user-memory*/
    current_user_memory += chunk_tot_sz;
  }


  uintptr_t remaining_CAKE = UPPERLIM - current_user_memory;
  if(remaining_CAKE){/*Create chunk out of remaining memoryspace*/
    temp->next = (struct chunk*) current_header;
    temp = temp->next;

    temp->chunksize = remaining_CAKE;
    temp->isfree = TRUE;
    temp->memptr = current_user_memory;

    /*no need to increment traverse variables since we are done*/
  }


  return (void*) newbreak->memptr;
}




/*Okay, so now this should return some memory, but sbrk2 should probably
be modified to correctly address where headers are put...*/
void *malloc(size_t sizerequest){
  if(sizerequest <= 0){
    /*Nothing to allocate*/
    return NULL;
  }
  /* round size to closest multiple of 16 */
  if(sizerequest % 16){
    sizerequest = sizerequest + 16 - (sizerequest % 16);
  }


  /*Memory has not been initialized, get memory from kernel*/
  if(MEM == NULL){
    return more_memory_please(MEM, sizerequest);
  }

  /*Accumulator to make sure we can use adjacent nodes*/
  int accsize = -HEADERSIZE; /*account for header of first chunk*/
  struct chunk *firstAccNode = MEM;

  struct chunk *node;
  /*Traverse linked list and look for (a) chunk(s) big enough*/
  for(node = MEM; node->next != NULL; node = node->next){

    /*Checking node could be a funct w/ params:
    node, sizerequest, &accsize, firstAccNode*/


    if( node->isfree && (sizerequest <= node->chunksize) ){
      /*Best case, chunk is free & big enough*/
      /*Set chunk to busy & return pointer to memoryaddress*/
      node->isfree = FALSE;
      return (void*) node->memptr;
    }

    else if(node->isfree){
      /*There might be adjacent free chunks, check if sizeof them is enough*/
      accsize += node->chunksize+HEADERSIZE;

      if(firstAccNode == NULL){
        firstAccNode = node; /*first node on "free-streak"*/
      }

      if(sizerequest <= accsize){
        /*Accumulated enough memory, chunks in between are now garbage*/
        firstAccNode->chunksize = accsize;
        /*redirect next node*/
        firstAccNode->next = node->next;
        /*Set chunk to busy*/
        firstAccNode->isfree = FALSE;
        return (void*) firstAccNode->memptr;
      }
    }

    else{ /* node is not free */
      /*reset accsize & accnode*/
      accsize = -HEADERSIZE;
      firstAccNode = NULL;
    }
  }

  /*We are now on last chunk! HOW TO HANDLE? GO TO FUNCTION checknode()*/
  if( node->isfree && (sizerequest <= node->chunksize) ){
    /*Best case, chunk is free & big enough*/
    /*Set chunk to busy & return pointer to memoryaddress*/
    node->isfree = FALSE;
    return (void*) node->memptr;
  }

  else if(node->isfree){
    /*There might be adjacent free chunks, check if sizeof them is enough*/
    accsize += node->chunksize+HEADERSIZE;

    if(firstAccNode == NULL){
      firstAccNode = node; /*first node on "free-streak"*/
    }

    if(sizerequest <= accsize){
      /*Accumulated enough memory, chunks in between are now garbage*/
      firstAccNode->chunksize = accsize;
      /*redirect next node*/
      firstAccNode->next = node->next;
      /*Set chunk to busy*/
      firstAccNode->isfree = FALSE;
      return (void*) firstAccNode->memptr;
    }
  }


  /*If we get here there wasn't a chunk large enough, create new one*/
  return more_memory_please(node, sizerequest);
}


int main(){
  char *str;
  str = (char*) malloc(6);

  *str = 'h';
  *(str+1) = 'e';
  *(str+2) = 'l';
  *(str+3) = 'l';
  *(str+4) = 'o';
  *(str+5) = '\0';

  write(1, str, 6);

  return 0;
}


// void free(void *ptr){
//   if(ptr == NULL){
//     return;
//   }
//
//   /*
//   each take a pointer to a block of memory allocated by malloc(3),
//   but the pointer will not necessarily be to the very first byte of region.
//   You must support this
//
//   You will have to remember to merge sections of memory if adjacent
//   ones become free.
//
//   realloc(3) must try to minimize copying. That is, it must attempt
//   in-place expansion (or shrinking) if it is possible, including merging
//   with adjacent free chunks, if any
//
//   Also, remember, if realloc(3) is called with NULL it does the same
//   thing as malloc(3), and if realloc(3) is called with a size of 0
//   and the pointer is not NULL, it’s equivalent to free(ptr).2
//   */
// }
//
//
// void *realloc(void *ptr, size_t sizerequest){
//   if(ptr == NULL){
//     return malloc(sizerequest);
//   }
//   else if(sizerequest == 0){
//     free(ptr);
//     return NULL;
//   }
//
// }
//
// void *calloc(size_t nmemb, size_t sizerequest){
//
// }
