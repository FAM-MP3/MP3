/*
 * LabUART.cpp
 *
 *  Created on: Oct 9, 2018
 *      Author: kimfl
 */

#include "LabUART.hpp"

LabUART::LabUART()
{
    uart_case = Uart2;          // default UART is UART2
}

void LabUART::Initialize(UART_CASE uart, uint16_t baudRate)
{
    /*       baud rate formula           */
    uint16_t UD = sys_get_cpu_clock() / (16*baudRate);

    switch(uart)
    {
        case Uart2: {
                        uart_case = uart;
                        LPC_SC->PCONP |= (1 << 24);                 // power on
                        LPC_SC->PCLKSEL1 &= ~(3 << 16);             // clear UART2 clk
                        LPC_SC->PCLKSEL1 |= (1 << 16);              // select UART2 clk

                        LPC_PINCON->PINSEL4 &= ~(0xF << 16);        // clear P2.8 and P2.9
                        LPC_PINCON->PINSEL4 |= (0xA << 16);         // select TXD2 for P2.8 and RXD2 for P2.9

                        /*       Set DLL and DMM             */
                        LPC_UART2->LCR |= (1 << 7);                 // enable DLAB
                        LPC_UART2->DLL = (UD >> 0) & 0xFF;          // latch LSB
                        LPC_UART2->DLM = (UD >> 8) & 0xFF;          // latch MSB
                        LPC_UART2->LCR &= ~(1 << 7);                // disable DLAB

                        LPC_UART2->LCR |= (3 << 0);                 // 8-bit char length
                        LPC_UART2->LCR &= ~(1 << 2);                // set 1 stop bit
                        LPC_UART2->LCR &= ~(1 << 3);                // disable parity

                        LPC_UART2->FCR |= (1 << 0);                 // enable FIFO

                        LPC_UART2->FCR |= (1 << 1) | (1 << 2);      // clear FIFO
                        LPC_UART2->IER = (1 << 0) | (1 << 2);       // enable status interrupts
                        NVIC_EnableIRQ(UART2_IRQn);
                        break;
                    }
        case Uart3:{
                        uart_case = uart;
                        LPC_SC->PCONP |= (1 << 25);                 // power on
                        LPC_SC->PCLKSEL1 &= ~(3 << 18);             // clear UART3 clk
                        LPC_SC->PCLKSEL1 |= (1 << 18);              // select UART3 clk

                        LPC_PINCON->PINSEL9 &= ~(0xF << 24);        // clear P4.28 and P4.29
                        LPC_PINCON->PINSEL9 |= (0xF << 24);         // select TXD3 for P4.28 and RXD3 for P4.29

                        /*       Set DLL and DMM             */
                        LPC_UART3->LCR |= (1 << 7);                 // enable DLAB
                        LPC_UART3->DLL &= ~(0xFF << 0);          // latch LSB
                        LPC_UART3->DLL = (UD >> 0) & 0xFF;          // latch LSB
                        LPC_UART3->DLM &= ~(0xFF << 0);
                        LPC_UART3->DLM = (UD >> 8) & 0xFF;          // latch MSB
                        LPC_UART3->LCR &= ~(1 << 7);                // disable DLAB

                        LPC_UART3->LCR |= (3 << 0);                 // 8-bit char length
                        LPC_UART3->LCR &= ~(1 << 2);                // set 1 stop bit
                        LPC_UART3->LCR &= ~(1 << 3);                // disable parity

                        LPC_UART3->FCR |= (1 << 0);                 // enable FIFO

                        LPC_UART3->FCR |= (1 << 1) | (1 << 2);      // clear FIFO
                        LPC_UART3->IER = (1 << 0) | (1 << 2);       // enable status interrupts
                        NVIC_EnableIRQ(UART3_IRQn);
                        break;
                   }
        default: printf("ERROR: Incorrect UART.\n");
                 break;
    }
//    Transmit('A');

}

char LabUART::Receive()
{
    switch(uart_case)
    {
        case Uart2: return LPC_UART2->RBR;
            break;
        case Uart3: return LPC_UART3->RBR;
            break;
        default: return -1;
            break;
    }
}

void LabUART::Transmit(char data)
{
    switch(uart_case)
    {
        case Uart2: LPC_UART2->THR = data;
            break;
        case Uart3: LPC_UART3->THR = data;
            break;
        default:
            break;
    }
}

