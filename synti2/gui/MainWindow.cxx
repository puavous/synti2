#include "MainWindow.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>

/** Builds the main window with widgets reflecting a patch description. */
void
build_main_window(Fl_Window * window) //, synti2::PatchDescr *pd){
{
  /* Overall Operation Buttons */
  window->resizable(window);

  Fl_Tabs *tabs = new Fl_Tabs(0,0,1200,740);

  Fl_Group *gr = new Fl_Group(0,22,1200,720, "Patches");
  //Fl_Group *patchedit = build_patch_editor(pd);
  gr->end();

  gr = new Fl_Group(0,22,1200,720, "MIDI mapper");
  //build_message_mapper(1,23,1198,718);
  gr->end();

  gr = new Fl_Group(0,22,1200,720, "Features");
  gr->end();

  gr = new Fl_Group(0,22,1200,720, "Exe Builder");
  gr->end();

  tabs->end();
  window->end();
}

using namespace synti2gui;
MainWindow::MainWindow(int w, int h)  : Fl_Window(w,h){
  build_main_window(this);
}
