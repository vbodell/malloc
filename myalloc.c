/******************************************************************************
myalloc.c
homemade, moonshined, malloc substitute

Author: Victor Bodell
Started on: 09/20/2017
First release: TBA

******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <errno.h> /* For error handling, including EXIT_FAILURE */
#include <unistd.h> /* For syscall declarations */
#include "myalloc.h"

#define BIGGESTREQUEST INTPTR_MAX-ALIGNER-HEADERSIZE

#define ALIGNER 16 /*To make sure memory is in alignment*/


/*Okay, so now this should return some memory when asked for*/
void *myalloc(size_t sizerequest){
  if(sizerequest == 0 || sizerequest > (BIGGESTREQUEST)){
    /*Nothing to allocate, or request to big*/
    return NULL;
  }

  /* round size to closest multiple of 16 */
  if(sizerequest % ALIGNER){
    sizerequest += ALIGNER - (sizerequest % ALIGNER);
    printf("size aligned: %lu\n", sizerequest);
  }


  /*Memory has not been initialized, get memory from kernel*/
  if(MEM == NULL){
    return more_memory_please(MEM, sizerequest);
  }

  struct chunk *node;
  /*Traverse linked list and look for (a) chunk(s) big enough*/
  for(node = MEM; node->next != NULL; node = node->next){

    /*Checking node could be a funct w/ params:
    node, sizerequest, ??*/


    if( node->isfree && (sizerequest <= node->chunksize) ){
      /*Best case, chunk is free & big enough*/
      /*Set chunk to busy */
      node->isfree = FALSE;
      /*create new chunk out of unused memory*/
      if(sizerequest+HEADERSIZE < node->chunksize){
        struct chunk *temp = node->next;


        node->next = (struct chunk*) (node->memptr + sizerequest);
        struct chunk *newnode = node->next;

        newnode->chunksize = node->chunksize - sizerequest - HEADERSIZE,
        newnode->isfree = TRUE,
        newnode->memptr = (uintptr_t) newnode + HEADERSIZE,
        newnode->next = temp;

        node->chunksize = sizerequest;
      }

      return (void*) node->memptr;
    }
  }

  /*We are now on last chunk! HOW TO HANDLE? GO TO FUNCTION checknode()*/
  if( node->isfree && (sizerequest <= node->chunksize) ){
    /*Best case, chunk is free & big enough*/
    /*Set chunk to busy */
    node->isfree = FALSE;
    /*create new chunk out of unused memory*/
    if(sizerequest+HEADERSIZE < node->chunksize){
      struct chunk *temp = node->next;


      node->next = (struct chunk*) (node->memptr + sizerequest);
      struct chunk *newnode = node->next;

      newnode->chunksize = node->chunksize - sizerequest - HEADERSIZE,
      newnode->isfree = TRUE,
      newnode->memptr = (uintptr_t) newnode + HEADERSIZE,
      newnode->next = temp;

      node->chunksize = sizerequest;
    }
    return (void*) node->memptr;
  }

  /*If we get here there wasn't a chunk large enough, create new ones*/
  return more_memory_please(node, sizerequest);
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
  if((vp = myalloc(membytes)) == NULL){
    return NULL;
  }

  /*clear all memory cells*/
  int *tp = (int*) vp;
  for(int i = 0; i<(membytes/sizeof(int)); i++){
    *(tp+i) = 0;
  }

  return vp;
}



int main(){

  /*LOOK INTO NEGATIVE NUMBERS-UNSIGNED, INCLUDES LARGE CHUNKS...*/
  // void *t1 = myalloc(INTPTR_MAX-17);
  // printf("Test1: sz=-4, p=%p\n", t1);
  // void *t2 = myalloc(0);
  // printf("Test2: sz=0, p=%p\n", t2);
  // void *t3 = myalloc(1);
  // printf("Test3: sz=1, line above should read alignment p=%p\n", t3);
  //

  char *str;
  str = (char*) myalloc(400);
  // int *tp = str;
  // if(str) printf("Test4: Successfully allocated char*, p=%p\n", str);
  //
  *str = 'h';
  *(str+1) = 'e';
  *(str+2) = 'l';
  *(str+3) = 'l';
  *(str+4) = 'o';
  *(str+5) = '\0';

  printf("\n%s, %p\n\n", str, str);

  int *ip = (int*) myalloc(31);
  struct chunk *t = getchunk((uintptr_t)ip);
  printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);

  double *dp = (double*) myalloc(100);

  void *vp = myalloc(150);
  printf("Test5: allocated multiple pointers within chunks, ip=%p, dp=%p, vp=%p\n", ip, dp, vp);
  // printf("BRK: %p, UL: %p \n", (void*)BREAK, (void*)UPPERLIM);
  void *vpn = myalloc(37);
  uintptr_t req = (uintptr_t)vpn + 48+HEADERSIZE;
  printf("%p\n", req);
  if((t = getchunk(req)))
    printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);

  fprintMemory("before.txt");
  void *vpn2 = myalloc(1024);

  fprintMemory("memoryprint.txt");

  free(str);
  free(dp);
  free(vp);
  free(ip);
  // mergechunks();
  fprintMemory("merged.txt");

  printf("BRK: %p, UL: %p \n", (void*)BREAK, (void*)UPPERLIM);
  return 0;
}





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


/*Shrink chunk c and create new chunk out of remaining*/
void shrink(struct chunk *c, size_t sizerequest){
  if(c->chunksize <= sizerequest){
    /*request invalid*/
    return;
  }
  if( (c->chunksize-sizerequest) > HEADERSIZE ){ /*We can fit new chunk*/
    struct chunk *temp = c->next;
    c->next = (struct chunk*) (c->memptr + sizerequest);
    struct chunk *nc = c->next;

    nc->chunksize = c->chunksize - sizerequest - HEADERSIZE,
    nc->isfree = TRUE,
    nc->memptr = (uintptr_t) nc + HEADERSIZE,
    nc->next = temp;
    c->chunksize = sizerequest;
  }
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
  void *vp = myalloc(sizerequest);
  if(vp == NULL){
    /*Could not allocate memory!*/
    return NULL;
  }
  /*define pointers for copying*/
  int *copy = (int *) c->memptr;
  int *paste = (int *) vp;

  for(int i = 0; i < (sizerequest/sizeof(int)); i++){
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
