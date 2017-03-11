/****************************************************************************
 * Copyright (C) 2016 Maschell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include <string.h>

#include "coreinit_function_patcher.h"
#include "dynamic_libs/os_functions.h"
#include "utils/logger.h"

/*
 * ENUM_BEG(SCILanguage, uint32_t)
   ENUM_VALUE(Japanese,             0x00)
   ENUM_VALUE(English,              0x01)
   ENUM_VALUE(French,               0x02)
   ENUM_VALUE(German,               0x03)
   ENUM_VALUE(Italian,              0x04)
   ENUM_VALUE(Spanish,              0x05)
   ENUM_VALUE(Chinese,              0x06)
   ENUM_VALUE(Korean,               0x07)
   ENUM_VALUE(Dutch,                0x08)
   ENUM_VALUE(Portugese,            0x09)
   ENUM_VALUE(Russian,              0x0A)
   ENUM_VALUE(Taiwanese,            0x0B)
   ENUM_VALUE(Max,                  0x0C)
ENUM_END(SCILanguage)

ENUM_BEG(SCIRegion, uint8_t)
   ENUM_VALUE(JAP,                  0x01)
   ENUM_VALUE(USA,                  0x02)
   ENUM_VALUE(EUR,                  0x04)
ENUM_END(SCIRegion)
*/

u32 language=0x01;
u8 region = 0x04;
static bool enable=true;

void SetEnableCoreInitHooks(bool e){
    enable=e;
}

void SetLanguage(u32 lang)
{
    language = lang;
}

void SetRegion(u8 reg)
{
    region=reg;
}

DECL(void, _Exit, void){
    log_print("_Exit: Disabling coreinit hooks...\n");
    enable=false;
    real__Exit();
}

DECL(int, UCReadSysConfig, int IOHandle, int count, struct UCSysConfig* settings){
    log_print("UCReadSysConfig\n");
    
    int result = real_UCReadSysConfig(IOHandle,count,settings);
        
    if(enable)
    {
        if(result!=0)
            return result;

        if(strcmp(settings->name,"cafe.language")==0)
        {
            log_print("UCReadSysConfig: cafe.language found!\n");
            log_print("UCReadSysConfig: forcing language...\n");
            log_printf("UCReadSysConfig: original lang %d, new %d\n",*((int*)settings->data),language);
            *((int*)settings->data)=language;
            
        }
    }
    
    return result;
}

DECL(int, MCP_GetSysProdSettings, int IOHandle, struct MCPSysProdSettings* settings){
    log_print("MCP_GetSysProdSettings\n");
    
    int result = real_MCP_GetSysProdSettings(IOHandle,settings);
    
        
    if(enable)
    {
        if(result!=0)
            return result;
                
        log_print("MCP_GetSysProdSettings: forcing platform region...\n");
        log_printf("MCP_GetSysProdSettings: original region %d, new %d...\n", settings->platformRegion,region);
        settings->platformRegion = region;
    }
    
    return result;
}

hooks_magic_t method_hooks_coreinit[] __attribute__((section(".data"))) = {
    MAKE_MAGIC(_Exit,                               LIB_CORE_INIT,  STATIC_FUNCTION),
    MAKE_MAGIC(UCReadSysConfig,                     LIB_CORE_INIT,  STATIC_FUNCTION),
    MAKE_MAGIC(MCP_GetSysProdSettings,                     LIB_CORE_INIT,  STATIC_FUNCTION),
};

u32 method_hooks_size_coreinit __attribute__((section(".data"))) = sizeof(method_hooks_coreinit) / sizeof(hooks_magic_t);

//! buffer to store our instructions needed for our replacements
volatile unsigned int method_calls_coreinit[sizeof(method_hooks_coreinit) / sizeof(hooks_magic_t) * FUNCTION_PATCHER_METHOD_STORE_SIZE] __attribute__((section(".data")));

