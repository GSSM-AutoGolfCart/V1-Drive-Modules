/**
 * @file module.h
 * 
 * @author Joseph Telaak
 * 
 * @brief Module outline and standard utils
 * 
 * @version 0.1
 * 
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// --------- Standard Module Components
#include "can_adapter.h"
#include "net.h"

#ifdef HAS_LIGHT
    #include "id_light.h"
#endif

void standardModuleSetup(int CAN_CS_PIN = Default_CAN_CS);
void standardModuleLoopHead();
void standardModuleLoopTail();
void ready();
void holdTillEnabled();