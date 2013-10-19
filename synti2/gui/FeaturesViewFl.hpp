#ifndef SYNTI2_GUI_VIEWFEATURES_FL
#define SYNTI2_GUI_VIEWFEATURES_FL
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include "PatchBank.hpp"
#include <vector>
#include <string>
namespace synti2gui {

  class FeatureCheckButton: public Fl_Check_Button {
  private:
    std::string _featkey;
  public:
    void featkey(std::string s){_featkey = s;}
    const std::string& featkey(){return _featkey;}
    FeatureCheckButton(int x, int y, int w, int h, const char *tit):
        Fl_Check_Button(x,y,w,h,tit){}
  };

  class ViewFeatures: public Fl_Group {
  private:
    static void feat_callback (Fl_Widget* w, void* p);
    PatchBank *pb; // we assume the PatchBank outlasts this view.
    std::vector<std::string> keys;
    void build_feature_selector(int x, int y, int w, int h);
  public:
    ViewFeatures(int,int,int,int,const char*,
                 PatchBank*);
  };
}

#endif
