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

  /*IF adjacent chunks are free, merge*/
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
  for(int i = 0; i<(membytes/sizeof(int)); i++){
    *(tp+i) = 0;
  }

  return vp;
}



/*Attempts to merge adjacent chunk starting from c
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



int main(){
  // void *t1 = myalloc(INTPTR_MAX-17);
  // printf("Test1: sz=-4, p=%p\n", t1);
  // void *t2 = myalloc(0);
  // printf("Test2: sz=0, p=%p\n", t2);
  // void *t3 = myalloc(1);
  // printf("Test3: sz=1, line above should read alignment p=%p\n", t3);
  //

  char *str;
  str = (char*) malloc(400);
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

  int *ip = (int*) malloc(31);
  struct chunk *t = getchunk((uintptr_t)ip);
  printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);

  double *dp = (double*) malloc(100);

  void *vp = malloc(150);
  printf("Test5: allocated multiple pointers within chunks, ip=%p, dp=%p, vp=%p\n", ip, dp, vp);
  // printf("BRK: %p, UL: %p \n", (void*)BREAK, (void*)UPPERLIM);
  // void *vpn = malloc(37);
  // uintptr_t req = (uintptr_t)vpn + 48+HEADERSIZE;
  // printf("%p\n", req);
  // if((t = getchunk(req)))
  //   printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);

  fprintMemory("before.txt");
  void *vpn2 = malloc(1024);

  void *vpn3 = malloc(200);

  fprintMemory("memoryprint.txt");

  // free(str);
  // free(dp);
  free(vp);
  // free(ip);
  struct chunk *c = getchunk((uintptr_t)vp);
  printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", c->chunksize, c->isfree, c->memptr, c->next);
  // attemptmerge(c, 176);


  c = getchunk(vpn3);
  printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", c->chunksize, c->isfree, c->memptr, c->next);
  // if(attemptmerge(NULL, 160)){
  //   printf("ja\n");
  // }
  if(attemptmerge(c, 224)){
    printf("ja\n");
  }
  fprintMemory("merged.txt");
  if(attemptmerge(c, 256)){
    printf("ja\n");
  }
  fprintMemory("merag.txt");
  printf("BRK: %p, UL: %p \n", (void*)BREAK, (void*)UPPERLIM);
  return 0;
}




/*--------------------REALLOC----------------------------------*/






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
