/*
 * VS1053.h
 *
 *  Created on: Dec 5, 2018
 *      Author: kimfl
 */

#ifndef VS1053_H_
#define VS1053_H_

#include <stdio.h>
#include "gpio.hpp"
#include "storage.hpp"
#include "ssp1.h"
#include "vs10xx_uc.h"
#include "task.h"


class VS1053 {
    public:
        void init(LPC1758_GPIO_Type dreq, LPC1758_GPIO_Type xcs, LPC1758_GPIO_Type xdcs);
        void WriteSpiByte(uint8_t data);
        uint8_t ReadSpiByte(void);
        void WriteSci(uint8_t addr, uint16_t data);
        uint16_t ReadSci(uint8_t addr);
        void WriteSdi(uint8_t *buffer, uint16_t byte_size);
        void soft_reset(void);
        void sineTest(uint8_t n, uint16_t ms);
        void setVolume(uint8_t left, uint8_t right);
        virtual ~VS1053();
        VS1053();
        GPIO* DREQ;
        GPIO* xCS;
        GPIO* xDCS;
        FIL track;
        uint8_t dataBuffer[512];
};



#endif /* VS1053_H_ */
