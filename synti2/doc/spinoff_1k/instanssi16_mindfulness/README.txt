Universal Mandala Mindfulness Colouring Book 1k
==================================================

These are the sources for my Instanssi 2016 intro compo entry. The
compo size limit was 4096 bytes, but this packs into only 1025 (using
zopfli with --i3000).

System requirements:

- Build: Linux x86_64 installation, GNU development tools, NASM
  assembler, zopfli packer.

- Run: Linux x86_64 installation with SDL2 and OpenGL
  libraries. (usually found in Desktop Linux distros; proprietary
  graphics drivers may need separate installation).

- Tested on current Fedora and Debian. Essentially the same machine
  code has been found to work on Ubuntu, too, with earlier compo
  entries.

Of course, I was hoping to meet 1024 bytes, as in actual 1k compos,
but decided to use the time to prepare a better "making of"
-presentation at the party seminar session. I'm pretty sure this would
fit into 1024 bytes (1 byte smaller than the current version) using
a combination of the following tricks:

- The binary is a "moving target" until the artwork is final, and
  screen resolution set according to the party rules (this time as
  1280x720 fullscreen), so the final bytes can only be optimized with
  otherwise final production. Now I didn't go through the ELF header
  and binary code locations after fine-tuning the shader and
  synth. Some bytes could probably be saved using locations that
  repeat byte patterns used elsewhere in the otherwise final code.

- I didn't try redundant code repetitions that seem to affect the
  split point chosen automatically by zopfli. A better split point can
  save a few bytes, even when the uncompressed size is greater due to
  the repetitions.

- In the very final form, some bits or bytes could still be saved by
  modifying the assembler instruction combinations - LEA vs. PUHS&POP,
  absolute vs. RIP-relative addressing, etc.

The following things I just had to do after the compo (getting the
size down to 1025 from the 1030+ that it was at the compo):

- The code for stopping after 60s was not optimal at all; it was just
  added for convenience and in a "usual, non-obscure" way. => Now it
  is just a bit smaller. Running time is just a bit over 60s, i.e.,
  approximately 0x2c0000 samples at 48000kHz.

- The synth was quite closely tuned to the orchestral 440Hz tuning and
  MIDI note numbers, but obviously the "song" doesn't rely on
  those. So the base tuning could easily just repeat some nearby byte
  pattern, and the sequence and audio rendering code could likely be
  made a bit (i.e., several bytes:)) smaller. => Now it's a bit more
  straightforward. Perhaps could gain a bit or two more from the
  synth code...

- Oops, and there was some syntactically unnecessary parentheses in
  the shader. Removed those. One byte shorter.

- Changed the shader algorithm just a bit. I doubt that anyone will
  notice the difference in graphics :). One byte shorter.

