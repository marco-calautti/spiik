#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "main.h"
#include "common/common.h"
#include "system/memory.h"

#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/gx2_functions.h"
#include "dynamic_libs/syshid_functions.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "patcher/coreinit_function_patcher.h"
#include "utils/function_patcher.h"
#include "kernel/kernel_functions.h"
#include "utils/logger.h"

#define PRINT_TEXT2(x, y, ...) { char msg[80]; snprintf(msg, 80, __VA_ARGS__); OSScreenPutFontEx(0, x, y, msg); OSScreenPutFontEx(1, x, y, msg); }

bool launched=false;

/* Entry point */
extern "C" int Menu_Main(void)
{
    Init();

    u64 currentId = OSGetTitleID();
    u64 menuId = _SYSGetSystemApplicationTitleId(0);
    
    log_printf("Launched: %d, Id: %d\n",launched, (u32)(currentId&0xFFFFFFFF));
    
    //Reset everything when were going back to the Mii Maker or the HBL Channel
    if( (launched && (currentId == 0x0005000013374842ULL)) || 
        (strlen(cosAppXmlInfoStruct.rpx_name) > 0 && strcasecmp("ffl_app.rpx", cosAppXmlInfoStruct.rpx_name) == 0)){
        log_print("Disposing everything!\n");
        deInitFull();
        return EXIT_SUCCESS;
    }
    
    if(currentId == menuId) //skip changing language and most importantly, region to system menu.
    {
        deInit();
        return EXIT_RELAUNCH_ON_LOAD;
    }
    

    //First run, ask the user some settings before coming back to System menu
    if(strlen(cosAppXmlInfoStruct.rpx_name) <= 0){
        
        if(SelectionMenu())
        {
            log_print("Nothing to do, exiting...\n");
            deInitFull();
            return EXIT_SUCCESS;
        }
        launched=true;
        SYSLaunchMenu();
        return EXIT_RELAUNCH_ON_LOAD;
    }

    //!*******************************************************************
    //!                        Patching functions                        *
    //!*******************************************************************
    SetEnableCoreInitHooks(true);
    log_print("Patching functions\n");
    ApplyPatches();
    
    
    return EXIT_RELAUNCH_ON_LOAD;
}

/*
    Patching all the functions!!!
*/
void ApplyPatches(){
    PatchInvidualMethodHooks(method_hooks_coreinit,     method_hooks_size_coreinit,     method_calls_coreinit);
}

void Init()
{
    InitOSFunctionPointers();
    InitSocketFunctionPointers();

    InitSysFunctionPointers();

    InitVPadFunctionPointers();
    InitPadScoreFunctionPointers();

    SetupKernelCallback();

    log_init("192.168.0.21");
}

/*
    Restoring hooks but keep logging
*/
void deInit(){
    RestoreInvidualInstructions(method_hooks_coreinit,  method_hooks_size_coreinit);
}

void deInitFull(){
    deInit();
    log_deinit();
}

int SelectionMenu(){
        
    int langId=1;
    const char* languages[]={ 
        "Japanese",
        "English",
        "French",            
        "German",              
        "Italian",              
        "Spanish",             
        "Chinese",              
        "Korean",               
        "Dutch",                
        "Portugese",            
        "Russian",             
        "Taiwanese"};
        
    int regionId=2;
    const char* regions[]={
        "JAP", "USA", "EUR"
    };
    
    int selectionId=0;
    bool update=true;
    
    VPADData vpad_data;
    int vpadError = -1;
    
    VPADInit();
    memoryInitialize();
    
    // Prepare screen
    int screen_buf0_size = 0;
    int screen_buf1_size = 0;

    // Init screen and screen buffers
    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(0);
    screen_buf1_size = OSScreenGetBufferSizeEx(1);

    unsigned char *screenBuffer = (unsigned char*)MEM1_alloc(screen_buf0_size + screen_buf1_size, 0x40);

    OSScreenSetBufferEx(0, screenBuffer);
    OSScreenSetBufferEx(1, (screenBuffer + screen_buf0_size));

    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);

    
    while(1)
    {
        VPADRead(0, &vpad_data, 1, &vpadError);

        if(update)
        {
            OSScreenClearBufferEx(0, 0);
            OSScreenClearBufferEx(1, 0);
            
            PRINT_TEXT2(0,0,"Spiik: Region and Language enforcer v0.1b (Phoenix)");
            PRINT_TEXT2(0,2,"Select with DPAD and confirm with A. Press HOME to exit.");
            //PRINT_TEXT2("\n");
                
            if(selectionId==0)
            {
                PRINT_TEXT2(0,4,"-> Language: %s\n",languages[langId]);
                PRINT_TEXT2(0,5,"   Region: %s\n",regions[regionId]);
            }else
            {
                PRINT_TEXT2(0,4,"   Language: %s\n",languages[langId]);
                PRINT_TEXT2(0,5,"-> Region: %s\n",regions[regionId]);
            }
            update=false;
            OSScreenFlipBuffersEx(0);
            OSScreenFlipBuffersEx(1);
        }
        
        u32 pressedBtns = vpad_data.btns_d | vpad_data.btns_h;
        
        if (pressedBtns & VPAD_BUTTON_HOME){
            MEM1_free(screenBuffer);
            screenBuffer = NULL;
            memoryRelease();
            return -1;
        } 
        else if (pressedBtns & VPAD_BUTTON_A) break;
        else if (pressedBtns & VPAD_BUTTON_DOWN){
            selectionId = selectionId==0? 1 : 0;
            update=true;
        }else if (pressedBtns & VPAD_BUTTON_UP){
            selectionId = selectionId==0? 1 : 0;
            update=true;
        }else if (pressedBtns & VPAD_BUTTON_RIGHT){
            if(selectionId==0)
                langId=(langId+1)%12;
            else
                regionId = (regionId+1)%3;
            update=true;
        }else if (pressedBtns & VPAD_BUTTON_LEFT){
            if(selectionId==0)
                langId= langId==0? 11 : (langId-1)%12;
            else
                regionId = regionId==0? 2 : (regionId-1)%3;
            update=true;
        }
        usleep(80000);
    }
    
    SetLanguage(langId);
    SetRegion(1<<regionId);
        
    MEM1_free(screenBuffer);
    screenBuffer = NULL;
    memoryRelease();
    return 0;
}
