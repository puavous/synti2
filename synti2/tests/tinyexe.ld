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

  /* Order of sections has some non-zero impact on packed exe size: */
  .interp         : { *(.interp) }
  .dynstr	  : { *(.dynstr) }
  .dynamic        : { *(.dynamic) }
  .dynsym         : { *(.dynsym) }
  .gnu.version    : { *(.gnu.version) }
  .gnu.version_r  : { *(.gnu.version_r) }

  .hash : { *(.hash) }

  .rela.plt       : { *(.rela.plt) }
  .got            : { *(.got) *(.igot) }
  .got.plt        : { *(.got.plt)  *(.igot.plt) }
  .plt            : { *(.plt) *(.iplt) }
  /* Combine text and rodata */
  .text           : { *(.text) *(.rodata) *(.rodata*) }

  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */

  . = ALIGN (CONSTANT (MAXPAGESIZE)) - ((CONSTANT (MAXPAGESIZE) - .) 
          & (CONSTANT (MAXPAGESIZE) - 1)); 
/*
  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
*/

  .data           : { *(.data) }

  .bss : { *(.dynbss) *(.bss .bss.* .gnu.linkonce.b.*) }


  /DISCARD/ : { *(.note.gnu.build-id) }

/*
   . = ALIGN(. != 0 ? 64 / 8 : 1); }  */
/*
  . = DATA_SEGMENT_END (.);
*/
}
