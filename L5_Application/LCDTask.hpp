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

class LCDTask
{
    public:
//        bool LCDinit(void);
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
        bool LCDSet_Cursor(int line, int position);

    private:

};

#endif /* LCDTASK_HPP_ */
