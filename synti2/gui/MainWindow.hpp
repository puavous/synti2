#ifndef SYNTI2_GUI_MAINWINDOW
#define SYNTI2_GUI_MAINWINDOW
#include <FL/Fl.H>
#include <FL/Fl_Window.H>

namespace synti2gui{
  class MainWindow : public Fl_Window{
  public:
    MainWindow(int w, int h) : Fl_Window(w,h){};
  };
}

#endif
