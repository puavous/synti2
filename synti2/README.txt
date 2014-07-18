Synti2 Software Synthesizer with some tool programs and examples.

Status:

  - The engine code and some examples (including the Assembly 2013
    entry) will be shown at the sponsor stand of the Department of
    Mathematical Information Technology / University of Jyväskylä at
    Assembly Summer 2014. I also have a local, hidden branch for a new
    compo entry for the 4k compo at Asm'14; that branch will be pushed
    to the public repository after the 4k compo has been shown on the
    big screen.

  - The synth engine has only one (or maybe two) changes to be
    done. I'm very close to defining synti2 complete as a synthesis
    engine / interface.

  - There is some evil timing-issue (tempo instability) with SDL
    stand-alone. Needs to be analysed and fixed, if at all possible!

  - Tool programs are still, and may always remain, incomplete, buggy
    hacks. They suffice for my current purposes. Feel free to
    implement them better ...

  - Also the build "system" may remain flaky and convoluted for this
    project..



Dependencies:

  - GNU build tools

  - Jack audio connection kit for real-time sound output

  - Fltk for sound editor GUI

  - SDL for graphics and stand-alone audio; needed on target machine,
    too

  - OpenGL for graphics effects; needed on target machine, too

  - ShaderMinifier (and mono to run it) for tiny shader code

  - sstrip for tiny exes

  - zopfli for tiny exes

  - libsndfile for exporting stand-alone audio as wav.

  - any standard MIDI sequencer for composing music

  - docutils & tex for some of the documentation / presentation slides

(There might be more.. should check)
