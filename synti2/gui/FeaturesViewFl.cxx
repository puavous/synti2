#include "FeaturesViewFl.hpp"
#include "PatchBank.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Input.H>

#include <iostream>
#include <vector>

using synti2base::PatchBank;
namespace synti2gui {

std::vector<std::string> ViewFeatures::keys;

void
ViewFeatures::feat_callback (Fl_Widget* w, void* p){
  FeatureCheckButton *fv = (FeatureCheckButton*)w;
  size_t ind = (size_t)p;
  fv->pb->setFeature(keys[ind],fv->value());
}
void
ViewFeatures::cap_callback (Fl_Widget* w, void* p){
  FeatureValueInput *fv = (FeatureValueInput*)w;
  size_t ind = (size_t)p;
  fv->value(fv->clamp(fv->value()));
  fv->pb->setCapacity(keys[ind],fv->value());
}

void
ViewFeatures::build_feature_selector(int x, int y, int w, int h)
{
  if (pb==NULL) {
    std::cerr << "Error: No PatchBank given. Can't build GUI." << std::endl;
    return;
  }

  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w-2,h-2);
  int px = x+1, py=30, width=60, height=20, captwidth=350;

  std::vector<CapacityDescription>::const_iterator cit;
  FeatureValueInput *vi;
  Fl_Box *lbl;
  for (cit=pb->getCapacityBegin(); cit!= pb->getCapacityEnd(); ++cit){
    lbl = new Fl_Box(px,py,captwidth,height,0);
    lbl->copy_label((*cit).getHumanReadable().c_str());
    lbl->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);

    vi = new FeatureValueInput(px+captwidth,py,width,height,pb,
                               (*cit).getKey());
    vi->bounds((*cit).getMin(),(*cit).getMax());
    vi->when(FL_WHEN_ENTER_KEY);

    keys.push_back((*cit).getKey());
    wcap.push_back(vi);
    vi->argument(keys.size()-1);
    vi->callback(cap_callback);
    py+=height;
  }

  std::vector<FeatureDescription>::const_iterator fit;
  FeatureCheckButton *ckb;
  for (fit=pb->getFeatureBegin(); fit!=pb->getFeatureEnd(); ++fit){
    ckb = new FeatureCheckButton(px,py,200,20,
                                 "unlabeled",
                                 pb,
                                 (*fit).getKey());
    ckb->copy_label((*fit).getHumanReadable().c_str());
    keys.push_back((*fit).getKey());
    wfeat.push_back(ckb);
    ckb->argument(keys.size()-1);
    ckb->callback(feat_callback);
    py += height;
  }
  reloadWidgetValues();
  pb->addReloadListener(refreshViewFromData,this);

  scroll->end();
}

void ViewFeatures::reloadWidgetValues(){
    for(size_t i=0;i<wcap.size();++i){
        wcap[i]->reloadValue();
    }
    for(size_t i=0;i<wfeat.size();++i){
        wfeat[i]->reloadValue();
    }
}

ViewFeatures::ViewFeatures(int x, int y, int w, int h,
                           const char * name,
                           PatchBank *ipb)
  : Fl_Group(x, y, w, h, name), pb(ipb)
{
  build_feature_selector(x,y,w,h);
  this->end();
}
}
