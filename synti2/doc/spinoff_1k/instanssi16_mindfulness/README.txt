Universal Mandala Mindfulness Colouring Book 1k
==================================================

These are the sources for my Instanssi 2016 intro compo entry. The
compo size limit was 4096 bytes, but this packs into only 1035 (using
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
fit into 1024 bytes (11 bytes smaller than the current version) using
a combination of the following tricks:

- The binary is a "moving target" until the artwork is final, and
  screen resolution set according to the party rules (this time as
  1080x720 fullscreen), so the final bytes can only be optimized with
  otherwise final production. Now I didn't go through the ELF header
  and binary code locations after fine-tuning the shader and
  synth. Some bytes could probably be saved using locations that
  repeat byte patterns used elsewhere in the otherwise final code.

- The code for stopping after 60s may not be optimal at all; it was
  just added for convenience and in a "usual, non-obscure" way.

- I didn't try redundant code repetitions that seem to affect the
  split point chosen automatically by zopfli. A better split point can
  save a few bytes, even when the uncompressed size is greater due to
  the repetitions.

- The synth is now quite closely tuned to the orchestral 440Hz tuning
  and MIDI note numbers, but obviously the "song" doesn't rely on
  those. So the base tuning could easily just repeat some nearby byte
  pattern, and the sequence and audio rendering code could likely be
  made a bit (i.e., several bytes:)) smaller.

- In the very final form, some bits or bytes could still be saved by
  modifying the assembler instruction combinations - LEA vs. PUHS&POP,
  absolute vs. RIP-relative addressing, etc.
