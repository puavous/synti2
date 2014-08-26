/* This is a stripped down version of what ld was using by
   default. The gzipped exe will be about 120 bytes shorter using this,
   but I don't know what happens to executable portability... 

  When dlopen and dlsym are used for function linkage, we can lose
  almost all of the sections... Some of them seem to make the
  compressed exe smaller, though FIXME: Examine, and make it
  so... there wille be more space for graphics that way FIXME: Also,
  make all data in ULTRASMALL mode static.. and clean up the codes
  even if they are eventually in one bunch (only in ultrasmall mode..)
  FIXME: dlclose() in all but ultrasmall mode.
*/

SEARCH_DIR("/lib64"); SEARCH_DIR("/usr/lib64"); SEARCH_DIR("/usr/local/lib64"); 
SEARCH_DIR("/lib"); SEARCH_DIR("/usr/lib"); SEARCH_DIR("/usr/local/lib"); 

ENTRY(_start)

SECTIONS
{
  /* Read-only sections, merged into text segment: */

  . = SEGMENT_START("text-segment", 0x400000) + SIZEOF_HEADERS;

  .dynsym         : { *(.dynsym) }

  /* Will be shorter compressed with these (wonder why.. aha.. has
     to do with relocations.. have to learn some more..): */
  .rela.plt       :
    {
      *(.rela.plt)  PROVIDE_HIDDEN (__rela_iplt_start = .);
      *(.rela.iplt) PROVIDE_HIDDEN (__rela_iplt_end = .);
    }

/*.plt            : { *(.plt) *(.iplt) }*/

/*
  .text           : { *(.text) }

  .rodata         : { *(.rodata) *(.rodata*) }
*/
  .text           : { *(.text) *(.rodata) *(.rodata*) }

  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */

  . = ALIGN (CONSTANT (MAXPAGESIZE)) - ((CONSTANT (MAXPAGESIZE) - .) 
          & (CONSTANT (MAXPAGESIZE) - 1)); 
  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));

  .dynamic        : { *(.dynamic) }
  .got            : { *(.got) *(.igot) }
  .got.plt        : { *(.got.plt)  *(.igot.plt) }

  .data           : { *(.data) }

/* Is it safe to not have bss (?): */
/*  .bss : { *(.dynbss) *(.bss .bss.* .gnu.linkonce.b.*) *(COMMON)
   . = ALIGN(. != 0 ? 64 / 8 : 1); }  */
/*
  . = DATA_SEGMENT_END (.);
*/
}
