#include "PatchesViewFl.hpp"
#include "Action.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Input.H>

#include <iostream>

using synti2base::PatchBank;

namespace synti2gui{

/** Changes the current patch, and updates other widgets. */
void cb_change_patch(Fl_Widget* w, void* p){
  ViewPatches *ww = (ViewPatches*)w;
  if (p == NULL) {
    std::cerr << "PatchBank is NULL. Change omitted." << std::endl;
    return;
  }
  PatchBank *pbh = (PatchBank*)p;
  double val = ((Fl_Valuator*)w)->value();
  if (!ww->setActivePatch(val)){
    std::cerr << pbh->getLastErrorMessage() << std::endl;
    val = ww->getActivePatch();
  }
  ((Fl_Valuator*)w)->value(val);
}

void cb_patch_name(Fl_Widget* w, void* p){
  if (p == NULL) {
    std::cerr << "PatchBankModel is NULL. Name unchanged." << std::endl;
    return;
  } else {
    //pbank->at(curr_patch).setName(((Fl_Input*)w)->value());
  }
}


/** Builds the patch editor widgets. */
void build_patch_editor(Fl_Group *gr, PatchBank *pbh = NULL)
{
  if (pbh==NULL) {
    std::cerr << "Error: No PatchBank given. Can't build GUI." << std::endl;
    return;
  }
  Fl_Scroll *scroll = new Fl_Scroll(0,25,1200,740);

  // FIXME: From a factory who can setup bound updates etc.(?)
  Fl_Counter *patch = new Fl_Counter(50,25,50,25,"Patch");

  patch->type(FL_SIMPLE_COUNTER);
  patch->align(FL_ALIGN_LEFT);
  patch->bounds(0,pbh->getNumPatches()-1);
  patch->precision(0);
  patch->callback(cb_change_patch, pbh);

  Fl_Input * widget_patch_name = new Fl_Input(150,25,90,25,"Name");
  widget_patch_name->callback(cb_patch_name,pbh);

#if 0

  int px=280, py=25, w=75, h=25, sp=2;
  int labsz = 16;
  Fl_Button *box = new Fl_Button(px+ 0*(w+sp),py,w,h,"S&end this");
  box->callback(cb_send_current); box->labelsize(labsz); 
  button_send_current = box;

  box = new Fl_Button(px + 1*(w+sp),py,w,h,"Send al&l");
  box->callback(cb_send_all); box->labelsize(labsz); 
  button_send_all = box;

  px += w/2;
  box = new Fl_Button(px + 2*(w+sp),py,w,h,"Save this");
  box->callback(cb_save_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 3*(w+sp),py,w,h,"&Save all");
  box->callback(cb_save_all); box->labelsize(labsz); 

  box = new Fl_Button(px + 4*(w+sp),py,w,h,"Ex&port C");
  box->callback(cb_export_c); box->labelsize(labsz); 

  box = new Fl_Button(px + 5*(w+sp),py,w,h,"Clear this");
  box->callback(cb_clear_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 6*(w+sp),py,w,h,"Load this");
  box->callback(cb_load_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 7*(w+sp),py,w,h,"Load all");
  box->callback(cb_load_all); box->labelsize(labsz);

  /*
    Quit will go to a menu.
  px += w/2;
  box = new Fl_Button(px + 8*(w+sp),py,w,h,"&Quit");
  box->callback(cb_exit); box->argument((long)window); box->labelsize(17); 
  */

  /* Parameters Valuator Widgets */
  px=5; py=50; w=25; h=20; sp=2;
  for (int i=0; i < pd->nPars("I3"); i++){
    /* Need to store all ptrs and have attach_to_values() */
    Fl_Value_Input *vi = new Fl_Value_Input(px+i*(w+sp),py,w,h);
    widgets_i3.push_back(vi);
    vi->bounds(pd->getMin("I3",i),pd->getMax("I3",i)); 
    vi->precision(0); vi->argument(i);
    vi->color(colortab[pd->getGroup("I3",i)]);
    vi->tooltip(pd->getDescription("I3", i).c_str());
    vi->argument(i);
    vi->callback(cb_new_i3_value);
  }

  py=80; w=85;
  int npars = pd->nPars("F");
  int ncols = 4;
  int nrows = (npars / ncols) + 1;
  int i = 0;
  for (int col=0; col < ncols; col++){
    for (int row=0; row < nrows; row++){
      if (i==npars) break;
      /* Need to store all ptrs and have attach_to_values() */
      Fl_Roller *vsf = 
        new Fl_Roller(px+col*250,py+row*(h+sp),w,h);
      widgets_f.push_back(vsf);
      /* FIXME: think? */
      vsf->bounds(pd->getMin("F",i),pd->getMax("F",i)); 
      vsf->precision(pd->getPrecision("F",i));
      vsf->color(colortab[pd->getGroup("F",i)]);
      vsf->type(FL_HOR_NICE_SLIDER);
      vsf->label(createFvalLabel(i,pd->getDescription("F",i)).c_str());
      flbl.push_back(createFvalLabel(i,pd->getDescription("F",i)));//pd->getDescription("F",i)); // for use in the other part
      vsf->align(FL_ALIGN_RIGHT);
      vsf->callback(cb_new_f_value);
      vsf->argument(i);
      i++;
    }
  }
#endif
  scroll->end();
}



using namespace synti2gui;
ViewPatches::ViewPatches(int x, int y, int w, int h, const char * name, PatchBank *pbh)  : Fl_Group(x, y, w, h, name){
  pb = pbh;
  build_patch_editor(this, pb);
}

}
