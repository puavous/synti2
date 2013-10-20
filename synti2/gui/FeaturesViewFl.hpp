#ifndef SYNTI2_GUI_VIEWFEATURES_FL
#define SYNTI2_GUI_VIEWFEATURES_FL
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Input.H>
#include "PatchBank.hpp"
#include <vector>
#include <string>
namespace synti2gui
{

class FeatureCheckButton: public Fl_Check_Button
{
public:
    PatchBank *pb;
    FeatureCheckButton(int x, int y, int w, int h,
                       const char *tit, PatchBank *ipb):
        Fl_Check_Button(x,y,w,h,tit),pb(ipb) {}
};

class FeatureValueInput: public Fl_Value_Input
{
public:
    PatchBank *pb;
    FeatureValueInput(int x, int y, int w, int h,
                       PatchBank *ipb):
        Fl_Value_Input(x,y,w,h),pb(ipb) {}
};


class ViewFeatures: public Fl_Group
{
private:
    static std::vector<std::string> keys;
    static void feat_callback (Fl_Widget* w, void* p);
    static void cap_callback (Fl_Widget* w, void* p);
    /** The PatchBank that is controlled must outlast the view**/
    PatchBank *pb;
    void build_feature_selector(int x, int y, int w, int h);
public:
    ViewFeatures(int,int,int,int,const char*,
                 PatchBank*);
};
}

#endif
