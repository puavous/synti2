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

PHDRS
{
  interp PT_INTERP ;
  text PT_LOAD FILEHDR PHDRS ;
  dynamic PT_DYNAMIC ;
}


SECTIONS
{
  /* Read-only sections, merged into text segment: */

  . = SEGMENT_START("text-segment", 0x400000) + SIZEOF_HEADERS;

  /* Order of sections has some non-zero impact on packed exe size: */

/*
  . = ALIGN (0x100);
  .interp         : { *(.interp) }

  Hmm.. interp section can be let go, if the exe is run by the
   self-extracting sh hack explicitly using the command '/lib64/ld-2*
   $a' Then we could also rid of 'chmod u+x $a' so actually would gain
   many bytes.  On 32-bit systems this would be '/lib/ld-2* $a'

  /DISCARD/ : { *(.interp) } 

  These we can get rid of afterwards with some violent action:
  /DISCARD/ : { *(.gnu.version*) *(.comment*)}

  This can be avoided with compile option (no-build-id or so..):
  /DISCARD/ : { *(.note.gnu.build-id) *(.comment) }


*/

  . = ALIGN (0x100);
  .dynamic        : { *(.dynamic) } :text :dynamic


  .hash : { *(.hash) } :text

  .dynsym         : { *(.dynsym) } :text


  .gnu.version    : { *(.gnu.version) } :text

  .gnu.version_r  : { *(.gnu.version_r) } :text



  .got            : { *(.got) *(.igot) } :text

  .plt            : { *(.plt) *(.iplt) } :text

  .got.plt        : { *(.got.plt)  *(.igot.plt) } :text

  .rela.plt       : { *(.rela.plt) } :text


  /* Combine text and rodata */
  .text           : { *(.text) *(.rodata) *(.rodata*) } :text
  .interp         : { *(.interp) } :text :interp
  .dynstr	  : { *(.dynstr) } :text

  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
/*
  . = ALIGN (CONSTANT (MAXPAGESIZE)) - ((CONSTANT (MAXPAGESIZE) - .) 
          & (CONSTANT (MAXPAGESIZE) - 1)); 

  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
*/

  .data           : { *(.data) } :text

  . = ALIGN (0x100);
  .bss : { *(.dynbss) *(.bss .bss.*) }

/*
  . = DATA_SEGMENT_END (.);
*/
}
