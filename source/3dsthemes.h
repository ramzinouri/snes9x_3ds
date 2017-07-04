#ifndef _3DSTHEMES_H_
#define _3DSTHEMES_H_

#include "snes9x.h"


typedef struct 
{
    char *Name;
    uint32 topBGColor;
    uint32 menuBarColor;
    uint32 menuBackColor;
    uint32 menuTxtColor;
    uint32 menuTabTxtColor;
    uint32 selectedTabColor;
    uint32 selectedItemBackColor;
    uint32 selectedItemTextColor;
    uint32 selectedItemDescriptionTextColor;
    uint32 checkedItemTextColor;
    uint32 normalItemTextColor;
    uint32 normalItemDescriptionTextColor;
    uint32 disabledItemTextColor;
    uint32 headerItemTextColor;
    uint32 subtitleTextColor;
    uint32 dialogColor;
    uint32 dialogColorInfo;
    uint32 dialogColorError;
    uint32 dialogBackColor;
    uint32 dialogTextColor;
    uint32 dialogItemTextColor;
    uint32 dialogSelectedItemTextColor;
    uint32 dialogSelectedItemBackColor;
    uint32 progressBarColor;
    uint32 txtBottomColor;
    uint32 batteryChargeColor;

} Theme3ds;


extern Theme3ds Themes[2];
#define TOTALTHEMECOUNT 2



#endif