#ifndef _MAIN_H_
#define _MAIN_H_

#include "common/types.h"
#include "dynamic_libs/os_functions.h"

/* Main */
#ifdef __cplusplus
extern "C" {
#endif

//! C wrapper for our C++ functions
int Menu_Main(void);
int SelectionMenu(void);
void Init(void);
void deInit(void);
void deInitFull(void);
void ApplyPatches(void);

#ifdef __cplusplus
}
#endif

#endif
