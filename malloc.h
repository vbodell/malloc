/******************************************************************************
myalloc.h
homemade malloc substitute, header file containing syscall & data structure
chunks, which simulates pieces of a pie to allocate memory.
Also including functions specific for data structure.

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

/*declare memory buffer*/
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
void shrink(struct chunk *, size_t);

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
    if(node->isfree){/*We can expand from previous node*/
      node->chunksize += newbreak->chunksize + HEADERSIZE;
      node->isfree = FALSE;
      shrink(node, sizerequest); /*Shrink to properly accomodate request*/
      return (void *)node->memptr;
    }
    else{
      node->next = newbreak;
    }
  }

  struct chunk *t = newbreak;
  int remaining_CAKE = UPPERLIM -
    (newbreak->memptr + newbreak->chunksize + HEADERSIZE);

  if(remaining_CAKE > 0){/*Create chunk out of remaining memoryspace*/
    t->next = (struct chunk*) (newbreak->memptr + newbreak->chunksize);
    t = t->next;

    nbrkptr = (uintptr_t) t;
    t->chunksize = remaining_CAKE;
    t->isfree = TRUE;
    t->memptr = nbrkptr + HEADERSIZE;
  }

  return (void*) newbreak->memptr;
}

/*-------------------KERNEL-MEMORY-RETURN-FUNCT------------------*/
/*A function for returning a chunk t to kernel*/
void here_you_go_kernel(struct chunk *t){
  uintptr_t returnPiece = (t->chunksize+HEADERSIZE); /*size to return*/
  if(sbrk(-returnPiece) == SBRKERR){
    /*sbrk(2) failed*/
    errno = ENOMEM;
    return;
  }

  UPPERLIM = (uintptr_t) t; /*Upperlimit is where chunk began*/
  /*Make sure final node doesn't point to returned memory*/
  for(t = MEM; t->next->next; t = t->next);
  t->next = NULL;

  return;
}

/*---------------------------CHUNK-FUNCTIONS--------------------------------*/

void mergechunks(){
  /*Traverse and merge adjacent free chunks*/
  struct chunk *t;

  for(t = MEM; t->next; t=t->next){
    /*If current chunk is free we can start merging*/
    if(t->isfree){
      struct chunk *temp = t->next;

      while(temp && temp->isfree){
        /*If there is a next chunk that also happens to be free*/

        t->chunksize += temp->chunksize+HEADERSIZE; /*increment size of first*/
        /*remove temp from chain (now part of memoryblock)*/
        t->next = temp->next;
        temp = temp->next; /*Move to adjacent free chunk*/
      }
    }
    /*while merging we might be on last node, thus we must break forloop*/
    if(t->next == NULL)
      break;
    /*We have now merged all we can, move to next node*/
  }

  /*We have more than one cake out on the field*/
  if(t->isfree && t->chunksize + HEADERSIZE > CAKESIZE &&
      ((uintptr_t) t - BREAK > CAKESIZE)){
    here_you_go_kernel(t);
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

/*Shrink chunk c and create new chunk out of remaining*/
void shrink(struct chunk *c, size_t sizerequest){

  if(c->chunksize <= sizerequest){
    /*request invalid*/
    return;
  }
  if( (c->chunksize-sizerequest) > HEADERSIZE ){ /*We can fit new chunk*/
    /*Store next in list*/
    struct chunk *temp = c->next;
    /*let next of current node be new node*/
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

/*A function to print information about malloc memory*/
void fprintMemory(char *fname){
  FILE *fp;

  fp = fopen(fname, "w+");

  struct chunk *t = MEM;
  char str1[80];

  while(t->next){

    sprintf(str1, "Chunk: {sz=%lu, free=%d, &mem=%lx, &next=%p}\n",
      t->chunksize, t->isfree, t->memptr, t->next);
    fputs(str1, fp);

    t = t->next;
  }
  /*t now points to the last node in the chain but we have not printed it.*/

  sprintf(str1, "Chunk: {sz=%lu, free=%d, &mem=%lx, &next=%p}\n",
    t->chunksize, t->isfree, t->memptr, t->next);
  fputs(str1, fp);
  fclose(fp);
}
