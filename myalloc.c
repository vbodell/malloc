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
#include "myalloc.h"



void mergechunks(){
  /*Traverse and merge adjacent free chunks*/
  struct chunk *t;

  for(t = MEM; t->next; t=t->next){
    /*If current chunk is free we can start merging*/
    if(t->isfree){
      struct chunk *temp = t->next;

      while(temp && temp->isfree){
        /*If there is a next chunk that also happens to be free*/
        // printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", temp->chunksize, temp->isfree, temp->memptr, temp->next);
        t->chunksize += temp->chunksize+HEADERSIZE; /*increment size of first*/
        t->next = temp->next; /*remove temp from chain (now part of memoryblock)*/
        temp = temp->next; /*Move to adjacent free chunk*/
        // printf("%p\n", temp);
      }
    }
    /*while merging we might be on last node, thus we must break forloop*/
    if(t->next == NULL)
      break;
    /*We have now merged all we can, move to next node*/
  }
  return;
}


/*Okay, so now this should return some memory, but sbrk2 should probably
be modified to correctly address where headers are put...*/
void *myalloc(size_t sizerequest){
  if(sizerequest <= 0){
    /*Nothing to allocate*/
    return NULL;
  }

  /* round size to closest multiple of 16 */
  if(sizerequest % ALIGNER){
    sizerequest = sizerequest + ALIGNER - (sizerequest % ALIGNER);
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


  /*If we get here there wasn't a chunk large enough, create new ones*/
  return more_memory_please(node, sizerequest);
}




int main(){
  char *str;
  str = (char*) myalloc(6);

  *str = 'h';
  *(str+1) = 'e';
  *(str+2) = 'l';
  *(str+3) = 'l';
  *(str+4) = 'o';
  *(str+5) = '\0';

  printf("str=%s, adr=%p\n", str, str);
  fprintMemory("memoryprint.txt");

  mergechunks();
  fprintMemory("merged.txt");


  return 0;
}



/* Clear and Allocate memory,
  n=number of items
  sz=size of item
  Does the same as malloc except clearing all memory cells before return */
void *calloc(size_t n, size_t sz){
  size_t membytes = n*sz;
  void *vp;

  /*Memory couldn't be allocated*/
  if((vp = myalloc(membytes)) == NULL){
    return NULL;
  }


  /*clear all memory cells*/
  long *tp = (long*) vp;
  for(int i = 0; i<(membytes/sizeof(long)); i++){
    *(tp+i) = 0;
  }

  return vp;
}



struct chunk *getchunk(uintptr_t address){
  struct chunk *temp = MEM;
  /*Checking if address is within previously allocated memory*/
  if(address < temp->memptr || address > UPPERLIM)
    return NULL; /*if not we return null*/

  for(; temp->next; temp=temp->next){
    if( (address >= temp->memptr) && (address < temp->next->memptr) ){
      /*Address is between two adjacent nodes, belongs to first of them*/
      return temp;
    }
  }
  /*we're on last node*/
  if((address >= temp->memptr) && (address <= UPPERLIM)){
    return temp;
  }
  /*We can't end up here*/
  printf("I misspoke...\n");
}



void free(void *ptr){
  if(ptr == NULL){
    return;
  }

  /*Get which chunk/header from pointer*/
  struct chunk *c = getchunk( (uintptr_t) ptr);

  if(c == NULL){
    return; /*Memory could not be found in allocation*/
  }

  /*Mark chunk as free*/
  c->isfree = TRUE;

  /*IF adjacent chunk is free, include it?*/
  //mergechunks();

  return;

  /*
  each take a pointer to a block of memory allocated by malloc(3),
  but the pointer will not necessarily be to the very first byte of region.
  You must support this

  You will have to remember to merge sections of memory if adjacent
  ones become free.

  realloc(3) must try to minimize copying. That is, it must attempt
  in-place expansion (or shrinking) if it is possible, including merging
  with adjacent free chunks, if any

  Also, remember, if realloc(3) is called with NULL it does the same
  thing as malloc(3), and if realloc(3) is called with a size of 0
  and the pointer is not NULL, it’s equivalent to free(ptr)
  */
}





void *realloc(void *ptr, size_t sizerequest){
  if(ptr == NULL){
    return myalloc(sizerequest);
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

  /*If we can fit memory in current chunk, just return the pointer*/
  /*HOW ABOUT SHRINKING??*/
  if(c->chunksize > sizerequest){
    /*Just making sure address was allocated by malloc/calloc/realloc*/
    if(!(c->isfree)){
      return (void*) c->memptr;
    }
    else{/*memory was not allocated by malloc!*/
      return NULL;
    }
  }
  else if(c->next && c->next->isfree){

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
        return (void *) c->memptr;
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
        return (void *) c->memptr;
      }
    }
  }

  /*Sadly chunk was not big enough nor expandable enough, must reallocate*/
  void *vp = myalloc(sizerequest);
  if(vp == NULL){
    /*Could not allocate memory!*/
    return NULL;
  }
  /*define pointers for copying*/
  long *copy = (long *) c->memptr;
  long *paste = (long *) vp;

  for(int i = 0; i < (sizerequest/sizeof(long)); i++){
    *(paste+i) = *(copy+i);
  }
  /*vp now contains original memory*/
  return vp;
}
