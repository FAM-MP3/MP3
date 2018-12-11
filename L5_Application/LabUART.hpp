/*
 * LabUART.hpp
 *
 *  Created on: Oct 9, 2018
 *      Author: kimfl
 */

#ifndef LABUART_HPP_
#define LABUART_HPP_

#include <stdio.h>
#include "FreeRTOS.h"
#include "LPC17xx.h"

class LabUART
{
 public:
    enum UART_CASE
    {
        Uart2,      // default is Uart2
        Uart3,
    };

    LabUART();
    ~LabUART() { } // nothing to do in deconstructor

    // TODO: Fill in methods for Initialize(), Transmit(), Receive() etc.
    void Initialize(UART_CASE uart, uint16_t baudRate);
    char Receive();
    void Transmit(char data);

    // Optional: For the adventurous types, you may inherit from "CharDev" class
    // to get a lot of functionality for free
 private:
    UART_CASE uart_case;
};



#endif /* LABUART_HPP_ */
