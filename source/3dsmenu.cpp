#include <cstring>
#include <string.h>
#include <stdio.h>
#include <3ds.h>

#include "snes9x.h"
#include "memmap.h"
#include "port.h"

#include "3dsexit.h"
#include "3dsmenu.h"
#include "3dsgpu.h"
#include "3dsui.h"
#include "3dsthemes.h"
#include "3dssettings.h"
#include "lodepng.h"



#define CONSOLE_WIDTH           40
#define MENU_HEIGHT             (14)
#define DIALOG_HEIGHT           (5)

#define ANIMATE_TAB_STEPS 3


typedef struct
{
    SMenuItem   *MenuItems;
    char        SubTitle[256];
    char        *Title;
    char        *DialogText;
    int         ItemCount;
    int         FirstItemIndex;
    int         SelectedItemIndex;
} SMenuTab;


SMenuTab            menuTab[4];
SMenuTab            dialogTab;

int                 menuTabCount;
int                 currentMenuTab = 0;
bool                isDialog = false;
int                 lastProgress=0;

bool                transferGameScreen = false;
int                 transferGameScreenCount = 0;

bool                swapBuffer = true;
int dialogBackColor=0x000000;

//-------------------------------------------------------
// Sets a flag to tell the menu selector
// to transfer the emulator's rendered frame buffer
// to the actual screen's frame buffer.
//
// Usually you will set this to true during emulation,
// and set this to false when this program first runs.
//-------------------------------------------------------
void menu3dsSetTransferGameScreen(bool transfer)
{
    transferGameScreen = transfer;
    if (transfer)
        transferGameScreenCount = 2;
    else
        transferGameScreenCount = 0;

}



// Draw a black screen.
//
void menu3dsDrawBlackScreen(float opacity)
{
    ui3dsDrawRect(0, 0, 320, 240, 0x000000, opacity);    
}



void menu3dsSwapBuffersAndWaitForVBlank()
{
    if (transferGameScreenCount)
    {
        gpu3dsTransferToScreenBuffer();
        transferGameScreenCount --;
    }
    if (swapBuffer)
    {
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    else
    {
        gspWaitForVBlank();
    }
    
    swapBuffer = false;
}


void menu3dsDrawItems(
    SMenuTab *currentTab, int horizontalPadding, int menuStartY, int maxItems,
    int selectedItemBackColor,
    int selectedItemTextColor, 
    int selectedItemDescriptionTextColor,  
    int normalItemTextColor,
    int normalItemDescriptionTextColor,
    int disabledItemTextColor, 
    int headerItemTextColor, 
    int subtitleTextColor)
{
    int fontHeight = 13;
    char gauge[52];
    char menuTextBuffer[512];
    char selectedTextBuffer[512];
    
    // Display the subtitle
    if (currentTab->SubTitle[0])
    {
        maxItems--;
        snprintf (menuTextBuffer, 511, "%s", currentTab->SubTitle);
        ui3dsDrawStringWithNoWrapping(20, menuStartY, 300, menuStartY + fontHeight, 
            subtitleTextColor, HALIGN_LEFT, menuTextBuffer);
        menuStartY += fontHeight;
    }

    int line = 0;
    int color = 0x000000;

    // Draw all the individual items
    //
    for (int i = currentTab->FirstItemIndex;
        i < currentTab->ItemCount && i < currentTab->FirstItemIndex + maxItems; i++)
    {
        int y = line * fontHeight + menuStartY;

        if (currentTab->MenuItems[i].Text == NULL)
            snprintf (menuTextBuffer, 511, "");
        else
            snprintf (menuTextBuffer, 511, "%s", currentTab->MenuItems[i].Text);

        // Draw the selected background 
        //
        if (currentTab->SelectedItemIndex == i)
            ui3dsDrawRect(0, y, 320, y + 14, selectedItemBackColor);
        
        if (currentTab->MenuItems[i].Type == MENUITEM_HEADER1)
        {
            color = headerItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);
            ui3dsDrawRect(horizontalPadding, y + fontHeight - 1, 320 - horizontalPadding, y + fontHeight, color);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_HEADER2)
        {
            color = headerItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_DISABLED)
        {
            color = disabledItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_ACTION)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

            color = normalItemDescriptionTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemDescriptionTextColor;
            if (currentTab->MenuItems[i].Description != NULL)
            {
                snprintf(menuTextBuffer, 511, "%s", currentTab->MenuItems[i].Description);
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, menuTextBuffer);
            }
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_CHECKBOX)
        {
            if (currentTab->MenuItems[i].Value == 0)
            {
                color = disabledItemTextColor;
                if (currentTab->SelectedItemIndex == i)
                    color = selectedItemTextColor;
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

                snprintf(menuTextBuffer, 511, "\xfe");
                ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, menuTextBuffer);
            }
            else
            {
                color = normalItemTextColor;
                if (currentTab->SelectedItemIndex == i)
                    color = selectedItemTextColor;
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

                snprintf(menuTextBuffer, 511, "\xfd");
                ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, menuTextBuffer);
            }
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_GAUGE)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;

            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

            int max = 40;
            int diff = currentTab->MenuItems[i].GaugeMaxValue - currentTab->MenuItems[i].GaugeMinValue;
            int pos = (currentTab->MenuItems[i].Value - currentTab->MenuItems[i].GaugeMinValue) * (max - 1) / diff;

            for (int j = 0; j < max; j++)
                gauge[j] = (j == pos) ? '\xfa' : '\xfb';
            gauge[max] = 0;
            ui3dsDrawStringWithNoWrapping(245, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, gauge);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_PICKER)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;

            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 160, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

            snprintf(selectedTextBuffer, 511, "");
            if (currentTab->MenuItems[i].PickerItems != NULL)
            {
                for (int j = 0; j < currentTab->MenuItems[i].PickerItemCount; j++)
                {
                    SMenuItem *pickerItems = (SMenuItem *)currentTab->MenuItems[i].PickerItems;
                    if (pickerItems[j].ID == currentTab->MenuItems[i].Value)
                    {
                        snprintf(selectedTextBuffer, 511, "%s", pickerItems[j].Text);
                    }
                }
                ui3dsDrawStringWithNoWrapping(160, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, selectedTextBuffer);
            }
        }

        line ++;
    }


    // Draw the "up arrow" to indicate more options available at top
    //
    if (currentTab->FirstItemIndex != 0)
    {
        ui3dsDrawStringWithNoWrapping(320 - horizontalPadding, menuStartY, 320, menuStartY + fontHeight, disabledItemTextColor, HALIGN_CENTER, "\xf8");
    }

    // Draw the "down arrow" to indicate more options available at bottom
    //
    if (currentTab->FirstItemIndex + maxItems < currentTab->ItemCount)
    {
        ui3dsDrawStringWithNoWrapping(320 - horizontalPadding, menuStartY + (maxItems - 1) * fontHeight, 320, menuStartY + maxItems * fontHeight, disabledItemTextColor, HALIGN_CENTER, "\xf9");
    }
    
}

