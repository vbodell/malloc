//UPON COMPLETION, DELETE PRINTS & REPLACE MYALLOC


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


void fprintMemory()
{
  FILE *fp;

  fp = fopen("memoryprint.txt", "w+");

  struct chunk *t = MEM;
  char str1[80];

  while(t->next){

    sprintf(str1, "Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);
    fputs(str1, fp);

    t = t->next;
  }
  /*t now points to the last node in the chain but we have not printed it.*/

  sprintf(str1, "Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);
  fputs(str1, fp);
  fclose(fp);
}


int main(){
  void *vp;
  vp = more_memory_please(NULL, 16);


  struct chunk *t;
  for(t = MEM; t->next; t=t->next);

  more_memory_please(t, 64);


  fprintMemory();

  return 0;
}

/*
int oldcode(){
  int a = 16 - ((sizeof(struct chunk)) % 16 ? sizeof(struct chunk) % 16 : 16);
  printf("size: %lu\n", HEADERSIZE);
  printf("a: %d\n", a);

  void *adr;
  if((adr = sbrk(0)) == (int*)-1){
    perror("sbrk(2), something went wrong...");
    return 1;
  }
  uintptr_t t = adr;
  char *k = t;
  char debugStr[80];
  sprintf(debugStr, "Successfully allocated memory w/ address: %p inc %p\n", t, ++t);
  write(1, debugStr, 80);

  // adr = (int *) adr;
  // *adr = 99;
  // *(++adr) = 95;
  sprintf(debugStr, "Add 1 to address: %p, %d prev %d\n", ++adr);
  write(1, debugStr, 80);

  //Only prints if MEM has been initialized
  if(MEM){
    printf("JA\n");
  }
  else
    printf("NEJ\n");



  int size = 16;
  // if(sbrk((2*(size+HEADERSIZE))) == -1){
  //   perror("wrong");
  // }

  struct chunk *trav;
  struct chunk t = {size, 1, size+HEADERSIZE, NULL};
  MEM = &t;
  trav = &t;
  // Must assign before going to null!
  struct chunk t2 = {size*2, 0, size+HEADERSIZE, NULL};
  trav->next = &t2;
  trav = trav->next;

  struct chunk t3 = {size*10, 1, size+HEADERSIZE, NULL};
  trav->next = &t3;
  trav = trav->next;

  struct chunk t4 = {size*9, 0, size+HEADERSIZE, NULL};
  trav->next = &t4;
  trav = trav->next;

  printf("3\n");
  trav = MEM;
  while(trav != NULL){
    printf("Chunk: {s=%lu, free=%d, adress=%p}\n", trav->size, trav->isfree, trav->address);
    trav = trav->next;
  }

}
*/
