#include "MainWindow.hpp"
#include "PatchesViewFl.hpp"
#include "MidiMapperViewFl.hpp"
#include "PatchBankHandler.hpp"
using synti2gui::PatchBankHandler;
using synti2gui::ViewPatches;
using synti2gui::ViewMidiMapper;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>

/** Builds the main window with widgets reflecting a patch description. */
void
build_main_window(Fl_Window * window, PatchBankHandler *pbh)
{
  Fl_Group *gr = NULL;

  window->resizable(window);

  Fl_Tabs *tabs = new Fl_Tabs(0,0,1200,740);

  gr = new ViewPatches(0,22,1200,720,"Patches",pbh);
  gr->end();

  gr = new ViewMidiMapper(0,22,1200,720,"MIDI mapper",pbh);
  gr->end();

  gr = new Fl_Group(0,22,1200,720, "Features");
  gr->end();

  gr = new Fl_Group(0,22,1200,720, "Exe Builder");
  gr->end();

  tabs->end();
  window->end();
}

using namespace synti2gui;
MainWindow::MainWindow(int w, int h, 
                       PatchBankHandler *pbh) : Fl_Window(w,h){
  build_main_window(this, pbh);
}