// Display the list of choices for selection
//
void menu3dsDrawMenu(int menuItemFrame, int translateY)
{
    SMenuTab *currentTab = &menuTab[currentMenuTab];

    char tempBuffer[CONSOLE_WIDTH];

    // Draw the flat background
    //
    ui3dsDrawRect(0, 0, 320, 24, Themes[settings3DS.Theme].menuBarColor);
    ui3dsDrawRect(0, 24, 320, 220, Themes[settings3DS.Theme].menuBackColor);
    ui3dsDrawRect(0, 220, 320, 240, Themes[settings3DS.Theme].menuBottomBarColor);

    // Draw the tabs at the top
    //
    for (int i = 0; i < menuTabCount; i++)
    {
        int color = i == currentMenuTab ? Themes[settings3DS.Theme].menuTxtColor : Themes[settings3DS.Theme].menuTxtUnselectedColor ;
        ui3dsDrawStringWithNoWrapping(i * 75 + 10, 6, (i+1)*75 + 10, 21, color, HALIGN_CENTER, 
            menuTab[i].Title);

        if (i == currentMenuTab)
            ui3dsDrawRect(i * 75 + 10, 21, (i+1)*75 + 10, 24, Themes[settings3DS.Theme].selectedTabColor);
    }

    // Shadows
    //ui3dsDrawRect(0, 23, 320, 24, 0xaaaaaa);
    //ui3dsDrawRect(0, 24, 320, 25, 0xcccccc);
    //ui3dsDrawRect(0, 25, 320, 27, 0xeeeeee);


    //battery display
    ui3dsDrawRect(287, 223, 315, 236, Themes[settings3DS.Theme].menuTxtColor, 1.0f);
    ui3dsDrawRect(284, 226, 287, 233, Themes[settings3DS.Theme].menuTxtColor, 1.0f);
    ui3dsDrawRect(289, 224, 313, 235, Themes[settings3DS.Theme].menuBottomBarColor, 1.0f);
    ui3dsDrawRect(291, 226, 311, 233, Themes[settings3DS.Theme].menuTxtColor, 0.5f);
    
    ptmuInit();
    
    u8 batteryChargeState = 0;
    u8 batteryLevel = 0;
    if(R_SUCCEEDED(PTMU_GetBatteryChargeState(&batteryChargeState)) && batteryChargeState) {
        ui3dsDrawRect(291, 226, 311, 233,Themes[settings3DS.Theme].batteryChargeColor, 1.0f);
    } else if(R_SUCCEEDED(PTMU_GetBatteryLevel(&batteryLevel))) {
        ui3dsDrawRect(311-5*(batteryLevel-1), 226, 311, 233, Themes[settings3DS.Theme].menuTxtColor, 1.0f);

    } else {
        ui3dsDrawRect(311, 226, 311, 233, Themes[settings3DS.Theme].menuTxtColor, 1.0f);
    }
 
    ptmuExit();


    ui3dsDrawStringWithNoWrapping(10, 223, 310, 240, Themes[settings3DS.Theme].menuTxtColor, HALIGN_LEFT,
        "\xF6:Select \xF0:Ok  \xF1:Cancel");
    //ui3dsDrawStringWithNoWrapping(10, 223, 310, 240, 0xFFFFFF, HALIGN_RIGHT,
    //    "SNES9x for 3DS " SNES9X_VERSION);
    char verText[32];
    if(batteryChargeState!=0)
        snprintf(verText, 32, "%s \xF7", REVISION);
    else
        snprintf(verText, 32, "%s ", REVISION);

    ui3dsDrawStringWithNoWrapping(10, 223, 282, 240, Themes[settings3DS.Theme].menuTxtColor, HALIGN_RIGHT,verText);


    int line = 0;
    int maxItems = MENU_HEIGHT;
    int menuStartY = 29;

    ui3dsSetTranslate(menuItemFrame * 3, translateY);

    if (menuItemFrame == 0)
    {
        menu3dsDrawItems(
            currentTab, 20, menuStartY, maxItems,
            Themes[settings3DS.Theme].selectedItemBackColor,       // selectedItemBackColor
            Themes[settings3DS.Theme].selectedItemTextColor,       // selectedItemTextColor
            Themes[settings3DS.Theme].selectedItemDescriptionTextColor,       // selectedItemDescriptionTextColor

            Themes[settings3DS.Theme].normalItemTextColor,       // normalItemTextColor      
            Themes[settings3DS.Theme].normalItemDescriptionTextColor,       // normalItemDescriptionTextColor      
            Themes[settings3DS.Theme].disabledItemTextColor,       // disabledItemTextColor
            Themes[settings3DS.Theme].headerItemTextColor,       // headerItemTextColor
            Themes[settings3DS.Theme].subtitleTextColor);      // subtitleTextColor
    }
    else
    {
        if (menuItemFrame < 0)
            menuItemFrame = -menuItemFrame;
        float alpha = (float)(ANIMATE_TAB_STEPS - menuItemFrame + 1) / (ANIMATE_TAB_STEPS + 1);

        int white = ui3dsApplyAlphaToColor(0xFFFFFF, 1.0f - alpha);
        
         menu3dsDrawItems(
            currentTab, 20, menuStartY, maxItems,
            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].selectedItemBackColor, alpha) + white,       // selectedItemBackColor
            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].selectedItemTextColor, alpha) + white,       // selectedItemTextColor
            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].selectedItemDescriptionTextColor, alpha) + white,       // selectedItemDescriptionTextColor

            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].normalItemTextColor, alpha) + white,       // normalItemTextColor      
            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].normalItemDescriptionTextColor, alpha) + white,       // normalItemDescriptionTextColor      
            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].disabledItemTextColor, alpha) + white,       // disabledItemTextColor
            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].headerItemTextColor, alpha) + white,       // headerItemTextColor
            ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].subtitleTextColor, alpha) + white);      // subtitleTextColor       

    }

      
/*
    ui3dsDrawStringWithWrapping(10, 10, 100, 70, 0xff0000, HALIGN_LEFT, "This is a long text that should wrap to a few lines!");
    ui3dsDrawStringWithWrapping(10, 90, 100, 150, 0xff0000, HALIGN_RIGHT, "This is a long text that should wrap and right justify itself!");
    ui3dsDrawStringWithWrapping(10, 170, 100, 230, 0xff0000, HALIGN_CENTER, "This is a long text that should wrap and center justify itself!");
    ui3dsDrawStringWithNoWrapping(110, 10, 200, 70, 0xff0000, HALIGN_LEFT, "This is a long text will be truncated");
    ui3dsDrawStringWithNoWrapping(110, 90, 200, 150, 0xff0000, HALIGN_CENTER, "This is a long text will be truncated");
    ui3dsDrawStringWithNoWrapping(110, 170, 200, 230, 0xff0000, HALIGN_RIGHT, "This is a long text will be truncated");
*/
}


