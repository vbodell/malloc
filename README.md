Victor Bodell (sbodell)

# Instructions
1) Compile with 'make' or 'make malloc'
   This builds from source files and creates an archive (libmalloc.a), 
   a library to link to.
2) If target intel-all ('make intel-all') is specified a 32-bit and a 64-bit
   shared library will be built


# Other Information
Concerning Debugging output: Whenever calloc or realloc is called they will use
malloc as their memory allocator, size allocated will not be available for 
malloc or calloc after the return from malloc, which means a call to one of
them will first produce debugging output to malloc, and afterwards calloc or
realloc.

Concerning returning memory to kernel when large chunks become free, the
function is here_you_go_kernel(), it has not been as extensively tested as the
rest of the code and is therefor not called (the calling code is in comments,
lines 175-179 in malloc.h)

There is also a function to print the chunks in the malloc.h file,
lines 263-2286 in malloc.h, which is not called either, 
used only during testing.
