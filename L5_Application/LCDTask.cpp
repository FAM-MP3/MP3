///*
// * LCDTask.cpp
// *
// *  Created on: Dec 5, 2018
// *      Author: Maytinee
// */
#include <LCDTask.hpp>
#include <stdio.h>
#include "LabUART.hpp"
#include "utilities.h"

LabUART uart;
bool LCDTask :: LCDClearDisplay()
{
    uart.Transmit(0xFE);
    uart.Transmit(0x01); //0x01
    delay_ms(100);
    return true;
}

bool LCDTask :: LCDTurnDisplayOn()
{
    uart.Transmit(0xFE);
    uart.Transmit(0x0C);
    delay_ms(100);
    return true;
}

bool LCDTask :: LCDSetBaudRate()
{
    uart.Transmit(0x7C);
    uart.Transmit(0x0D);

    return true;
}

bool LCDTask :: LCDBackLightON()
{
    uart.Transmit(0x7C);
    uart.Transmit(0x9D);
    delay_ms(100);
    return true;
}

bool LCDTask :: LCD4Lines()
{
    uart.Transmit(0x7C);
    uart.Transmit(0x05);
    delay_ms(100);
    return true;
}

bool LCDTask :: LCD20char()
{
    uart.Transmit(0x7C);
    uart.Transmit(0x03);
    delay_ms(100);
    return true;
}

bool LCDTask :: LCDCursorPosition()
{
    uart.Transmit(0xFE);
    uart.Transmit(0x9); //Line 1
    delay_ms(100);

    uart.Transmit(0xFE);
    uart.Transmit(0x73); //Line 2
    delay_ms(100);

    uart.Transmit(0xFE);
    uart.Transmit(0x39); //Line 3
    delay_ms(100);

    uart.Transmit(0xFE);
    uart.Transmit(0x103); //Line 4
    delay_ms(100);
    return true;
}

bool LCDTask :: LCDSet_Cursor(int line, int position)
{
    uart.Transmit(0xFE);

    switch(line)
    {
        case 1: uart.Transmit(0x80 + position); //Line 1
            break;
        case 2: uart.Transmit(0xC0 + position); //Line 2
            break;
        case 3: uart.Transmit(0x94 + position); //Line 3
            break;
        default: uart.Transmit(0xD4 + position); //Line 4
            break;
    }

    return true;
}


bool LCDTask :: LCDBlinkCursor()
{
    uart.Transmit(0xFE);
    uart.Transmit(0x0D);
    delay_ms(100);
    return true;
}

bool LCDTask :: LCDLine1()
{
    uart.Transmit(0xFE);
       uart.Transmit(0x80);
       delay_ms(100);
    return true;
}

bool LCDTask :: LCDLine2()
{
    uart.Transmit(0xFE);
    uart.Transmit(0xC0);
    delay_ms(100);
    return true;
}


bool LCDTask :: LCDLine3()
{
    uart.Transmit(0xFE);
     uart.Transmit(0x94);
     delay_ms(100);
     return true;
}

bool LCDTask :: LCDLine4()
{
    uart.Transmit(0xFE);
     uart.Transmit(0xD4);
     delay_ms(100);
     return true;
}

bool LCDTask :: LCDScrollRight()
{
    uart.Transmit(0xFE);
    uart.Transmit(0x1C);
    delay_ms(100);
    return true;
}