void menu3dsDrawDialog()
{
    // Dialog's Background
    int dialogBackColor2 = ui3dsApplyAlphaToColor(dialogBackColor, 0.9f);
    ui3dsDrawRect(0, 0, 320, 75, dialogBackColor2);
    ui3dsDrawRect(0, 75, 320, 160, dialogBackColor);

    // Left trim the dialog title
    int len = strlen(dialogTab.Title);
    int startChar = 0;
    for (int i = 0; i < len; i++)
        if (dialogTab.Title[i] != ' ')
        {
            startChar = i;
            break;
        }

    // Draw the dialog's title and descriptive text
    int dialogTitleTextColor = 
        ui3dsApplyAlphaToColor(dialogBackColor, 0.5f) + 
        ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].dialogTextColor, 0.5f);
    ui3dsDrawStringWithNoWrapping(30, 10, 290, 25, dialogTitleTextColor, HALIGN_LEFT, &dialogTab.Title[startChar]);
    ui3dsDrawStringWithWrapping(30, 30, 290, 70, Themes[settings3DS.Theme].dialogTextColor, HALIGN_LEFT, dialogTab.DialogText);

    // Draw the selectable items.
    int dialogItemDescriptionTextColor = dialogTitleTextColor;
    menu3dsDrawItems(
        &dialogTab, 30, 80, DIALOG_HEIGHT,
        Themes[settings3DS.Theme].dialogSelectedItemBackColor,        // selectedItemBackColor
        Themes[settings3DS.Theme].dialogSelectedItemTextColor,        // selectedItemTextColor
        dialogItemDescriptionTextColor,     // selectedItemDescriptionColor

        Themes[settings3DS.Theme].dialogItemTextColor,                // normalItemTextColor
        dialogItemDescriptionTextColor,     // normalItemDescriptionTextColor
        dialogItemDescriptionTextColor,     // disabledItemTextColor
        Themes[settings3DS.Theme].dialogItemTextColor,                // headerItemTextColor
        Themes[settings3DS.Theme].dialogItemTextColor                 // subtitleTextColor
        );
}

void menu3dsDrawDialogProgressBar(float per)
{
    aptMainLoop();
    menu3dsDrawEverything();

    char text[15];
    snprintf(text, 14, "%u %%", (unsigned char)per);
    aptMainLoop();
    int y = 80;

    ui3dsDrawRect(30, 100+y, 290, 120+y, Themes[settings3DS.Theme].progressBarColor);
    ui3dsDrawRect(31, 101+y, 289, 119+y, dialogBackColor);

    ui3dsDrawRect(33, 103+y, 33+(254*per)/100, 117+y, Themes[settings3DS.Theme].progressBarColor);

    ui3dsDrawRect(30, 120+y, 290, 135+y, dialogBackColor);
    ui3dsDrawStringWithNoWrapping(30, 120+y, 290, 135+y, Themes[settings3DS.Theme].dialogTextColor, HALIGN_LEFT, text);

 
    menu3dsSwapBuffersAndWaitForVBlank();
    swapBuffer = true;

}


void menu3dsDrawEverything(int menuFrame, int menuItemsFrame, int dialogFrame)
{
    if (!isDialog)
    {
        int y = 0 + menuFrame * menuFrame * 120 / 32;

        ui3dsSetViewport(0, 0, 320, 240);
        ui3dsSetTranslate(0, 0);
        ui3dsDrawRect(0, 0, 400, y, 0x000000);
        ui3dsSetTranslate(0, y);
        menu3dsDrawMenu(menuItemsFrame, y);
    }
    else
    {
        int y = 80 + dialogFrame * dialogFrame * 80 / 32;

        ui3dsSetViewport(0, 0, 320, y);
        //ui3dsBlitToFrameBuffer(savedBuffer, 1.0f - (float)(8 - dialogFrame) / 10);
        ui3dsSetTranslate(0, 0);
        menu3dsDrawMenu(0, 0);
        ui3dsDrawRect(0, 0, 320, y, 0x000000, (float)(8 - dialogFrame) / 10);

        ui3dsSetViewport(0, 0, 320, 240);
        ui3dsSetTranslate(0, y);
        menu3dsDrawDialog();
        ui3dsSetTranslate(0, 0);
    }
    swapBuffer = true;
}


SMenuTab *menu3dsAnimateTab(int direction)
{
    SMenuTab *currentTab = &menuTab[currentMenuTab];

    if (direction < 0)
    {
        for (int i = 1; i <= ANIMATE_TAB_STEPS; i++)
        {
            aptMainLoop();
            menu3dsDrawEverything(0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }

        currentMenuTab--;
        if (currentMenuTab < 0)
            currentMenuTab = menuTabCount - 1;
        currentTab = &menuTab[currentMenuTab];
        
        for (int i = -ANIMATE_TAB_STEPS; i <= 0; i++)
        {
            aptMainLoop();
            menu3dsDrawEverything(0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }
    }
    else if (direction > 0)
    {
        for (int i = -1; i >= -ANIMATE_TAB_STEPS; i--)
        {
            aptMainLoop();
            menu3dsDrawEverything(0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }

        currentMenuTab++;
        if (currentMenuTab >= menuTabCount)
            currentMenuTab = 0;
        currentTab = &menuTab[currentMenuTab];
        
        for (int i = ANIMATE_TAB_STEPS; i >= 0; i--)
        {
            aptMainLoop();
            menu3dsDrawEverything(0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }
    }
    return currentTab;
}


static u32 lastKeysHeld = 0xffffff;
static u32 thisKeysHeld = 0;


// Displays the menu and allows the user to select from
// a list of choices.
//
int menu3dsMenuSelectItem(void (*itemChangedCallback)(int ID, int value))
{
    int framesDKeyHeld = 0;
    int returnResult = -1;

    char menuTextBuffer[512];

    SMenuTab *currentTab = &menuTab[currentMenuTab];

    if (isDialog)
        currentTab = &dialogTab;

    for (int i = 0; i < 2; i ++)
    {
        aptMainLoop();
        menu3dsDrawEverything();
        menu3dsSwapBuffersAndWaitForVBlank();

        hidScanInput();
        lastKeysHeld = hidKeysHeld();
    }

    while (aptMainLoop())
    {   
        if (appExiting)
        {
            returnResult = -1;
            break;
        }
        menu3dsDrawEverything();
        gpu3dsCheckSlider();
        hidScanInput();
        thisKeysHeld = hidKeysHeld();

        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        int maxItems = MENU_HEIGHT;
        if (isDialog)
            maxItems = DIALOG_HEIGHT;

        if (currentTab->SubTitle[0])
        {
            maxItems--;
        }

        if (thisKeysHeld & KEY_UP || thisKeysHeld & KEY_DOWN)
            framesDKeyHeld ++;
        else
            framesDKeyHeld = 0;
        if (keysDown & KEY_B)
        {
            returnResult = -1;
            break;
        }
        if ((keysDown & KEY_RIGHT) || (keysDown & KEY_R))
        {
            if (!isDialog)
            {
                if (keysDown & KEY_RIGHT &&
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_GAUGE)
                {
                    if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value <
                        currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
                    {
                        currentTab->MenuItems[currentTab->SelectedItemIndex].Value ++ ;
                    }
                    menu3dsDrawEverything();
                }
                else
                {
                    currentTab = menu3dsAnimateTab(+1);
                }
            }
        }
        if ((keysDown & KEY_LEFT) || (keysDown & KEY_L))
        {
            if (!isDialog)
            {
                if (keysDown & KEY_LEFT &&
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_GAUGE)
                {
                    // Gauge adjustment
                    if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value >
                        currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue)
                    {
                        currentTab->MenuItems[currentTab->SelectedItemIndex].Value -- ;
                    }
                    menu3dsDrawEverything();
                }
                else
                {
                    currentTab = menu3dsAnimateTab(-1);
                }
            }
        }
        if (keysDown & KEY_START || keysDown & KEY_A)
        {
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_ACTION)
            {
                returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
                break;
            }
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_CHECKBOX)
            {
                if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value == 0)
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value = 1;
                else
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value = 0;
                menu3dsDrawEverything();
            }
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_PICKER)
            {
                snprintf(menuTextBuffer, 511, "%s", currentTab->MenuItems[currentTab->SelectedItemIndex].Text);
                int resultValue = menu3dsShowDialog(menuTextBuffer, 
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerDescription,
                    Themes[settings3DS.Theme].dialogColorPicker,
                    (SMenuItem *)currentTab->MenuItems[currentTab->SelectedItemIndex].PickerItems,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerItemCount,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value
                    );
                if (resultValue != -1)
                {
                    if (itemChangedCallback)
                        itemChangedCallback(currentTab->MenuItems[currentTab->SelectedItemIndex].ID, resultValue);
                    
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value = resultValue;
                }
                menu3dsDrawEverything();
                menu3dsHideDialog();


            }
        }
        if (keysDown & KEY_UP || ((thisKeysHeld & KEY_UP) && (framesDKeyHeld > 30) && (framesDKeyHeld % 2 == 0)))
        {
            int moveCursorTimes = 0;

            do
            {
                if (thisKeysHeld & KEY_X)
                {
                    currentTab->SelectedItemIndex -= 15;
                    if (currentTab->SelectedItemIndex < 0)
                        currentTab->SelectedItemIndex = 0;
                }
                else
                {
                    currentTab->SelectedItemIndex--;
                    if (currentTab->SelectedItemIndex < 0)
                    {
                        currentTab->SelectedItemIndex = currentTab->ItemCount - 1;
                    }
                }
                moveCursorTimes++;
            }
            while (
                (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_DISABLED ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER1 ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER2
                ) &&
                moveCursorTimes < currentTab->ItemCount);

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            menu3dsDrawEverything();

        }
        if (keysDown & KEY_DOWN || ((thisKeysHeld & KEY_DOWN) && (framesDKeyHeld > 30) && (framesDKeyHeld % 2 == 0)))
        {
            int moveCursorTimes = 0;
            do
            {
                if (thisKeysHeld & KEY_X)
                {
                    currentTab->SelectedItemIndex += 15;
                    if (currentTab->SelectedItemIndex >= currentTab->ItemCount)
                        currentTab->SelectedItemIndex = currentTab->ItemCount - 1;
                }
                else
                {
                    currentTab->SelectedItemIndex++;
                    if (currentTab->SelectedItemIndex >= currentTab->ItemCount)
                    {
                        currentTab->SelectedItemIndex = 0;
                        currentTab->FirstItemIndex = 0;
                    }
                }
                moveCursorTimes++;
            }
            while (
                (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_DISABLED ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER1 ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER2
                ) &&
                moveCursorTimes < currentTab->ItemCount);

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            menu3dsDrawEverything();
        }

        menu3dsSwapBuffersAndWaitForVBlank();
    }

    return returnResult;
    
}



