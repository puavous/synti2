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
  void ViewPatches::build_patch_editor(Fl_Group *gr, PatchBank *pbh)
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

  int px=280, py=25, w=75, h=25, sp=2;
  int labsz = 16;


  Fl_Button *box = new Fl_Button(px+ 0*(w+sp),py,w,h,"S&end this");
  //box->callback(cb_send_current); box->labelsize(labsz); 
  //button_send_current = box;

  box = new Fl_Button(px + 1*(w+sp),py,w,h,"Send al&l");
  //box->callback(cb_send_all); box->labelsize(labsz); 
  //button_send_all = box;

  px += w/2;
  box = new Fl_Button(px + 2*(w+sp),py,w,h,"Save this");
  //box->callback(cb_save_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 3*(w+sp),py,w,h,"&Save all");
  //box->callback(cb_save_all); box->labelsize(labsz); 

  box = new Fl_Button(px + 5*(w+sp),py,w,h,"Clear this");
  //box->callback(cb_clear_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 6*(w+sp),py,w,h,"Load this");
  //box->callback(cb_load_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 7*(w+sp),py,w,h,"Load all");
  //box->callback(cb_load_all); box->labelsize(labsz);

  int i=0;
  std::vector<std::string>::const_iterator i4it,fit;
  Fl_Value_Input *vi;

  px=5; py=50; w=25; h=20; sp=2;
  //FIXME: Make a derived I4_Input / I4_Filter_Input / I4_OnOff_Input.
  //FIXME: Include "type of GUI" and allowed values as attributes of
  //both the derived widget and I4Par description.
  /* Parameters Valuator Widgets */
  for (i4it=pbh->getI4Begin(activePatch); 
       i4it!=pbh->getI4End(activePatch);
       ++i4it){
    // FIXME: Actual data from desription
    vi = new Fl_Value_Input(px+(i++)*(w+sp),py,w,h);
    vi->tooltip(pbh->getI4Par(activePatch,*i4it).getHumanReadable().c_str());
    vi->precision(0);
    //vi->color(colortab[pd->getGroup("I3",i)]);
    //vi->argument(i); // what is da argument then?
    //vi->callback(cb_new_i3_value);
  }

  py=80; w=85; h=15;
  int npars = pbh->getFEnd(activePatch) - pbh->getFBegin(activePatch);
  int ncols = 4;
  int nrows = (npars / ncols) + 1;
  i = 0;
  int col=0,row=0;

  for (fit=pbh->getFBegin(activePatch); 
       fit!=pbh->getFEnd(activePatch);
       ++fit){
    // FIXME: Actual data from desription
      Fl_Roller *vsf = 
        new Fl_Roller(px+col*250,py+row*(h+sp),w,h);
      vsf->tooltip(pbh->getFPar(activePatch,*fit).getHumanReadable().c_str());
      vsf->type(FL_HOR_NICE_SLIDER);
      vsf->label(pbh->getFPar(activePatch,*fit).getHumanReadable().c_str());
      vsf->align(FL_ALIGN_RIGHT);

      row++;
      if (row>30){row=0;col++;}
  }
#if 0

  for (int col=0; col < ncols; col++){
    for (int row=0; row < nrows; row++){
      if (i==npars) break;
      /* Need to store all ptrs and have attach_to_values() */
      Fl_Roller *vsf = 
        new Fl_Roller(px+col*250,py+row*(h+sp),w,h);
      //widgets_f.push_back(vsf);
      /* FIXME: think? */
      vsf->bounds(pd->getMin("F",i),pd->getMax("F",i)); 
      vsf->precision(pd->getPrecision("F",i));
      vsf->color(colortab[pd->getGroup("F",i)]);
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
  ViewPatches::ViewPatches(int x, int y, int w, int h, const char * name, PatchBank *pbh) 
    : Fl_Group(x, y, w, h, name), activePatch(0)
{
  pb = pbh;
  build_patch_editor(this, pb);
}

}
