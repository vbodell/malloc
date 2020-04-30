/******************************************************************************
malloc.c
homemade, moonshined, malloc substitute
Contains the functions malloc, free, calloc & realloc respectively
Helper-functions and datastructure found in header-file, malloc.h

Author: Victor Bodell
Started on: 09/20/2017
First release: 09/25/2017

******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h> /* For error handling, including EXIT_FAILURE */
#include <unistd.h> /* For syscall declarations */
#include "malloc.h"

#define BIGGESTREQUEST INTPTR_MAX-ALIGNER-HEADERSIZE

#define ALIGNER 16 /*To make sure memory is in alignment*/

#define BUF_SZ 70

static char firstRun = TRUE;
static char DEBUG = FALSE;

/*Okay, so now this should return some memory when asked for*/
void *malloc(size_t size){

  if(firstRun){
    if(getenv("DEBUG_MALLOC")){
      DEBUG = TRUE;
    }
    firstRun=FALSE;
  }

  if(size == 0 || size > (BIGGESTREQUEST)){
    /*Nothing to allocate, or request to big*/
    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: malloc(%lu) => (ptr=%p, size=%d)",
        size, NULL, 0);
      puts(str);
    }
    return NULL;
  }
  /* round size to closest multiple of 16 */
  int alignedSize = size;
  if(alignedSize % ALIGNER){
    alignedSize = size + ALIGNER - (size % ALIGNER);
  }

  /*Memory has not been initialized, get memory from kernel*/
  if(MEM == NULL){
    void *ptr = more_memory_please(MEM, alignedSize);
    if(DEBUG){
      char str[BUF_SZ];
      struct chunk *t = getchunk((uintptr_t)ptr);
      snprintf(str, BUF_SZ, "MALLOC: malloc(%lu) => (ptr=%p, size=%d)",
        size, ptr, (int) t->chunksize);
      puts(str);
    }
    return ptr;
  }

  struct chunk *node;
  /*Traverse linked list and look for (a) chunk(s) big enough*/
  for(node = MEM; node->next != NULL; node = node->next){

    if( node->isfree && (alignedSize <= node->chunksize) ){
      /*Best case, chunk is free & big enough*/
      /*Set chunk to busy */
      node->isfree = FALSE;
      /*create new chunk out of unused memory*/
      shrink(node, alignedSize);

      if(DEBUG){
        char str[BUF_SZ];
        snprintf(str, BUF_SZ, "MALLOC: malloc(%lu) => (ptr=%p, size=%d)",
          size, (void *)node->memptr, (int) node->chunksize);
        puts(str);
      }

      return (void*) node->memptr;
    }
  }

  /*We are now on last chunk! HOW TO HANDLE? GO TO FUNCTION checknode()*/
  if( node->isfree && (alignedSize <= node->chunksize) ){
    /*Best case, chunk is free & big enough*/
    /*Set chunk to busy */
    node->isfree = FALSE;
    /*create new chunk out of unused memory*/
    shrink(node, alignedSize);

    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: malloc(%lu) => (ptr=%p, size=%d)",
        size, (void*) node->memptr, (int) node->chunksize);
      puts(str);
    }

    return (void*) node->memptr;
  }
  void *ptr = more_memory_please(node, alignedSize);

  if(DEBUG){
    char str[BUF_SZ];
    struct chunk *t = getchunk((uintptr_t)ptr);
    snprintf(str, BUF_SZ, "MALLOC: malloc(%lu) => (ptr=%p, size=%d)",
      size, ptr, (int) t->chunksize);
    puts(str);
  }
  /*If we get here there wasn't a chunk large enough, create new ones*/
  return ptr;
}

void free(void *ptr){
  if(ptr == NULL){
    return;
  }
  if(DEBUG){
    char str[BUF_SZ];
    snprintf(str, BUF_SZ, "MALLOC: free(%p)", ptr);
    puts(str);
  }
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
    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: calloc(%lu,%lu) => (ptr=%p, size=%d)",
        n, sz, NULL, 0);
      puts(str);
    }
    return NULL;
  }

  /*clear all memory cells*/
  memset(vp, 0, membytes);


  if(DEBUG){
    char str[BUF_SZ];
    struct chunk *t = getchunk((uintptr_t)vp);
    snprintf(str, BUF_SZ, "MALLOC: calloc(%lu,%lu) => (ptr=%p, size=%d)",
      n, sz, vp, (int)t->chunksize);
    puts(str);
  }
  return vp;
}

void *realloc(void *ptr, size_t sizerequest){
  int oldsize = sizerequest;
  /*Checking parameters to determine best course of action*/
  if(ptr == NULL){
    return malloc(sizerequest);
  }
  else if(sizerequest == 0){
    free(ptr);
    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: realloc(%p,%d) => (ptr=%p, size=%d)",
        ptr, oldsize, NULL, 0);
      puts(str);
    }
    return NULL;
  }

  /*Get which chunk/header from pointer*/
  struct chunk *c = getchunk( (uintptr_t) ptr);

  if(c == NULL){
    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: realloc(%p,%zu) => (ptr=%p, size=%d)",
        ptr, sizerequest, NULL, 0);
      puts(str);
    }
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
      if(DEBUG){
        char str[BUF_SZ];
        snprintf(str, BUF_SZ, "MALLOC: realloc(%p,%zu) => (ptr=%p, size=%d)",
          ptr, sizerequest, NULL, 0);
        puts(str);
      }
      return NULL; /*memory was not allocated by malloc!*/
    }
    /*release the memory we might not need*/
    shrink(c, sizerequest);
    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: realloc(%p,%d) => (ptr=%p, size=%d)",
        ptr, oldsize, (void*)c->memptr, (int)c->chunksize);
      puts(str);
    }
    return (void*) c->memptr;
  }

  /*Chunk too small, attempt to merge w/ adjacent chunk*/
  if(attemptmerge(c, sizerequest)){
    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: realloc(%p,%d) => (ptr=%p, size=%d)",
        ptr, oldsize, (void*)c->memptr, (int)c->chunksize);
      puts(str);
    }
    return (void*) c->memptr;
  }

  /*Sadly, chunk was not big enough nor expandable enough, must reallocate*/
  void *vp = malloc(sizerequest);
  if(vp == NULL){
    /*Could not allocate memory!*/
    if(DEBUG){
      char str[BUF_SZ];
      snprintf(str, BUF_SZ, "MALLOC: realloc(%p,%d) => (ptr=%p, size=%d)",
        ptr, oldsize, NULL, 0);
      puts(str);
    }
    return NULL;
  }

  /*Copy Memory to new location*/
  char *copy = (char *) c->memptr;
  int i;
  for(i = 0; i < sizerequest; i++){
    memset(vp+i, *(copy+i), 1);
  }

  /*Finally, set original ptr free*/
  free(ptr);
  /*vp now contains original memory*/
  if(DEBUG){
    char str[BUF_SZ];
    struct chunk *t;
    t = getchunk((uintptr_t)vp);
    snprintf(str, BUF_SZ, "MALLOC: realloc(%p,%d) => (ptr=%p, size=%d)",
      ptr, oldsize, (void*)t->memptr, (int)t->chunksize);
    puts(str);
  }
  return vp;
}