void menu3dsAddTab(char *title, SMenuItem *menuItems, int itemCount)
{
    SMenuTab *currentTab = &menuTab[menuTabCount];

    currentTab->Title = title;
    currentTab->MenuItems = menuItems;
    currentTab->ItemCount = itemCount;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID > -1)
        {
            currentTab->SelectedItemIndex = i;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + MENU_HEIGHT)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - MENU_HEIGHT + 1;
            break;
        }
    }

    menuTabCount++;
}



void menu3dsSetTabSubTitle(int tabIndex, char *subtitle)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    currentTab->SubTitle[0] = 0;
    if (subtitle != NULL)
        strncpy(currentTab->SubTitle, subtitle, 255);
}

void menu3dsSetCurrentMenuTab(int tabIndex)
{
    currentMenuTab = tabIndex;
}


void menu3dsSetSelectedItemIndexByID(int tabIndex, int ID)
{
    currentMenuTab = tabIndex;

    SMenuTab *currentTab = &menuTab[tabIndex];

    int maxItems = MENU_HEIGHT;
    if (currentTab->SubTitle[0])
        maxItems--;

    for (int i = 0; i < currentTab->ItemCount; i++)
    {
        if (currentTab->MenuItems[i].ID == ID)
        {
            currentTab->SelectedItemIndex = i;

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            break;
        }
    }
}


void menu3dsSetValueByID(int tabIndex, int ID, int value)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    for (int i = 0; i < currentTab->ItemCount; i++)
    {
        if (currentTab->MenuItems[i].ID == ID)
        {
            currentTab->MenuItems[i].Value = value;
            break;
        }
    }
}


int menu3dsGetValueByID(int tabIndex, int ID)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    for (int i = 0; i < currentTab->ItemCount; i++)
    {
        if (currentTab->MenuItems[i].ID == ID)
        {
            return currentTab->MenuItems[i].Value;
        }
    }
    return -1;
}


void menu3dsClearMenuTabs()
{
    menuTabCount = 0;
    currentMenuTab = 0;
}



int menu3dsShowMenu(void (*itemChangedCallback)(int ID, int value), bool animateMenu)
{
    isDialog = false;

    if (animateMenu)
    {
        for (int f = 8; f >= 0; f--)
        {
            aptMainLoop();
            menu3dsDrawEverything(f, 0, 0);    
            menu3dsSwapBuffersAndWaitForVBlank();  
        }
    }

    return menu3dsMenuSelectItem(itemChangedCallback);

}

void menu3dsHideMenu()
{
    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        menu3dsDrawEverything(f, 0, 0);    
        menu3dsSwapBuffersAndWaitForVBlank();  
    }    
    ui3dsSetTranslate(0, 0);
}

int menu3dsShowDialog(char *title, char *dialogText, int newDialogBackColor, SMenuItem *menuItems, int itemCount, int selectedID)
{
    SMenuTab *currentTab = &dialogTab;

    dialogBackColor = newDialogBackColor;

    currentTab->Title = title;
    currentTab->DialogText = dialogText;
    currentTab->MenuItems = menuItems;
    currentTab->ItemCount = itemCount;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;

    for (int i = 0; i < itemCount; i++)
    {
        if ((selectedID == -1 && menuItems[i].ID > -1) || 
            menuItems[i].ID == selectedID)
        {
            currentTab->SelectedItemIndex = i;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + DIALOG_HEIGHT)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - DIALOG_HEIGHT + 1;
            break;
        }
    }

    // fade the dialog fade in
    //
    aptMainLoop();
    menu3dsDrawEverything();
    menu3dsSwapBuffersAndWaitForVBlank();  
    //ui3dsCopyFromFrameBuffer(savedBuffer);

    isDialog = true;
    for (int f = 8; f >= 0; f--)
    {
        aptMainLoop();
        menu3dsDrawEverything(0, 0, f);    
        menu3dsSwapBuffersAndWaitForVBlank();  
    }

    // Execute the dialog and return result.
    //
    if (itemCount > 0)
    {
        int result = menu3dsMenuSelectItem(NULL);

        return result;
    }
    return 0;
}

