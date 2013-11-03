#include "MainWindow.hpp"
#include "PatchesViewFl.hpp"
#include "MidiMapperViewFl.hpp"
#include "FeaturesViewFl.hpp"
#include "ExeBuilderViewFl.hpp"
#include "PatchBank.hpp"
using synti2base::PatchBank;
using synti2base::MidiMap;
using synti2gui::ViewPatches;
using synti2gui::ViewMidiMapper;
using synti2gui::ViewFeatures;
using synti2gui::ViewExeBuilder;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>

/** Builds the main window with widgets reflecting a patch description. */
void
build_main_window(Fl_Window * window, PatchBank *pbh)
{
  Fl_Group *gr = NULL;

  window->resizable(window);

  Fl_Tabs *tabs = new Fl_Tabs(0,0,1200,740);

  gr = new ViewPatches(0,22,1200,720,"Patches",pbh);
  gr->end();

  gr = new ViewMidiMapper(0,22,1200,720,"MIDI mapper",pbh->leakMidiMapPtr());
  gr->end();

  gr = new ViewFeatures(0,22,1200,720,"Features",pbh);
  gr->end();

  gr = new ViewExeBuilder(0,22,1200,720,"Exe Builder",pbh);
  gr->end();

  tabs->end();
  window->end();
}

using namespace synti2gui;
MainWindow::MainWindow(int w, int h,
                       PatchBank *pbh) : Fl_Window(w,h){
  build_main_window(this, pbh);
}
