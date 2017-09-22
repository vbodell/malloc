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
#define CAKESIZE 2048

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



  struct chunk *t = newbreak;
  int remaining_CAKE = UPPERLIM - (newbreak->memptr + newbreak->chunksize + HEADERSIZE);

  if(remaining_CAKE > 0){/*Create chunk out of remaining memoryspace*/
    t->next = (struct chunk*) (newbreak->memptr + newbreak->chunksize);
    t = t->next;

    nbrkptr = (uintptr_t) t;
    t->chunksize = remaining_CAKE;
    t->isfree = TRUE;
    t->memptr = nbrkptr + HEADERSIZE;

    /*no need to increment traverse variables since we are done*/
  }


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
