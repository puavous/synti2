/** @file midi2synti2.cxx
 *
 * A converter program that filters and transforms standard midi
 * messages (both on-line, and from SMF0 and SMF1 sequence files) to
 * messages that synti2 can receive (also both on-line, and in
 * stand-alone sequence playback mode). A graphical user interface is
 * provided for the on-line mode, and a non-graphical command-line
 * mode is available for the SMF conversion stage. (FIXME: Not all is
 * implemented as of yet)
 *
 * This is made with some more rigor than earlier proof-of-concept
 * hacks, but there is a hard calendar time limitation before
 * Instanssi 2012, and thus many kludges and awkwardness are likely to
 * persist.
 *
 */

/* Includes needed by fltk. */
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>

/* Thanks to http://seriss.com/people/erco/fltk for nice and helpful
 * Fltk examples.
 */

class MainWin : public Fl_Window {
private:

public:  

  static void cb_exit(Fl_Widget *w, void *data){
    ((Fl_Window*)data)->hide();
  }

  MainWin(int X, int Y, 
          int W=100, int H=140,
          const char *L=0) : Fl_Window(X,Y,W,H,L)
  {
    Fl_Button *b = new Fl_Button(10,10,50,20,"&Quit");
    b->callback(cb_exit); b->argument((long)this);
    end();
  }
};

int main(int argc, char **argv){
  MainWin *mw = new MainWin(10,10);
  mw->show(argc, argv);
  int retval = Fl::run();

  return retval;
}
