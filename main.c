

int main(){
  // void *t1 = myalloc(INTPTR_MAX-17);
  // printf("Test1: sz=-4, p=%p\n", t1);
  // void *t2 = myalloc(0);
  // printf("Test2: sz=0, p=%p\n", t2);
  // void *t3 = myalloc(1);
  // printf("Test3: sz=1, line above should read alignment p=%p\n", t3);
  //

  // char *str;
  // str = (char*) malloc(400);
  // int *tp = str;
  // if(str) printf("Test4: Successfully allocated char*, p=%p\n", str);
  //
  // *str = 'h';
  // *(str+1) = 'e';
  // *(str+2) = 'l';
  // *(str+3) = 'l';
  // *(str+4) = 'o';
  // *(str+5) = '\0';
  //
  // printf("\n%s, %p\n\n", str, str);
  //
  // int *ip = (int*) malloc(31);
  // struct chunk *t = getchunk((uintptr_t)ip);
  // printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);
  //
  // double *dp = (double*) myalloc(100);
  //
  // void *vp = malloc(150);
  //
  char *p1 = calloc(10, sizeof(char));
  // float *p2 = calloc(90, sizeof(float));
  // double *c3 = calloc(41940, sizeof(double));
  int *cp4 = calloc(4100, sizeof(int));

  // printf("Test5: allocated multiple pointers within chunks, ip=%p, dp=%p, vp=%p\n", ip, dp, vp);
  // // printf("BRK: %p, UL: %p \n", (void*)BREAK, (void*)UPPERLIM);
  // void *vpn = myalloc(37);
  // uintptr_t req = (uintptr_t)vpn + 48+HEADERSIZE;
  // printf("%p\n", req);
  // if((t = getchunk(req)))
  //   printf("Chunk: {s=%lu, free=%d, adress=%lx, next=%p}\n", t->chunksize, t->isfree, t->memptr, t->next);
  //
  // fprintMemory("before.txt");
  // void *vpn2 = myalloc(1024);
  //
  fprintMemory("memoryprint.txt");
  //
  char* p2 = realloc(p1, 50);
  // free(p2);
  // free(c3);
  fprintMemory("merged.txt");
  // realloc(p1, 0);
  fprintMemory("temp.txt");
  // free(str);
  // free(dp);
  // free(vp);
  // free(ip);
  // mergechunks();
  // fprintMemory("merged.txt");
  //
  // printf("BRK: %p, UL: %p \n", (void*)BREAK, (void*)UPPERLIM);
  return 0;
}
