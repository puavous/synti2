#include "PatchBank.hpp"
#include "PatchesViewFl.hpp"
#include "Action.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_File_Chooser.H>

#include <iostream>
#include <fstream>

using synti2base::PatchBank;

namespace synti2gui{

/** Update of an "I4" value; FIXME: We need our own widget set! */
void ViewPatches::value_callback(Fl_Widget* w, void* p){
    S2Valuator *vin = (S2Valuator*)w;
    double val = vin->value();
    ViewPatches *vp = (ViewPatches*)p;
    vin->pb()->setParamValue(vp->getActivePatch(), vin->getKey(), val);
    vin->updateLooks();
}

void cb_patch_name(Fl_Widget* w, void* p){
  if (p == NULL) {
    std::cerr << "PatchBankModel is NULL. Name unchanged." << std::endl;
    return;
  } else {
    //pbank->at(curr_patch).setName(((Fl_Input*)w)->value());
  }
}

/** Sends current patch to MIDI output. */
void ViewPatches::butt_send_cb(Fl_Widget* w, void* p){
    ((ViewPatches*)p)->sendCurrentPatch();
}

/** Sends all patches. */
void ViewPatches::butt_send_all_cb(Fl_Widget* w, void* p){
    ((ViewPatches*)p)->pb->sendAllPatches();
}

/* helper. checks that a file exists.. */
bool file_exists(const char* fname){
  std::ifstream checkf(fname);
  return checkf.is_open();
}

/* Helper: Shows a file chooser dialog; returns filename or empty string
 * if there was a problem or cancel.
 */
std::string fileChooser(string const & path,
                        string const & extension,
                        bool save = true){

    string filter = "*"+extension;
    Fl_File_Chooser chooser(path.c_str(),filter.c_str(),Fl_File_Chooser::CREATE,
                            save?"Save -- select destination file.":"Load");
    chooser.show();
    while(chooser.shown()) Fl::wait();
    if ( chooser.value() == NULL ) return "";

    string res(chooser.value());
    if (res.find_first_of(".")==string::npos) res += extension;

    if (save){
        if (file_exists(res.c_str())){
            if (2 != fl_choice("File %s exists. \nDo you want to overwrite it?",
                              "Cancel", "No", "Yes", res.c_str())) return "";
        }
    } else {
        if (!file_exists(res.c_str())){
            return "";
        }
    }
    return res;
}

/** Saves a whole bank. */
void ViewPatches::butt_save_bank_cb(Fl_Widget* w, void* p){
    string fname = fileChooser(".",".s2bank",true);
    if (fname == "") return;

    std::ofstream ofs(fname.c_str(), std::ios::trunc);
    ((ViewPatches*)p)->pb->toStream(ofs);
}
void ViewPatches::butt_load_bank_cb(Fl_Widget* w, void* p){
    string fname = fileChooser(".",".s2bank",false);
    if (fname == "") return;

    std::ifstream ifs(fname.c_str());
    ((ViewPatches*)p)->pb->reloadFromStream(ifs);
}
void ViewPatches::butt_save_current_cb(Fl_Widget* w, void* p){
    string fname = fileChooser(".",".s2patch",true);
    if (fname == "") return;
    std::ofstream ofs(fname.c_str(), std::ios::trunc);
    ((ViewPatches*)p)->pb->writeOnePatch(((ViewPatches*)p)->getActivePatch(),ofs);
}
void ViewPatches::butt_load_current_cb(Fl_Widget* w, void* p){
    string fname = fileChooser(".",".s2patch",false);
    if (fname == "") return;
    std::ifstream ifs(fname.c_str());
    ((ViewPatches*)p)->pb->readOnePatch(((ViewPatches*)p)->getActivePatch(),ifs);
}
void ViewPatches::butt_clear_cb(Fl_Widget* w, void* p){
    ((ViewPatches*)p)->pb->clearOnePatch(((ViewPatches*)p)->getActivePatch());
}
void ViewPatches::butt_panic_cb(Fl_Widget* w, void* p){
    ((ViewPatches*)p)->pb->pleaseSendPanic();
}



/** Changes the current patch, and updates other widgets..
 *Or should that be a callback from PatchEdit? */
void ViewPatches::val_ipat_cb(Fl_Widget* w, void* p){
    ViewPatches *view = (ViewPatches*)p;
    view->setActivePatch(((Fl_Valuator*)w)->value());
}

/** Builds the patch editor widgets. */
  void ViewPatches::build_patch_editor(Fl_Group *gr)
{
  if (pb==NULL) {
    std::cerr << "Error: No PatchBank given. Can't build GUI." << std::endl;
    return;
  }
  Fl_Scroll *scroll = new Fl_Scroll(0,25,1200,740);

  // FIXME: From a factory who can setup bound updates etc.(?)
  Fl_Counter *patch = new Fl_Counter(50,25,50,25,"Patch");

  /* FIXME: This, fix, properly: */
  patch->type(FL_SIMPLE_COUNTER);
  patch->align(FL_ALIGN_LEFT);
  patch->bounds(0,pb->getNumPatches()-1);
  patch->precision(0);
  patch->callback(val_ipat_cb, this);

  Fl_Input * widget_patch_name = new Fl_Input(150,25,90,25,"Name");
  widget_patch_name->callback(cb_patch_name,pb);

  int px=280, py=25, w=70, h=25, sp=2;
  int labsz = 12;

  Fl_Button *box;
  box = new Fl_Button(px+ 0*(w+sp),py,w,h,"S&end this");
  box->callback(butt_send_cb,this); box->labelsize(labsz);

  box = new Fl_Button(px + 1*(w+sp),py,w,h,"Send al&l");
  box->callback(butt_send_all_cb,this); box->labelsize(labsz);

  px += w/2;
  box = new Fl_Button(px + 2*(w+sp),py,w,h,"Save this");
  box->callback(butt_save_current_cb,this); box->labelsize(labsz);

  box = new Fl_Button(px + 3*(w+sp),py,w,h,"&Save all");
  box->callback(butt_save_bank_cb,this); box->labelsize(labsz);

  px += w/2;
  box = new Fl_Button(px + 4*(w+sp),py,w,h,"Clear this");
  box->callback(butt_clear_cb,this); box->labelsize(labsz);

  box = new Fl_Button(px + 5*(w+sp),py,w,h,"Load this");
  box->callback(butt_load_current_cb,this); box->labelsize(labsz);

  box = new Fl_Button(px + 6*(w+sp),py,w,h,"Load all");
  box->callback(butt_load_bank_cb,this); box->labelsize(labsz);

  px += w/2;
  box = new Fl_Button(px + 7*(w+sp),py,w,h,"Panic!");
  box->callback(butt_panic_cb,this); box->labelsize(labsz);

  std::vector<std::string>::const_iterator i4it,fit;
  S2Valuator *vi;

  int ncols = 30;
  int col=0,row=0;

  px=5; py=50; w=25; h=20; sp=2;
  //FIXME: Make a derived I4_Input / I4_Filter_Input / I4_OnOff_Inpu?
  //FIXME: Include "type of GUI" and allowed values as attributes of
  //both the derived widget and I4Par description.(?)
  //Maybe Take the parameter description with GuiStyle field in ctor of S2VI?
  /* Parameters Valuator Widgets */
  for (i4it=pb->getI4Begin(activePatch);
       i4it!=pb->getI4End(activePatch); ++i4it)
  {
    vi = new S2ValueInput(px+col*(w+sp),py+row*(h+sp),w,h,pb,*i4it);
    vi->callback(value_callback, this);
    col++;if (col>=ncols) {col=0;row++;}

    RuleSet rs = pb->getI4Par(activePatch,*i4it).getRuleSet();
    rs.ownThisAction(new WidgetEnablerRuleAction(vi));
    pb->registerRuleAction(rs);
  }

  py=100; w=85; h=15;
  int npars = pb->getFEnd(activePatch) - pb->getFBegin(activePatch);
  int nrows = (npars / ncols) + 1;
  ncols = 4;col=0,row=0;

  for (fit=pb->getFBegin(activePatch);
       fit!=pb->getFEnd(activePatch); ++fit)
  {
      vi = new S2FValueInput(px+col*250,py+row*(h+sp),w,h,pb,*fit);
      vi->callback(value_callback, this);
      vi->updateLooks();
      row++;if (row>30){row=0;col++;}

      RuleSet rs = pb->getFPar(activePatch,*fit).getRuleSet();
      rs.ownThisAction(new WidgetEnablerRuleAction(vi));
      pb->registerRuleAction(rs);
  }

#if 0

  for (int col=0; col < ncols; col++){
    for (int row=0; row < nrows; row++){
      if (i==npars) break;
      /* FIXME: think? */
      vsf->bounds(pd->getMin("F",i),pd->getMax("F",i));
      vsf->precision(pd->getPrecision("F",i));
      vsf->color(colortab[pd->getGroup("F",i)]);
      vsf->label(createFvalLabel(i,pd->getDescription("F",i)).c_str());
      flbl.push_back(createFvalLabel(i,pd->getDescription("F",i)));//pd->getDescription("F",i)); // for use in the other part
      vsf->callback(cb_new_f_value);
      vsf->argument(i);
      i++;
    }
  }
#endif

  pb->forceAllRuleActions();

  scroll->end();
}



using namespace synti2gui;
  ViewPatches::ViewPatches(int x, int y, int w, int h, const char * name, PatchBank *pbh)
    : Fl_Group(x, y, w, h, name), activePatch(0)
{
  pb = pbh;
  build_patch_editor(this);
}

}