int menu3dsShowDialogProgress(char *title, char *dialogText, int newDialogBackColor)
{
    lastProgress=0;
    SMenuTab *currentTab = &dialogTab;

    dialogBackColor = newDialogBackColor;

    currentTab->Title = title;
    currentTab->DialogText = dialogText;
    currentTab->MenuItems = NULL;
    currentTab->ItemCount = NULL;

    currentTab->FirstItemIndex = NULL;
    currentTab->SelectedItemIndex = NULL;


    // fade the dialog fade in
    //
    aptMainLoop();
    menu3dsDrawEverything();
    menu3dsSwapBuffersAndWaitForVBlank();  
    //ui3dsCopyFromFrameBuffer(savedBuffer);

    isDialog = true;
    for (int f = 8; f >= 0; f--)
    {
        aptMainLoop();
        menu3dsDrawEverything(0, 0, f);    
        menu3dsSwapBuffersAndWaitForVBlank();  
    }
    return 0;
}

void menu3dsUpdateDialogProgress(int pos,int len)
{
    float p=0;
    if(len!=0)
        p=(len-pos)*100/len;
    if(p>lastProgress)
    {
        menu3dsDrawDialogProgressBar(p);
        lastProgress=lastProgress+1;
    }
}


void menu3dsHideDialog()
{
    // fade the dialog out
    //
    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        menu3dsDrawEverything(0, 0, f);
        menu3dsSwapBuffersAndWaitForVBlank();    
    }

    isDialog = false;
    
    // draw the updated menu
    //
    aptMainLoop();
    menu3dsDrawEverything();
    menu3dsSwapBuffersAndWaitForVBlank();  
    
}

int menu3dsShowDialogInfo(char *title, char *dialogText, int newDialogBackColor)
{
    SMenuItem optionsForOk[] = {
        { 2, 0, "OK", NULL, 0 }
    };

    SMenuTab *currentTab = &dialogTab;

    dialogBackColor = newDialogBackColor;

    currentTab->Title = title;
    currentTab->DialogText = dialogText;
    currentTab->MenuItems = optionsForOk;
    currentTab->ItemCount = 1;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;
    if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + DIALOG_HEIGHT)
            currentTab->FirstItemIndex = currentTab->SelectedItemIndex - DIALOG_HEIGHT + 1;

    // fade the dialog fade in

    aptMainLoop();
    menu3dsDrawEverything();
    menu3dsSwapBuffersAndWaitForVBlank();  

    isDialog = true;

    for (int f = 8; f >= 0; f--)
    {
        aptMainLoop();
        
        int y = 60 + f * f * 60 / 32;

        ui3dsSetViewport(0, 0, 320, y);
        ui3dsSetTranslate(0, 0);
        menu3dsDrawMenu(0, 0);
        ui3dsDrawRect(0, 0, 320, y, 0x000000, (float)(8 - f) / 10);

        ui3dsSetViewport(0, 0, 320, 240);
        ui3dsSetTranslate(0, y);

        // Dialog's Background
        int dialogBackColor2 = ui3dsApplyAlphaToColor(dialogBackColor, 0.9f);
        ui3dsDrawRect(0, 0, 320, 28, dialogBackColor2);
        ui3dsDrawRect(0, 28, 320, 180, dialogBackColor);

        // Left trim the dialog title
        int len = strlen(dialogTab.Title);
        int startChar = 0;
        for (int i = 0; i < len; i++)
            if (dialogTab.Title[i] != ' ')
            {
                startChar = i;
                break;
         }

    // Draw the dialog's title and descriptive text
        int dialogTitleTextColor = 
        ui3dsApplyAlphaToColor(dialogBackColor, 0.5f) + 
        ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].dialogTextColor, 0.5f);
        ui3dsDrawStringWithNoWrapping(30, 10, 290, 25, dialogTitleTextColor, HALIGN_LEFT, &dialogTab.Title[startChar]);
        ui3dsDrawStringWithWrapping(30, 30, 290, 150, Themes[settings3DS.Theme].dialogTextColor, HALIGN_LEFT, dialogTab.DialogText);

        // Draw the selectable items.
        int dialogItemDescriptionTextColor = dialogTitleTextColor;
        menu3dsDrawItems(
            &dialogTab, 30, 160, DIALOG_HEIGHT,
            Themes[settings3DS.Theme].dialogSelectedItemBackColor,        // selectedItemBackColor
            Themes[settings3DS.Theme].dialogSelectedItemTextColor,        // selectedItemTextColor
            dialogItemDescriptionTextColor,     // selectedItemDescriptionColor

            Themes[settings3DS.Theme].dialogItemTextColor,                // normalItemTextColor
            dialogItemDescriptionTextColor,     // normalItemDescriptionTextColor
            dialogItemDescriptionTextColor,     // disabledItemTextColor
            Themes[settings3DS.Theme].dialogItemTextColor,                // headerItemTextColor
            Themes[settings3DS.Theme].dialogItemTextColor                 // subtitleTextColor
            );
        
        ui3dsSetTranslate(0, 0);
        swapBuffer = true;     
        menu3dsSwapBuffersAndWaitForVBlank();  
    }

    for (int i = 0; i < 2; i ++)
    {
        hidScanInput();
        lastKeysHeld = hidKeysHeld();
    }

    while (aptMainLoop())
    {   
        if (appExiting)
            break;
        hidScanInput();
        thisKeysHeld = hidKeysHeld();

        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if ((keysDown & KEY_B)||(keysDown&KEY_A))
            break;
    }

    // fade the dialog fade out

    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        
        int y = 60 + f * f * 60 / 32;

        ui3dsSetViewport(0, 0, 320, y);
        ui3dsSetTranslate(0, 0);
        menu3dsDrawMenu(0, 0);
        ui3dsDrawRect(0, 0, 320, y, 0x000000, (float)(8 - f) / 10);

        ui3dsSetViewport(0, 0, 320, 240);
        ui3dsSetTranslate(0, y);

        // Dialog's Background
        int dialogBackColor2 = ui3dsApplyAlphaToColor(dialogBackColor, 0.9f);
        ui3dsDrawRect(0, 0, 320, 28, dialogBackColor2);
        ui3dsDrawRect(0, 28, 320, 180, dialogBackColor);

        // Left trim the dialog title
        int len = strlen(dialogTab.Title);
        int startChar = 0;
        for (int i = 0; i < len; i++)
            if (dialogTab.Title[i] != ' ')
            {
                startChar = i;
                break;
         }

    // Draw the dialog's title and descriptive text
        int dialogTitleTextColor = 
        ui3dsApplyAlphaToColor(dialogBackColor, 0.5f) + 
        ui3dsApplyAlphaToColor(Themes[settings3DS.Theme].dialogTextColor, 0.5f);
        ui3dsDrawStringWithNoWrapping(30, 10, 290, 25, dialogTitleTextColor, HALIGN_LEFT, &dialogTab.Title[startChar]);
        ui3dsDrawStringWithWrapping(30, 30, 290, 150, Themes[settings3DS.Theme].dialogTextColor, HALIGN_LEFT, dialogTab.DialogText);

        // Draw the selectable items.
        int dialogItemDescriptionTextColor = dialogTitleTextColor;
        menu3dsDrawItems(
            &dialogTab, 30, 160, DIALOG_HEIGHT,
            Themes[settings3DS.Theme].dialogSelectedItemBackColor,        // selectedItemBackColor
            Themes[settings3DS.Theme].dialogSelectedItemTextColor,        // selectedItemTextColor
            dialogItemDescriptionTextColor,     // selectedItemDescriptionColor

            Themes[settings3DS.Theme].dialogItemTextColor,                // normalItemTextColor
            dialogItemDescriptionTextColor,     // normalItemDescriptionTextColor
            dialogItemDescriptionTextColor,     // disabledItemTextColor
            Themes[settings3DS.Theme].dialogItemTextColor,                // headerItemTextColor
            Themes[settings3DS.Theme].dialogItemTextColor                 // subtitleTextColor
            );
        
        ui3dsSetTranslate(0, 0);
        swapBuffer = true;     
        menu3dsSwapBuffersAndWaitForVBlank();  
    }

    return 0;
}

