#ifndef SYNTI2_GUI_MAINWINDOW
#define SYNTI2_GUI_MAINWINDOW
#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>

#include "PatchBankHandler.hpp"
using synti2gui::PatchBankHandler;

namespace synti2gui{
  class MainWindow : public Fl_Window{
  public:
    MainWindow(int w, int h, PatchBankHandler *pbh);
  };
}

#endif
