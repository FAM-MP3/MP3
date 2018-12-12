/*
 * InterruptLab.cpp
 *
 *  Created on: Sep 14, 2018
 *      Author: Maytinee
 */

#include "InterruptLab.h"
#include "LPC17xx.h"
#include <stdint.h>
#include <stdio.h>
#include "uart0_min.h"
#include "eint.h"
#include "lpc_isr.h"


LabGpioInterrupts :: LabGpioInterrupts()
{
    // Set all locations in the 2d lookup table (port/ pins) to NULL
     for(int i=0; i<2; i++)
     {
         for(int j=0; j<32; j++)
             pin_isr_map[i][j] = NULL;
     }
}


void LabGpioInterrupts :: Initialize()
{
    //configure NVIC to notice EINT3 IRQs; use NVIC_EnableIRQ()
    NVIC_EnableIRQ(EINT3_IRQn);
    //NVIC recognizes eint3 irqs
}

bool LabGpioInterrupts :: AttachInterruptHandler(uint8_t port, uint32_t pin, IsrPointer pin_isr, InterruptCondition condition)
{

    // check if ports are valid + if pin_isr is valid?
        if((port != 0 && port != 2) || (pin_isr == NULL))

            return false;


    //Check if pins are valid?
        if(port == 0) //0-12, 14-30
        {
            if((pin >= 12) && (pin <= 14) || (pin > 30))
            {
                //uart0_puts("PORT0");
                return false;
            }
        }

        if(port == 2) //0-13
        {
            port = 1; //mapping this to pin_isr_map[1][pin] since 2d

            if(pin >= 13)
            {
                //uart0_puts("PORT2");
                return false;
            }
        }


    //enable the interrupt conditions
    switch (condition)
    {
        case kRisingEdge:
            if(port == 0)
            {
                LPC_GPIOINT -> IO0IntEnR |= (1 << pin);
                //uart0_puts("Port 0 detects rising edge interrupt at this pin.");
            }
            else
                LPC_GPIOINT -> IO2IntEnR |= (1 << pin);
               // uart0_puts("Port 2 detects rising edge interrupt at this pin.");
           break;

        case kFallingEdge:
            if(port == 0)
            {
                LPC_GPIOINT -> IO0IntEnF |= (1 << pin);
                //uart0_puts("Port 0 detects falling edge interrupt at this pin.");
            }
            else
                LPC_GPIOINT -> IO2IntEnF |= (1 << pin);
               // uart0_puts("Port 2 detects falling edge interrupt at this pin.");
            break;

        case kBothEdges:
            if(port == 0)
                  {
                      LPC_GPIOINT -> IO0IntEnR |= (1 << pin);
                      LPC_GPIOINT -> IO0IntEnF |= (1 << pin);
                     // uart0_puts("Port 0 detects rising/falling edge interrupt at this pin.");
                  }
                  else
                      LPC_GPIOINT -> IO2IntEnR |= (1 << pin);
                      LPC_GPIOINT -> IO2IntEnF |= (1 << pin);
                      //uart0_puts("Port 2 detects rising/falling edge interrupt at this pin.");
                  break;

        default: return false;
    }


    //setting up the lookup table for pin/port combo to a function so that it can get called when that interrupt happens
        //need to set pin_isr_map (lookup table) to pin_isr= null

    if(pin_isr_map[port][pin] != NULL)
          {
            uart0_puts("Checking if isr_map has a function in the location.");
            return false; //checking if isr_map already had a function in the location

          }
          else
              pin_isr_map[port][pin] = pin_isr;


        //pin_isr doesnt always have to equal to null --> vikas

}

void LabGpioInterrupts :: HandleInterrupt()
{

       //indicating whether there is an interrupt
    uint8_t port;
    uint32_t pin;

    //checking the port status, EX. IO0IntStatF

 if((LPC_GPIOINT -> IntStatus & (1 << 0))) //one interrupt pending on Port 0
 {
    for(pin=0; pin<32; pin++)
    {
        if((LPC_GPIOINT -> IO0IntStatF & (1 << pin)))
        {
            uart0_puts("Interrupt falling edge detected port 0."); //indicating that there is a falling edge interrupt at Port 0
            pin_isr_map[0][pin]();
            LPC_GPIOINT -> IO0IntClr |= (1 << pin);
        }

        else if((LPC_GPIOINT -> IO0IntStatR & (1 << pin)))
        {
            uart0_puts("Interrupt rising edge detected port 0."); //indicating that there is a rising edge interrupt at Port 0
            pin_isr_map[0][pin]();
            LPC_GPIOINT -> IO0IntClr |= (1 << pin);
        }
    }
 }

 //port2
 else if((LPC_GPIOINT -> IntStatus & (1 << 2))) //one interrupt pending on Port 2
 {
     for(pin=0; pin<32; pin++)
         {
           if((LPC_GPIOINT -> IO2IntStatF & (1 << pin)))
           {
             uart0_puts("Interrupt falling edge detected port 2.");
             pin_isr_map[1][pin]();
             LPC_GPIOINT -> IO2IntClr |= (1 << pin);
           }

           else if((LPC_GPIOINT -> IO2IntStatR & (1 << pin)))
           {
             uart0_puts("Interrupt rising edge detected port 2.");
             pin_isr_map[1][pin]();
             LPC_GPIOINT -> IO2IntClr |= (1 << pin);
           }
         }
 }

}

