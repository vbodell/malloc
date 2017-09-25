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

  /*IF adjacent chunk is free, merge*/
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
  returns TRUE if successful*/
char attemptmerge(struct chunk* c, size_t sizerequest){
  /*If next node is free we might be able to merge to get enough memory*/
  uintptr_t accsize = c->chunksize;
  struct chunk *t = c->next;

  /*merging for as long as possible*/
  while(t->next && t->next->isfree){
    /*If adjacent node is free we accumulate its size and header*/
    accsize += t->chunksize+HEADERSIZE;

    /*clear memory in between nodes and expand memory of original chunk*/
    c->next = t->next;
    c->chunksize = accsize;

    if(accsize >= sizerequest){
      /*Bingo, merged so that adjacent cells are free*/
      /*Return original chunk now expanded*/
      return TRUE;
    }
    t = t->next;
  }
  /*We are on final node which still might work*/
  if(t->isfree){
    accsize += t->chunksize+HEADERSIZE;
    /*c is now last in chain*/
    c->next = NULL;
    c->chunksize = accsize;

    if(accsize >= sizerequest){
      return TRUE;
    }
  }
  /*Adjacent chunks were not sufficient*/
  return FALSE;
}



void *realloc(void *ptr, size_t sizerequest){
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
    // shrink(c, sizerequest);
    return (void*) c->memptr;
  }
  else if(c->next && c->next->isfree){
    /*If successful, return original memoryspace*/
    if(attemptmerge(c, sizerequest)){
      return (void*) c->memptr;
    }
  }

  /*Sadly chunk was not big enough nor expandable enough, must reallocate*/
  void *vp = malloc(sizerequest);
  if(vp == NULL){
    /*Could not allocate memory!*/
    return NULL;
  }
  /*define pointers for copying*/
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


  /*
  realloc(3) must try to minimize copying. That is, it must attempt
  in-place expansion (or shrinking) if it is possible, including merging
  with adjacent free chunks, if any

  Also, remember, if realloc(3) is called with NULL it does the same
  thing as malloc(3), and if realloc(3) is called with a size of 0
  and the pointer is not NULL, it’s equivalent to free(ptr)
  */
}
