/* Use dlopen, dlsym on linux */
/* FIXME: Make this proper, with error checking and dlclose()!!*/
#include<dlfcn.h>

/* opengl (and other! FIXME: naming) functions bypassing normal linkage */
#include "glfuncs.h"
func_t *myglfunc[NUMFUNCTIONS];

extern const char funs[];

#ifdef ULTRASMALL
void
__attribute__ ((externally_visible)) 
_start()
{
#ifndef NO_I64
  /* x64-64 requires stack alignment */
  /* TODO: check if rbp always arrives pre-zeroed...
      "xor %rbp,%rbp\n"                      \
   */
  asm (                                         \
       "and $0xfffffffffffffff0,%rsp"           \
                                                );
#endif
#else
int main(int argc, char *argv[]){
#endif

  void *handles[32];
  const char *strs;
  strs = funs;
  int i = 0;
  int nlib = 0;

  do{
    /* Open .so */
    handles[nlib] = dlopen(strs, RTLD_LAZY);
    while (*(++strs) != '\0'){};
    strs++;

    /* Load functions from this .so */
    do{
      myglfunc[i] = (func_t*) dlsym(handles[nlib], (const char *)strs );
#if NEED_DEBUG
        printf("Func %d at: %lx  (\"%s\")\n", i, myglfunc[i], strs);
#endif
      while (*(++strs) != '\0'){};
      i++;
    } while (*(++strs) != '\0');
    nlib++;
  } while (*(++strs) != '\0');

  
  main2();
  
#ifdef ULTRASMALL
  /* Inline assembler for exiting without need of stdlib */
  /* (Never mind the exit code.. not used for anything..)*/

  asm (                                       \
       "movl $1,%eax\n"                       \
       "int $128\n"                           \
      );
}
#else
  return 0;
}
#endif
