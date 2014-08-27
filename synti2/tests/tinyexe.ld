/* This is a stripped down version of what ld was using by
   default. The gzipped exe will be about 120 bytes shorter using this,
   but I don't know what happens to executable portability... 

  FIXME: clean up the codes even if they are eventually in one bunch
  (only in ultrasmall mode..)  FIXME: dlclose() in all but ultrasmall
  mode?

  NOTE: Could I use "objcopy -O binary" for some nasty/nice thing..?

*/

SEARCH_DIR("/lib64"); SEARCH_DIR("/usr/lib64"); SEARCH_DIR("/usr/local/lib64"); 
SEARCH_DIR("/lib"); SEARCH_DIR("/usr/lib"); SEARCH_DIR("/usr/local/lib"); 

ENTRY(_start)

SECTIONS
{
  /* Read-only sections, merged into text segment: */


  . = SEGMENT_START("text-segment", 0x400000) + SIZEOF_HEADERS;

  /* Order of sections has some non-zero impact on packed exe size: */
  . = ALIGN (0x100);
  .interp         : { *(.interp) }
  . = ALIGN (0x100);
  .dynstr	  : { *(.dynstr) }  
  . = ALIGN (0x100);
  .dynamic        : { *(.dynamic) }
  . = ALIGN (0x100);
  .dynsym         : { *(.dynsym) }
  . = ALIGN (0x100);
  .gnu.version    : { *(.gnu.version) }
  . = ALIGN (0x100);
  .gnu.version_r  : { *(.gnu.version_r) }
  . = ALIGN (0x100);
  .hash : { *(.hash) }

  . = ALIGN (0x100);
  .got            : { *(.got) *(.igot) }
  . = ALIGN (0x100);
  .got.plt        : { *(.got.plt)  *(.igot.plt) }
  . = ALIGN (0x100);
  .plt            : { *(.plt) *(.iplt) }
  . = ALIGN (0x100);
  .rela.plt       : { *(.rela.plt) }

  . = ALIGN (0x100);
  /* Combine text and rodata */
  .text           : { *(.text) *(.rodata) *(.rodata*) }

  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */

  . = ALIGN (CONSTANT (MAXPAGESIZE)) - ((CONSTANT (MAXPAGESIZE) - .) 
          & (CONSTANT (MAXPAGESIZE) - 1)); 
/*
  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
*/

  . = ALIGN (0x100);
  .data           : { *(.data) }

  . = ALIGN (0x100);
  .bss : { *(.dynbss) *(.bss .bss.*) }


  /DISCARD/ : { *(.note.gnu.build-id) }

/*
   . = ALIGN(. != 0 ? 64 / 8 : 1); }  */
/*
  . = DATA_SEGMENT_END (.);
*/
}
