/*
 * LCDTask.hpp
 *
 *  Created on: Dec 5, 2018
 *      Author: Maytinee
 */

#ifndef LCDTASK_HPP_
#define LCDTASK_HPP_

#include "scheduler_task.hpp"   //shared objects
#include "queue.h"              //queues to send filename of song to play
#include "semphr.h"             //semaphores to signal reader task from playCmd handler
#include "uart0_min.h"          //UART0 output in Hercules
#include <stdio.h>              //print f
#include "ff.h"                 //file system
#include "LabUART.hpp"

class LCDTask
{
    public:
//        bool LCDinit(void);
        LCDTask();
//        void setUART(LabUART passed_uart);
        bool LCDClearDisplay();
        bool LCDTurnDisplayOn();
        bool LCDSetBaudRate();
        bool LCDBackLightON();
        bool LCD4Lines();
        bool LCD20char();
        bool LCDCursorPosition();
        bool LCDBlinkCursor();
        bool LCDLine1();
        bool LCDLine2();
        bool LCDLine3();
        bool LCDLine4();
        bool LCDScrollRight();
        bool LCDSetCursor(int line, int position);
        bool LCDSetSplashScreen();

    private:
        LabUART uart;

};

#endif /* LCDTASK_HPP_ */