bool menu3dsTakeScreenshot(const char* path)
{
    u32 buffsize = 400*240*3;
    u8* tempbuf = (u8*)linearAlloc(buffsize);
    if (tempbuf == NULL)
        return false;
    memset(tempbuf, 0, buffsize);

    u8* framebuf = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    for (int y = 0; y < 240; y++)
        for (int x = 0; x < 400; x++)
        {
            int si = (((239 - y) + (x * 240)) * 4);
            int di =(x + y * 400 ) * 3;

            tempbuf[di+0] = framebuf[si+3];
            tempbuf[di+1] = framebuf[si+2];
            tempbuf[di+2] = framebuf[si+1];
        }

    unsigned char* png;
    size_t pngsize;

    unsigned error = lodepng_encode24(&png, &pngsize, tempbuf, 400, 240);
    if(!error) lodepng_save_file(png, pngsize, path);

    free (png);
    linearFree(tempbuf);
    return true;
};

void printROMInfo(char *txt)
{
    char temp[100];
    sprintf(txt,"File: %s\nName: %s\n",Memory.ROMFilename,Memory.ROMName);
    sprintf(temp, "Speed: %02X/%s\nROM Map: %s\n", Memory.ROMSpeed, ((Memory.ROMSpeed&0x10)!=0)?"FastROM":"SlowROM",(Memory.HiROM)?"HiROM":"LoROM");
    strcat(txt, temp);
    strcat(txt, "Kart contents: ");
    strcat(txt, Memory.KartContents ());
    strcat(txt, "\nOutput: ");
    if(Memory.ROMRegion>12||Memory.ROMRegion<2)
        strcat(txt, "NTSC 60Hz");
    else strcat(txt, "PAL 50Hz");

#define NOTKNOWN "Unknown Company "
    strcat(txt, "\nLicensee: ");
    int tmp=atoi(Memory.CompanyId);
    if(tmp==0)
        tmp=(Memory.HiROM)?Memory.ROM[0x0FFDA]:Memory.ROM[0x7FDA];
    switch(tmp)
        //				switch(((Memory.ROMSpeed&0x0F)!=0)?Memory.ROM[0x0FFDA]:Memory.ROM[0x7FDA])
        //				switch(atoi(Memory.CompanyId))
        //				switch(((Memory.CompanyId[0]-'0')*16)+(Memory.CompanyId[1]-'0'))
    {
        case 0:strcat(txt, "INVALID COMPANY");break;
        case 1:strcat(txt, "Nintendo");break;
        case 2:strcat(txt, "Ajinomoto");break;
        case 3:strcat(txt, "Imagineer-Zoom");break;
        case 4:strcat(txt, "Chris Gray Enterprises Inc.");break;
        case 5:strcat(txt, "Zamuse");break;
        case 6:strcat(txt, "Falcom");break;
        case 7:strcat(txt, NOTKNOWN "7");break;
        case 8:strcat(txt, "Capcom");break;
        case 9:strcat(txt, "HOT-B");break;
        case 10:strcat(txt, "Jaleco");break;
        case 11:strcat(txt, "Coconuts");break;
        case 12:strcat(txt, "Rage Software");break;
        case 13:strcat(txt, "Micronet"); break; //Acc. ZFE
        case 14:strcat(txt, "Technos");break;
        case 15:strcat(txt, "Mebio Software");break;
        case 16:strcat(txt, "SHOUEi System"); break; //Acc. ZFE
        case 17:strcat(txt, "Starfish");break; //UCON 64
        case 18:strcat(txt, "Gremlin Graphics");break;
        case 19:strcat(txt, "Electronic Arts");break;
        case 20:strcat(txt, "NCS / Masaya"); break; //Acc. ZFE
        case 21:strcat(txt, "COBRA Team");break;
        case 22:strcat(txt, "Human/Field");break;
        case 23:strcat(txt, "KOEI");break;
        case 24:strcat(txt, "Hudson Soft");break;
        case 25:strcat(txt, "Game Village");break;//uCON64
        case 26:strcat(txt, "Yanoman");break;
        case 27:strcat(txt, NOTKNOWN "27");break;
        case 28:strcat(txt, "Tecmo");break;
        case 29:strcat(txt, NOTKNOWN "29");break;
        case 30:strcat(txt, "Open System");break;
        case 31:strcat(txt, "Virgin Games");break;
        case 32:strcat(txt, "KSS");break;
        case 33:strcat(txt, "Sunsoft");break;
        case 34:strcat(txt, "POW");break;
        case 35:strcat(txt, "Micro World");break;
        case 36:strcat(txt, NOTKNOWN "36");break;
        case 37:strcat(txt, NOTKNOWN "37");break;
        case 38:strcat(txt, "Enix");break;
        case 39:strcat(txt, "Loriciel/Electro Brain");break;//uCON64
        case 40:strcat(txt, "Kemco");break;
        case 41:strcat(txt, "Seta Co.,Ltd.");break;
        case 42:strcat(txt, "Culture Brain"); break; //Acc. ZFE
        case 43:strcat(txt, "Irem Japan");break;//Irem? Gun Force J
        case 44:strcat(txt, "Pal Soft"); break; //Acc. ZFE
        case 45:strcat(txt, "Visit Co.,Ltd.");break;
        case 46:strcat(txt, "INTEC Inc."); break; //Acc. ZFE
        case 47:strcat(txt, "System Sacom Corp."); break; //Acc. ZFE
        case 48:strcat(txt, "Viacom New Media");break; //Zoop!
        case 49:strcat(txt, "Carrozzeria");break;
        case 50:strcat(txt, "Dynamic");break;
        case 51:strcat(txt, "Nintendo");break;
        case 52:strcat(txt, "Magifact");break;
        case 53:strcat(txt, "Hect");break;
        case 54:strcat(txt, NOTKNOWN "54");break;
        case 55:strcat(txt, NOTKNOWN "55");break;
        case 56:strcat(txt, "Capcom Europe");break;//Capcom? BOF2(E) MM7 (E)
        case 57:strcat(txt, "Accolade Europe");break;//Accolade?Bubsy 2 (E)
        case 58:strcat(txt, NOTKNOWN "58");break;
        case 59:strcat(txt, "Arcade Zone");break;//uCON64
        case 60:strcat(txt, "Empire Software");break;
        case 61:strcat(txt, "Loriciel");break;
        case 62:strcat(txt, "Gremlin Graphics"); break; //Acc. ZFE
        case 63:strcat(txt, NOTKNOWN "63");break;
        case 64:strcat(txt, "Seika Corp.");break;
        case 65:strcat(txt, "UBI Soft");break;
        case 66:strcat(txt, NOTKNOWN "66");break;
        case 67:strcat(txt, NOTKNOWN "67");break;
        case 68:strcat(txt, "LifeFitness Exertainment");break;//?? Exertainment Mountain Bike Rally (U).zip
        case 69:strcat(txt, NOTKNOWN "69");break;
        case 70:strcat(txt, "System 3");break;
        case 71:strcat(txt, "Spectrum Holobyte");break;
        case 72:strcat(txt, NOTKNOWN "72");break;
        case 73:strcat(txt, "Irem");break;
        case 74:strcat(txt, NOTKNOWN "74");break;
        case 75:strcat(txt, "Raya Systems/Sculptured Software");break;
        case 76:strcat(txt, "Renovation Products");break;
        case 77:strcat(txt, "Malibu Games/Black Pearl");break;
        case 78:strcat(txt, NOTKNOWN "78");break;
        case 79:strcat(txt, "U.S. Gold");break;
        case 80:strcat(txt, "Absolute Entertainment");break;
        case 81:strcat(txt, "Acclaim");break;
        case 82:strcat(txt, "Activision");break;
        case 83:strcat(txt, "American Sammy");break;
        case 84:strcat(txt, "GameTek");break;
        case 85:strcat(txt, "Hi Tech Expressions");break;
        case 86:strcat(txt, "LJN Toys");break;
        case 87:strcat(txt, NOTKNOWN "87");break;
        case 88:strcat(txt, NOTKNOWN "88");break;
        case 89:strcat(txt, NOTKNOWN "89");break;
        case 90:strcat(txt, "Mindscape");break;
        case 91:strcat(txt, "Romstar, Inc."); break; //Acc. ZFE
        case 92:strcat(txt, NOTKNOWN "92");break;
        case 93:strcat(txt, "Tradewest");break;
        case 94:strcat(txt, NOTKNOWN "94");break;
        case 95:strcat(txt, "American Softworks Corp.");break;
        case 96:strcat(txt, "Titus");break;
        case 97:strcat(txt, "Virgin Interactive Entertainment");break;
        case 98:strcat(txt, "Maxis");break;
        case 99:strcat(txt, "Origin/FCI/Pony Canyon");break;//uCON64
        case 100:strcat(txt, NOTKNOWN "100");break;
        case 101:strcat(txt, NOTKNOWN "101");break;
        case 102:strcat(txt, NOTKNOWN "102");break;
        case 103:strcat(txt, "Ocean");break;
        case 104:strcat(txt, NOTKNOWN "104");break;
        case 105:strcat(txt, "Electronic Arts");break;
        case 106:strcat(txt, NOTKNOWN "106");break;
        case 107:strcat(txt, "Laser Beam");break;
        case 108:strcat(txt, NOTKNOWN "108");break;
        case 109:strcat(txt, NOTKNOWN "109");break;
        case 110:strcat(txt, "Elite");break;
        case 111:strcat(txt, "Electro Brain");break;
        case 112:strcat(txt, "Infogrames");break;
        case 113:strcat(txt, "Interplay");break;
        case 114:strcat(txt, "LucasArts");break;
        case 115:strcat(txt, "Parker Brothers");break;
        case 116:strcat(txt, "Konami");break;//uCON64
        case 117:strcat(txt, "STORM");break;
        case 118:strcat(txt, NOTKNOWN "118");break;
        case 119:strcat(txt, NOTKNOWN "119");break;
        case 120:strcat(txt, "THQ Software");break;
        case 121:strcat(txt, "Accolade Inc.");break;
        case 122:strcat(txt, "Triffix Entertainment");break;
        case 123:strcat(txt, NOTKNOWN "123");break;
        case 124:strcat(txt, "Microprose");break;
        case 125:strcat(txt, NOTKNOWN "125");break;
        case 126:strcat(txt, NOTKNOWN "126");break;
        case 127:strcat(txt, "Kemco");break;
        case 128:strcat(txt, "Misawa");break;
        case 129:strcat(txt, "Teichio");break;
        case 130:strcat(txt, "Namco Ltd.");break;
        case 131:strcat(txt, "Lozc");break;
        case 132:strcat(txt, "Koei");break;
        case 133:strcat(txt, NOTKNOWN "133");break;
        case 134:strcat(txt, "Tokuma Shoten Intermedia");break;
        case 135:strcat(txt, "Tsukuda Original"); break; //Acc. ZFE
        case 136:strcat(txt, "DATAM-Polystar");break;
        case 137:strcat(txt, NOTKNOWN "137");break;
        case 138:strcat(txt, NOTKNOWN "138");break;
        case 139:strcat(txt, "Bullet-Proof Software");break;
        case 140:strcat(txt, "Vic Tokai");break;
        case 141:strcat(txt, NOTKNOWN "141");break;
        case 142:strcat(txt, "Character Soft");break;
        case 143:strcat(txt, "I\'\'Max");break;
        case 144:strcat(txt, "Takara");break;
        case 145:strcat(txt, "CHUN Soft");break;
        case 146:strcat(txt, "Video System Co., Ltd.");break;
        case 147:strcat(txt, "BEC");break;
        case 148:strcat(txt, NOTKNOWN "148");break;
        case 149:strcat(txt, "Varie");break;
        case 150:strcat(txt, "Yonezawa / S'Pal Corp."); break; //Acc. ZFE
        case 151:strcat(txt, "Kaneco");break;
        case 152:strcat(txt, NOTKNOWN "152");break;
        case 153:strcat(txt, "Pack in Video");break;
        case 154:strcat(txt, "Nichibutsu");break;
        case 155:strcat(txt, "TECMO");break;
        case 156:strcat(txt, "Imagineer Co.");break;
        case 157:strcat(txt, NOTKNOWN "157");break;
        case 158:strcat(txt, NOTKNOWN "158");break;
        case 159:strcat(txt, NOTKNOWN "159");break;
        case 160:strcat(txt, "Telenet");break;
        case 161:strcat(txt, "Hori"); break; //Acc. uCON64
        case 162:strcat(txt, NOTKNOWN "162");break;
        case 163:strcat(txt, NOTKNOWN "163");break;
        case 164:strcat(txt, "Konami");break;
        case 165:strcat(txt, "K.Amusement Leasing Co.");break;
        case 166:strcat(txt, NOTKNOWN "166");break;
        case 167:strcat(txt, "Takara");break;
        case 168:strcat(txt, NOTKNOWN "168");break;
        case 169:strcat(txt, "Technos Jap.");break;
        case 170:strcat(txt, "JVC");break;
        case 171:strcat(txt, NOTKNOWN "171");break;
        case 172:strcat(txt, "Toei Animation");break;
        case 173:strcat(txt, "Toho");break;
        case 174:strcat(txt, NOTKNOWN "174");break;
        case 175:strcat(txt, "Namco Ltd.");break;
        case 176:strcat(txt, "Media Rings Corp."); break; //Acc. ZFE
        case 177:strcat(txt, "ASCII Co. Activison");break;
        case 178:strcat(txt, "Bandai");break;
        case 179:strcat(txt, NOTKNOWN "179");break;
        case 180:strcat(txt, "Enix America");break;
        case 181:strcat(txt, NOTKNOWN "181");break;
        case 182:strcat(txt, "Halken");break;
        case 183:strcat(txt, NOTKNOWN "183");break;
        case 184:strcat(txt, NOTKNOWN "184");break;
        case 185:strcat(txt, NOTKNOWN "185");break;
        case 186:strcat(txt, "Culture Brain");break;
        case 187:strcat(txt, "Sunsoft");break;
        case 188:strcat(txt, "Toshiba EMI");break;
        case 189:strcat(txt, "Sony Imagesoft");break;
        case 190:strcat(txt, NOTKNOWN "190");break;
        case 191:strcat(txt, "Sammy");break;
        case 192:strcat(txt, "Taito");break;
        case 193:strcat(txt, NOTKNOWN "193");break;
        case 194:strcat(txt, "Kemco");break;
        case 195:strcat(txt, "Square");break;
        case 196:strcat(txt, "Tokuma Soft");break;
        case 197:strcat(txt, "Data East");break;
        case 198:strcat(txt, "Tonkin House");break;
        case 199:strcat(txt, NOTKNOWN "199");break;
        case 200:strcat(txt, "KOEI");break;
        case 201:strcat(txt, NOTKNOWN "201");break;
        case 202:strcat(txt, "Konami USA");break;
        case 203:strcat(txt, "NTVIC");break;
        case 204:strcat(txt, NOTKNOWN "204");break;
        case 205:strcat(txt, "Meldac");break;
        case 206:strcat(txt, "Pony Canyon");break;
        case 207:strcat(txt, "Sotsu Agency/Sunrise");break;
        case 208:strcat(txt, "Disco/Taito");break;
        case 209:strcat(txt, "Sofel");break;
        case 210:strcat(txt, "Quest Corp.");break;
        case 211:strcat(txt, "Sigma");break;
        case 212:strcat(txt, "Ask Kodansha Co., Ltd."); break; //Acc. ZFE
        case 213:strcat(txt, NOTKNOWN "213");break;
        case 214:strcat(txt, "Naxat");break;
        case 215:strcat(txt, NOTKNOWN "215");break;
        case 216:strcat(txt, "Capcom Co., Ltd.");break;
        case 217:strcat(txt, "Banpresto");break;
        case 218:strcat(txt, "Tomy");break;
        case 219:strcat(txt, "Acclaim");break;
        case 220:strcat(txt, NOTKNOWN "220");break;
        case 221:strcat(txt, "NCS");break;
        case 222:strcat(txt, "Human Entertainment");break;
        case 223:strcat(txt, "Altron");break;
        case 224:strcat(txt, "Jaleco");break;
        case 225:strcat(txt, NOTKNOWN "225");break;
        case 226:strcat(txt, "Yutaka");break;
        case 227:strcat(txt, NOTKNOWN "227");break;
        case 228:strcat(txt, "T&ESoft");break;
        case 229:strcat(txt, "EPOCH Co.,Ltd.");break;
        case 230:strcat(txt, NOTKNOWN "230");break;
        case 231:strcat(txt, "Athena");break;
        case 232:strcat(txt, "Asmik");break;
        case 233:strcat(txt, "Natsume");break;
        case 234:strcat(txt, "King Records");break;
        case 235:strcat(txt, "Atlus");break;
        case 236:strcat(txt, "Sony Music Entertainment");break;
        case 237:strcat(txt, NOTKNOWN "237");break;
        case 238:strcat(txt, "IGS");break;
        case 239:strcat(txt, NOTKNOWN "239");break;
        case 240:strcat(txt, NOTKNOWN "240");break;
        case 241:strcat(txt, "Motown Software");break;
        case 242:strcat(txt, "Left Field Entertainment");break;
        case 243:strcat(txt, "Beam Software");break;
        case 244:strcat(txt, "Tec Magik");break;
        case 245:strcat(txt, NOTKNOWN "245");break;
        case 246:strcat(txt, NOTKNOWN "246");break;
        case 247:strcat(txt, NOTKNOWN "247");break;
        case 248:strcat(txt, NOTKNOWN "248");break;
        case 249:strcat(txt, "Cybersoft");break;
        case 250:strcat(txt, NOTKNOWN "250");break;
        case 251:strcat(txt, "Psygnosis"); break; //Acc. ZFE
        case 252:strcat(txt, NOTKNOWN "252");break;
        case 253:strcat(txt, NOTKNOWN "253");break;
        case 254:strcat(txt, "Davidson"); break; //Acc. uCON64
        case 255:strcat(txt, NOTKNOWN "255");break;
        default:strcat(txt, NOTKNOWN);break;
    }

    strcat(txt, "\nROM Version: ");
    sprintf(temp, "1.%d", (Memory.HiROM)?Memory.ROM[0x0FFDB]:Memory.ROM[0x7FDB]);
    strcat(txt, temp);
    strcat(txt, "\nRegion: ");
    switch(Memory.ROMRegion)
    {
        case 0:
            strcat(txt, "Japan");
            break;
        case 1:
            strcat(txt, "USA/Canada");
            break;
        case 2:
            strcat(txt, "Oceania, Europe, and Asia");
            break;
        case 3:
            strcat(txt, "Sweden");
            break;
        case 4:
            strcat(txt, "Finland");
            break;
        case 5:
            strcat(txt, "Denmark");
            break;
        case 6:
            strcat(txt, "France");
            break;
        case 7:
            strcat(txt, "Holland");
            break;
        case 8:
            strcat(txt, "Spain");
            break;
        case 9:
            strcat(txt, "Germany, Austria, and Switzerland");
            break;
        case 10:
            strcat(txt, "Italy");
            break;
        case 11:
            strcat(txt, "Hong Kong and China");
            break;
        case 12:
            strcat(txt, "Indonesia");
            break;
        case 13:
            strcat(txt, "South Korea");
            break;
        case 14:strcat(txt, "Unknown region 14");break;
        default:strcat(txt, "Unknown region 15");break;
    }
};
void printAbout(char *txt)
{
    sprintf(txt,"Version: %s\nSnes9x Version: %s\nDate: %s", REVISION, VERSION,DATE);
    strcat(txt, "\n\nSnes9x for 3DS is a high-compatibility SNES emulator for your Old 3DS / 2DS. It runs many games at full speed (60 fps).\n");
    strcat(txt, "\n\nHome: https://github.com/bubble2k16/snes9x_3ds\n");
    strcat(txt, "Author: bubble2k16");
};