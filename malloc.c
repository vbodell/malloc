/******************************************************************************
malloc.c
homemade, moonshined, malloc substitute

Author: Victor Bodell
Started on: 09/20/2017
First release: TBA

******************************************************************************/


#include <stdio.h>
#include <stdint.h>
#include <errno.h> /* For error handling, including EXIT_FAILURE */
#include <unistd.h> /* For syscall declarations */
#include "malloc.h"

#define BIGGESTREQUEST INTPTR_MAX-ALIGNER-HEADERSIZE

#define ALIGNER 16 /*To make sure memory is in alignment*/



/*Shrink chunk c and create new chunk out of remaining*/
void shrink(struct chunk *c, size_t sizerequest){
  if(c->chunksize <= sizerequest){
    /*request invalid*/
    return;
  }
  if( (c->chunksize-sizerequest) > HEADERSIZE ){ /*We can fit new chunk*/
    /*Store next in list*/
    struct chunk *temp = c->next;
    /*let current nodes next be new node*/
    c->next = (struct chunk*) (c->memptr + sizerequest);
    struct chunk *nc = c->next;

    /*Define new chunk*/
    nc->chunksize = c->chunksize - sizerequest - HEADERSIZE,
    nc->isfree = TRUE,
    nc->memptr = (uintptr_t) nc + HEADERSIZE,
    nc->next = temp;
    /*Update shrunk size*/
    c->chunksize = sizerequest;
  }
}


/*Okay, so now this should return some memory when asked for*/
void *malloc(size_t size){
  if(size == 0 || size > (BIGGESTREQUEST)){
    /*Nothing to allocate, or request to big*/
    return NULL;
  }
  /* round size to closest multiple of 16 */
  if(size % ALIGNER){
    size += ALIGNER - (size % ALIGNER);
  }

  /*Memory has not been initialized, get memory from kernel*/
  if(MEM == NULL){
    return more_memory_please(MEM, size);
  }

  struct chunk *node;
  /*Traverse linked list and look for (a) chunk(s) big enough*/
  for(node = MEM; node->next != NULL; node = node->next){

    if( node->isfree && (size <= node->chunksize) ){
      /*Best case, chunk is free & big enough*/
      /*Set chunk to busy */
      node->isfree = FALSE;
      /*create new chunk out of unused memory*/
      shrink(node, size);

      return (void*) node->memptr;
    }
  }

  /*We are now on last chunk! HOW TO HANDLE? GO TO FUNCTION checknode()*/
  if( node->isfree && (size <= node->chunksize) ){
    /*Best case, chunk is free & big enough*/
    /*Set chunk to busy */
    node->isfree = FALSE;
    /*create new chunk out of unused memory*/
    shrink(node, size);

    return (void*) node->memptr;
  }

  /*If we get here there wasn't a chunk large enough, create new ones*/
  return more_memory_please(node, size);
}




void free(void *ptr){
  if(ptr == NULL){
    return;
  }
  // fprintMemory("m.txt");
  /*Get which chunk/header from pointer*/
  struct chunk *c = getchunk( (uintptr_t) ptr);

  if(c == NULL){
    return; /*Memory could not be found in allocation*/
  }

  /*Mark chunk as free*/
  c->isfree = TRUE;

  /*IF adjacent chunk free, merge*/
  mergechunks();

  return;
}





/* Clear and Allocate memory,
  n=number of items
  sz=size of item
  Does the same as malloc except clearing all memory cells before return */
void *calloc(size_t n, size_t sz){
  size_t membytes = n*sz;
  void *vp;

  /*Memory couldn't be allocated*/
  if((vp = malloc(membytes)) == NULL){
    return NULL;
  }

  /*clear all memory cells*/
  int *tp = (int*) vp;
  int i;
  for(i = 0; i<(membytes/sizeof(int)); i++){
    *(tp+i) = 0;
  }

  return vp;
}


/*--------------------REALLOC----------------------------------*/


/*Attempts to merge adjacent chunks starting from c
  to make c->chunksize >= sizerequest
  We only need to check next chunk since every call to free()
  results in adjacent free chunks merging
  Returns TRUE if successful*/
char attemptmerge(struct chunk* c, size_t sizerequest){

  struct chunk *t = c->next;
  /*If next node is free we might be able to merge to get enough memory*/
  if(t->isfree){
    c->next = t->next;
    c->chunksize += t->chunksize+HEADERSIZE;

    if(c->chunksize >= sizerequest){
      /*Before returning, make sure we're not wasting too much memory on user*/
      shrink(c, sizerequest);
      return TRUE;
    }
  }
  /*Adjacent chunk was not sufficient*/
  return FALSE;
}








/*--------------------REALLOC----------------------------------*/


void *realloc(void *ptr, size_t sizerequest){
  /*Checking parameters to determine best course of action*/
  if(ptr == NULL){
    return malloc(sizerequest);
  }
  else if(sizerequest == 0){
    free(ptr);
    return NULL;
  }

  /*Get which chunk/header from pointer*/
  struct chunk *c = getchunk( (uintptr_t) ptr);

  if(c == NULL){
    return NULL; /*Memory could not be found in allocation*/
  }

  /* round size to closest multiple of 16 */
  if(sizerequest % ALIGNER){
    sizerequest += ALIGNER - (sizerequest % ALIGNER);
  }

  /*If we can fit memory in current chunk, shrink & return the pointer*/
  if(c->chunksize > sizerequest){
    /*Just making sure address was allocated by malloc/calloc/realloc*/
    if(c->isfree){
      return NULL; /*memory was not allocated by malloc!*/
    }
    /*release the memory we might not need*/
    shrink(c, sizerequest);
    return (void*) c->memptr;
  }

  /*Chunk too small, attempt to merge w/ adjacent chunk*/
  if(attemptmerge(c, sizerequest)){
    return (void*) c->memptr;
  }

  /*Sadly, chunk was not big enough nor expandable enough, must reallocate*/
  void *vp = malloc(sizerequest);
  if(vp == NULL){
    /*Could not allocate memory!*/
    return NULL;
  }

  /*Copy Memory to new location*/
  int *copy = (int *) c->memptr;
  int *paste = (int *) vp;
  int i;
  for(i = 0; i < (sizerequest/sizeof(int)); i++){
    *(paste+i) = *(copy+i);
  }

  /*Finally, set original ptr free*/
  free(ptr);
  /*vp now contains original memory*/
  return vp;
}
