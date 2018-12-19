/*
 * def.hpp
 *
 *  Created on: Nov 16, 2018
 *      Author: kimfl
 */

#ifndef DEF_HPP_
#define DEF_HPP_

//#include "gpio.hpp"
#include "LabGPIO.hpp"
#include "LabSPI0.h"
#include "LCDTask.hpp"
#include "LabUART.hpp"

/* GPIO pins */
//GPIO GPIO1(P0_26);              // reset
//GPIO GPIO2(P1_31);              // DREQ
//GPIO GPIO3(P1_30);              // xCS
//GPIO GPIO4(P1_29);              // xDCS
LabGPIO GPIO1(0,26);              // reset
LabGPIO GPIO2(1,31);              // DREQ
LabGPIO GPIO3(1,30);              // xCS
LabGPIO GPIO4(1,29);              // xDCS

LabSpi0 SPI0;

//LabUART uart1;

//LabGPIO settingsButton(0,30);            // for scrolling through settings pages
//LabGPIO playPauseButton(0,29);           // for pause/play song
//LabGPIO upButton(0,1);                  // for scrolling up
//LabGPIO downButton(0,0);                  // for scrolling down

#endif /* DEF_HPP_ */
