/*
 * LabGPIO_Interrupts.cpp
 *
 *  Created on: Sep 14, 2018
 *      Author: kimfl
 */
#include "LabGPIO_Interrupts.hpp"
#include "LPC17xx.h"
#include <stdio.h>
#include <algorithm>
using namespace std;

LabGPIO_Interrupts::LabGPIO_Interrupts()
{
    // Set all locations to NULL
    for(int i=0; i<2; i++)
    {
        for(int j=0; j<32; j++)
            pin_isr_map[i][j] = NULL;
    }
}

void LabGPIO_Interrupts::Initialize()
{
    NVIC_EnableIRQ(EINT3_IRQn);
}

bool LabGPIO_Interrupts::AttachInterruptHandler(uint8_t port, uint32_t pin, IsrPointer pin_isr, InterruptCondition condition)
{
    // VALID Ports? VALID pin_isr?
    if((port != 0 && port != 2) || (pin_isr == NULL))
        return false;

    // VALID pins?
    if(port == 0) // port 0 has pins 0-12, 14-30
    {
        if((pin >= 12 && pin <= 14) || (pin > 30))
            return false;
    }
    if (port == 2) // port 2 has pins 0-13
    {
        port = 1;    // If Port == 2, fix it so that it saves to map[1][pin];

        if (pin > 13)
            return false;
    }

    // Set condition
    switch (condition)
    {
        case kRisingEdge: if (port == 0)
                            LPC_GPIOINT->IO0IntEnR |= (1 << pin);
                          else
                            LPC_GPIOINT->IO2IntEnR |= (1 << pin);
                          break;
        case kFallingEdge: if (port == 0)
                             LPC_GPIOINT->IO0IntEnF |= (1 << pin);
                           else
                             LPC_GPIOINT->IO2IntEnF |= (1 << pin);
                           break;
        case kBothEdges: if (port == 0)
                         {
                             LPC_GPIOINT->IO0IntEnR |= (1 << pin);
                             LPC_GPIOINT->IO0IntEnF |= (1 << pin);
                         }
                         else
                         {
                             LPC_GPIOINT->IO2IntEnR |= (1 << pin);
                             LPC_GPIOINT->IO2IntEnF |= (1 << pin);
                         }
                         break;
        default: return false;
    }

    // Check if isr_map already has a function in the location
    if(pin_isr_map[port][pin] != NULL)
        return false;
    else
        pin_isr_map[port][pin] = pin_isr;

    return true;
}

void LabGPIO_Interrupts::HandleInterrupt()
{
    // Check status
    uint8_t status= LPC_GPIOINT->IntStatus;

    switch (status)
    {
        case 1: { // Port 0 has interrupt(s) pending
                    uint32_t pin0R = LPC_GPIOINT->IO0IntStatR;
                    uint32_t pin0F = LPC_GPIOINT->IO0IntStatF;

                    // Loop until end of pin0R or pin0F (whichever is larger)
                    for(uint32_t i=0; i<max(pin0R, pin0F); i++)
                    {
                        // If the current bit is set, run the callback function
                        if ((pin0R & (1 << i)) || (pin0F & (1 << i)))
                        {
                            // Clear
                            LPC_GPIOINT->IO0IntClr |= (1 << i);
                            pin_isr_map[0][i]();
                        }
//                            interrupt0.push_back(i);
                    }
                }
            break;
        case 4: { // Port 2 has interrupt(s) pending
                    uint16_t pin2R = LPC_GPIOINT->IO2IntStatR;
                    uint16_t pin2F = LPC_GPIOINT->IO2IntStatF;

                    // Loop until end of pin2R or pin2F (whichever is larger)
                    for(uint32_t i=0; i<max(pin2R, pin2F); i++)
                    {
                        // If the current bit is set, run the callback function
                        if ((pin2R & (1 << i)) || (pin2F & (1 << i)))
                        {
                            // Clear
                            LPC_GPIOINT->IO2IntClr |= (1 << i);
                            pin_isr_map[1][i]();
                        }
                    }
                }
            break;
        case 5: { // Both ports have interrupts
                    uint32_t pin0R = LPC_GPIOINT->IO0IntStatR;
                    uint32_t pin0F = LPC_GPIOINT->IO0IntStatF;
                    uint16_t pin2R = LPC_GPIOINT->IO2IntStatR;
                    uint16_t pin2F = LPC_GPIOINT->IO2IntStatF;

                    // Loop until end of pin0R or pin0F (whichever is larger)
                    for(uint32_t i=0; i<max(pin0R, pin0F); i++)
                    {
                        // If the current bit is set, run the callback function
                        if ((pin0R & (1 << i)) || (pin0F & (1 << i)))
                        {
                            // Clear
                            LPC_GPIOINT->IO0IntClr |= (1 << i);
                            pin_isr_map[0][i]();
                        }
                    }

                    // Loop until end of pin2R or pin2F (whichever is larger)
                    for(uint32_t i=0; i<max(pin2R, pin2F); i++)
                    {
                        // If the current bit is set, run the callback function
                        if ((pin2R & (1 << i)) || (pin2F & (1 << i)))
                        {
                            // Clear
                            LPC_GPIOINT->IO2IntClr |= (1 << i);
                            pin_isr_map[1][i]();
                        }
                    }
                }
            break;
        default:
            break;
    }
}



