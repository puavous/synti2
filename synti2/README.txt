Synti2 Software Synthesizer with some tool programs and examples.

Status:

  - The synth engine design of synti2 is now final, including the
    interface (patch and song format). No new features or changes in
    how it 'sounds' will be made. (Note that the engine can already do
    more than the tool programs allow it to... and 4k productions
    require many "cool" features to be turned off to conserve size)

    Only bug fixes and improvements in code size and/or performance
    should be made.

  - There is some evil timing-issue (tempo instability) at least with
    SDL stand-alone. Needs to be analysed and fixed, if at all
    possible!

  - Tool programs are still, and may always remain, incomplete, buggy
    hacks. They suffice for my current purposes. Feel free to
    implement them better ...

  - Also the build "system" may remain flaky and convoluted for the
    lifetime of this synth engine.. I'll try to improve things in
    future projects.

Dependencies:

  - GNU build tools

  - Jack audio connection kit for real-time sound output

  - Fltk for sound editor GUI

  - SDL for graphics and stand-alone audio; run-time needed on target
    machine, too

  - OpenGL for graphics effects; run-time needed on target machine,
    too

  - ShaderMinifier (and mono to run it) for tiny shader code

  - sstrip for tiny exes

  - zopfli for tiny exes

  - libsndfile for exporting stand-alone audio as wav.

  - any sequencer with standard MIDI file export for composing music

  - docutils & tex for some of the documentation / presentation slides

(There might be more.. should check, and add in the above list)
