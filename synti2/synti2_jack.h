#ifndef SYNTI2_JACK_INTERFACE_H
#define SYNTI2_JACK_INTERFACE_H

#include "jack/control.h"
#include "jack/jack.h"
#include "jack/midiport.h"
#include <errno.h>

void
synti2_read_jack_midi(synti2_synth *s,
                      jack_port_t *inmidi_port,
                      jack_nframes_t nframes);

#endif
