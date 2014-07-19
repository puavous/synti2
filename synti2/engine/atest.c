/** 
 * A program to find out things that depend on the implementation of
 * the C compiler and underlying platform implementation. This will
 * write out a header file that describes the findings. Basically we
 * could do a lot of tests here for non-standard tricks like signed
 * integer overflow...
 *
 * Note to self: C99 and later standards have some of this stuff
 * built-in. So, I should start using that in future projects.
 *
 */

/* Wavetable size is defined in number of bits in the indices. This
 * program deduces other values from actual length of "unsigned int".
 * TODO: Why not make this a command line argument?
 */
#define BITS_FOR_WAVETABLE 16

#include<stdio.h>
int main(int argc, char **argv){
  const char* header = ""
    "/**\n"
    " * Constants depending on the computer architecture\n"
    " * and C compiler. Automatically generated by 'atest.c'\n"
    " */\n"
    "#ifndef SYNTI2_ARCHDEP_DEFINED\n"
    "#define SYNTI2_ARCHDEP_DEFINED\n\n";

  const char* footer = ""
    "\n#endif\n";

  unsigned int max_counter;
  unsigned int counter_to_table_shift;
  unsigned int wavetable_size;
  unsigned int wavetable_bitmask;
  unsigned int tmp_counter;
  int nbits_in_counter_val;
  unsigned char test_byte;
 
  /* Counter overflow point (same as UINT_MAX from limits.h?
   * well.. compute it ourselves anyway)
   */
  max_counter = 0;
  max_counter--;

  /* Check how many bits in counter: */
  tmp_counter = max_counter;
  nbits_in_counter_val = 0;
  while(tmp_counter > 0){
    tmp_counter >>= 1;
    nbits_in_counter_val++;
  }

  /*Wavetable size, bitmask and bit shift for division*/
  counter_to_table_shift = nbits_in_counter_val - BITS_FOR_WAVETABLE;
  wavetable_size = 1 << BITS_FOR_WAVETABLE;
  wavetable_bitmask = wavetable_size - 1;

  /* Check that "unsigned char" is 8 bits. Not much can be done if it
   * isn't, though..
   */
  test_byte = 0xff;
  test_byte++;
  if (test_byte != 0){
    printf("#error Can't build on this platform. Unsigned char is not 8 bits.\n");
    return 1;
  }

  printf(header);
  printf("#define MAX_COUNTER %u\n",max_counter);
  printf("#define WAVETABLE_SIZE 0x%x\n",
    wavetable_size);
  printf("#define WAVETABLE_BITMASK 0x%x\n",
    wavetable_bitmask);
  printf("#define COUNTER_TO_TABLE_SHIFT %d\n",
    counter_to_table_shift);
  printf("\n");
  printf("typedef unsigned char byte_t;\n");
  printf(footer);

  return 0;
}
