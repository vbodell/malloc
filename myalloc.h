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
static uintptr_t UPPERLIM = 0;

/* A big piece is a CAKE, which we turn into chunks (Pizza-slice?)*/
#define CAKESIZE 512

/*Define sbrk(2) error-value*/
#define SBRKERR (struct chunk*)-1

/*To improve readability*/
#define TRUE 1
#define FALSE 0



void *more_memory_please(struct chunk *node, uintptr_t sizerequest){
  struct chunk *newbreak;

  uintptr_t memrequest = (sizerequest+HEADERSIZE) > CAKESIZE ?
          sizerequest+HEADERSIZE : CAKESIZE;
  if((newbreak = (struct chunk*) sbrk(memrequest)) == SBRKERR){
    /*sbrk(2) failed*/
    errno = ENOMEM;
    return NULL;
  }
  /*pointer for arithmetics*/
  uintptr_t nbrkptr = (uintptr_t) newbreak;
  /*If First BREAK has not been set, set it*/
  if(!BREAK)
    BREAK = nbrkptr;

  /*Set current upper limit*/
  UPPERLIM = nbrkptr + memrequest;
  printf("UL %lx\n", UPPERLIM);
  printf("brk %p\n", newbreak);
  printf("brk %lx\n", nbrkptr);
  // /*current address for user memory*/
  // uintptr_t current_user_memory = nbrkptr+HEADERSIZE;

  /*Correctly initialize starting chunk to correct place in memory*/
  newbreak->chunksize = sizerequest,
  newbreak->isfree = FALSE,
  newbreak->memptr = nbrkptr+HEADERSIZE;


  /*If node is NULL, there is no root & memory has not been initialized*/
  if(node == NULL){
    MEM = newbreak;
  }
  else{ /*We have traversed linked list already*/
    node->next = newbreak;
  }

  // /* node used for traverse*/
  // struct chunk *temp = newbreak;

  // /*set memory pointers to next position*/
  // uintptr_t chunk_tot_sz = memrequest + HEADERSIZE;
  // current_user_memory += chunk_tot_sz;

  struct chunk *t = newbreak;
  int remaining_CAKE = UPPERLIM - (newbreak->memptr + newbreak->chunksize + HEADERSIZE);
  printf("UL %lx mptr %lx sz %lx h %lx\n", UPPERLIM, newbreak->memptr, newbreak->chunksize, HEADERSIZE);
  printf("RC %d\n", remaining_CAKE);
  if(remaining_CAKE > 0){/*Create chunk out of remaining memoryspace*/
    t->next = (struct chunk*) (newbreak->memptr + newbreak->chunksize);
    t = t->next;

    nbrkptr = (uintptr_t) t;
    t->chunksize = remaining_CAKE;
    t->isfree = TRUE;
    t->memptr = nbrkptr + HEADERSIZE;

    /*no need to increment traverse variables since we are done*/
  }


  /*HOW TO ASSIGN CHUNKS?? IMPROVE FORMULA!!*/
  /*Should probably just make next chunk out of remnant, let malloc do the rest*/
  // for(i = 1; (current_user_memory+ALIGNER*i+HEADERSIZE) < UPPERLIM; i++){
  //   i = (i%129); /*Maximum chunksize will be 128*16=2048 bytes*/
  //   int thissize = ALIGNER*i;
  //   chunk_tot_sz = HEADERSIZE + thissize;
  //
  //   /*Set address for next, and move to it for initialization*/
  //   temp->next = (struct chunk*) current_header;
  //   temp = temp->next;
  //   /*move to next header*/
  //   current_header += chunk_tot_sz;
  //
  //   /*Give chunk memorysize, set free=TRUE, pointer=current_user_memory+header+sizerequest, */
  //   temp->chunksize = thissize,
  //   temp->isfree = TRUE,
  //   temp->memptr = current_user_memory;
  //
  //   /*Increment current_user_memory to next address for user-memory*/
  //   current_user_memory += chunk_tot_sz;
  // }


  return (void*) newbreak->memptr;
}



void fprintMemory(char * fname)
{
  FILE *fp;

  fp = fopen(fname, "w+");

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
