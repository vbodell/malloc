/******************************************************************************
myalloc.h
homemade malloc substitute, header file containing syscall & data structure

Author: Victor Bodell

******************************************************************************/


/*----------------CHUNKS-------------------*/

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

#define ALIGNER 16 /*To make sure memory is in alignment*/

/* Roundup to next multiple of 16 if size of chunk is not already */
static int roundup = ALIGNER -
        ((sizeof(struct chunk) % ALIGNER) ?
              sizeof(struct chunk) % ALIGNER : ALIGNER);

/*Define size of chunk header*/
#define HEADERSIZE (sizeof(struct chunk) + roundup)

//declare memory buffer, HOW TO initialize??
static struct chunk *MEM;


/*----------------CHUNKS--END------------------*/

/*-----------CORE-MEMORY-PARAMS---------------*/

/*Lowest break*/
static uintptr_t BREAK = 0;

/*Upper Limit before we need sbrk*/
static uintptr_t UPPERLIM = 0;

/* A big piece is a CAKE, which we turn into chunks (Pizza-slice?)*/
#define CAKESIZE 64000

/*Define sbrk(2) error-value*/
#define SBRKERR (struct chunk*)-1

/*-----------CORE-MEMORY-PARAMS--END--------------*/


/*To improve readability*/
#define TRUE 1
#define FALSE 0


/*Function declarations*/
void fprintMemory(char * fname);



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


/*Gets the chunk specified from address
returns null if address is not amongst chunks*/
struct chunk *getchunk(uintptr_t address){
  struct chunk *temp = MEM; /*chunk* for traversing*/

  /*Checking if address is within previously allocated memory*/
  if((address < temp->memptr) || (address > UPPERLIM))
    return NULL; /*if not we return null*/

  for(; temp->next; temp=temp->next){
   if( (address >= temp->memptr) && (address < (uintptr_t) temp->next) ){
      /*Address is between two adjacent nodes, belongs to first of them*/
      return temp;
    }
  }

  /*we're on last node*/
  if((address >= temp->memptr) && (address <= UPPERLIM)){
    return temp;
  }

  /*If we end up here address specified was in a HEADER, wrong argument*/
  return NULL;
}


/*A function to print information about malloc memory*/
void fprintMemory(char * fname)
{
  FILE *fp;

  fp = fopen(fname, "w+");

  struct chunk *t = MEM;
  char str1[80];

  while(t->next){

    sprintf(str1, "Chunk: {sz=%lu, free=%d, &mem=%lx, &next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);
    fputs(str1, fp);

    t = t->next;
  }
  /*t now points to the last node in the chain but we have not printed it.*/

  sprintf(str1, "Chunk: {sz=%lu, free=%d, &mem=%lx, &next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);
  fputs(str1, fp);
  fclose(fp);
}
