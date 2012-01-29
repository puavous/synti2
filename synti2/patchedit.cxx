#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Dial.H>

/**
 * A small program for creating synti2 patches.
 *
 * UI idea:
 *
 *  File: experiments.s2patch
 *  Patch: __  <-- can be 0-f
 *  [Copy from...]
 *
 *  Env1K1T K1L   K2T
 *  [0.00] [1.27] [0.20]  ... [101.19] <-- textboxes with float vals.
 *  Env2K1T K1L
 *  [0.00] [1.27] [0.20]  ... [101.19] <-- textboxes with float vals.
 *  ...
 *  Dlylen Dlyfd
 *  [0.00] [1.27] [0.20]  ... [101.19] <-- textboxes with float vals.
 *
 *  [Save]
 *
 *  Clicking on a value box opens a dialog that allows MIDI input to
 *  be mapped into the box:
 *
 *   (o)  Coarse CC=___  [learn] min=[0] max=[127]
 *   ( ) +Fine   CC=___  [learn] min=[0.00] max=[1.27]
 *   [OK] [Cancel]
 */

int main(int argc, char **argv) {
  Fl_Window *window = new Fl_Window(600,180);
  Fl_Box *box = new Fl_Box(20,40,260,100,"Hello, World!");
  box->box(FL_UP_BOX);
  box->labelsize(36);
  box->labelfont(FL_BOLD+FL_ITALIC);
  box->labeltype(FL_SHADOW_LABEL);
  Fl_Dial *dial = new Fl_Dial(320,40,100,100,"Kissa123");
  dial->align(FL_ALIGN_CENTER);
  Fl_Dial *dial2 = new Fl_Dial(420,40,100,100,"@->| ja joo");
  dial2->type(FL_LINE_DIAL);
  window->end();
  window->show(argc, argv);
  return Fl::run();
}
