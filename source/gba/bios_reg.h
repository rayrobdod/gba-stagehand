#ifndef BIOS_REG_H
#define BIOS_REG_H

#include "gba/shared.h"

extern void (* volatile REG_ISR_MAIN)(void);

extern volatile interrupt_flag_t REG_IFBIOS;

#endif        //  #ifndef BIOS_REG_H
