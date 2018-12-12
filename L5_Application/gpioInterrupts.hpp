#include <stdint.h> //uint#_t variables for course coding standards
#include "FreeRTOS.h" //FreeRTOS Task Scheduler
#include "LPC17xx.h" //parameters for pins, example EINT3_IRQn
#include "task.h" //task functions
#include "uart0_min.h" //UART0 output in Hercules
#include <printf_lib.h>//For ISR debug output u0_dbg_printf("output");
//#include "semphr.h" //semaphore functions

//Global Variables
const uint8_t testPin = 0, testPort=0, EINT_3 = 21;
uint32_t IO0IntStatF, IO0IntStatR, IO2IntStatF, IO2IntStatR, count;

//Begin Defining Interrupt Class
enum InterruptCondition
{
    kRisingEdge,
    kFallingEdge,
    kBothEdges,
};

/**
 * Typdef a function pointer which will help in code readability
 * For example, with a function foo(), you can do this:
 * IsrPointer function_ptr = foo;
 * OR
 * IsrPointer function_ptr = &foo;
 */
typedef void (*IsrPointer)(void);

class LabGpioInterrupts
{
 public:
    /**
     * Optional: LabGpioInterrupts could be a singleton class, meaning, only one instance can exist at a time.
     * Look up how to implement this. It is best to not allocate memory in the constructor and leave complex
     * code to the Initialize() that you call in your main()
     */
    LabGpioInterrupts();

    /**
     * This should configure NVIC to notice EINT3 IRQs; use NVIC_EnableIRQ()
     */
    void Initialize();

    /**
     * This handler should place a function pointer within the lookup table for the HandleInterrupt() to find.
     *
     * @param[in] port         specify the GPIO port, and 1st dimension of the lookup matrix
     * @param[in] pin          specify the GPIO pin to assign an ISR to, and 2nd dimension of the lookup matrix
     * @param[in] pin_isr      function to run when the interrupt event occurs
     * @param[in] condition    condition for the interrupt to occur on. RISING, FALLING or BOTH edges.
     * @return should return true if valid ports, pins, isrs were supplied and pin isr insertion was successful
     */
    bool AttachInterruptHandler(uint8_t port, uint32_t pin, IsrPointer pin_isr, InterruptCondition condition);

    /**
     * This function is invoked by the CPU (through Eint3Handler) asynchronously when a Port/Pin
     * interrupt occurs. This function is where you will check the Port status, such as IO0IntStatF,
     * and then invoke the user's registered callback and find the entry in your lookup table.
     *
     * VERY IMPORTANT!
     *  - Be sure to clear the interrupt flag that caused this interrupt, or this function will be called
     *    repetitively and lock your system.
     *  - NOTE that your code needs to be able to handle two GPIO interrupts occurring at the same time.
     */
    void HandleInterrupt();

 private:
    /**
     * Allocate a lookup table matrix here of function pointers (avoid dynamic allocation)
     * Upon AttachInterruptHandler(), you will store the user's function callback
     * Upon the EINT3 interrupt, you will find out which callback to invoke based on Port/Pin status.
     */
    IsrPointer pin_isr_map[2][32];
};
LabGpioInterrupts::LabGpioInterrupts()
{

}
void LabGpioInterrupts::Initialize()
{
    NVIC_EnableIRQ(EINT3_IRQn);
}

bool LabGpioInterrupts::AttachInterruptHandler(uint8_t port, uint32_t pin, IsrPointer pin_isr, InterruptCondition condition)
{


    //1. Check for bad inputs (0==Port 0, 1==Port 2)
    if(port<0 || port>1)//invalid port number
    {
        uart0_puts("Error AttachingInterruptHandler: invalid port number");
        return false;
    }

    //Ports 0 and 2 have 32 pins each (0 to 31)
    //For Port 0, pins 12-14 and 31 are reserved
    //For Port 2, pins 14-31 are reserved
    if(pin<0 || pin>30 || (port==0 && pin>11 && pin < 15) || (port==2 && pin>13))
    {
        uart0_puts("Error AttachingInterruptHandler: invalid pin number");
        return false;
    }
    if(pin_isr == NULL)
    {
        uart0_puts("Error AttachingInterruptHandler: null pin_isr");
        return false;
    }

    //2. Activate interrupts according to condition
    //Use different registers for different ports
    if(port==0)
    {
        if(condition != kFallingEdge)//For Both Edges or Rising Edge
        {
            LPC_GPIOINT->IO0IntEnR |= (1 << pin);//enable rising edge interrupts for port 0 pin
        }
        if(condition != kRisingEdge)//For Both Edges or Falling Edge
        {
            LPC_GPIOINT->IO0IntEnF |= (1 << pin);//enable falling edge interrupts for port 0 pin
        }
    }
    else//port == 1, indicating port 2
    {
        if(condition != kFallingEdge)//For Both Edges or Rising Edge
        {
            LPC_GPIOINT->IO2IntEnR |= (1 << pin);//enable rising edge interrupts for port 2 pin
        }
        if(condition != kRisingEdge)//For Both Edges or Falling Edge
        {
            LPC_GPIOINT->IO2IntEnF |= (1 << pin);//enable falling edge interrupts for port 2 pin
        }
    }

    //3. Save pin ISR
    if(pin_isr_map[port][pin])
    {
        uart0_puts("Warning AttachingInterruptHandler: writing over an existing ISR");
    }
    pin_isr_map[port][pin] = pin_isr;

    return true;
}
void LabGpioInterrupts::HandleInterrupt()
{
    //1. Check port/pin statuses.
    //2. Invoke the callback function from the lookup table.
    //NOTE: Need to handle multiple GPIO interrupts.
    //3. Clear the interrupt flag that caused this interrupt.

    //uart0_puts("Begin Handling Interrupts");
    while(LPC_GPIOINT->IntStatus & (1 << 0))//while there are pending interrupts on Port 0
    {
        //Check current pin statuses
        IO0IntStatF = LPC_GPIOINT->IO0IntStatF;
        IO0IntStatR = LPC_GPIOINT->IO0IntStatR;

        for(count = 0; count < 12; count++)//For Port 0 pins 0-11
        {
            if((IO0IntStatF | IO0IntStatR) & (1 << count))
            {
                pin_isr_map[0][count]();
                LPC_GPIOINT->IO0IntClr |= (1 << count);
            }
        }
        for(count = 15; count < 31; count++)//For Port 0 pins 15-30
        {
            if((IO0IntStatF | IO0IntStatR) & (1 << count))
            {
                pin_isr_map[0][count]();
                LPC_GPIOINT->IO0IntClr |= (1 << count);
            }
        }

    }
    while(LPC_GPIOINT->IntStatus & (1 << 2))//while there are pending interrupts on Port 2
    {
        //Check current pin statuses
        IO2IntStatF = LPC_GPIOINT->IO2IntStatF;
        IO2IntStatR = LPC_GPIOINT->IO0IntStatR;
        for(count = 0; count < 15; count++)//For Port 2 pins 0-14
        {
            if((IO2IntStatF | IO2IntStatR) & (1 << count))
            {
                pin_isr_map[1][count]();
                LPC_GPIOINT->IO2IntClr |= (1 << count);
            }
        }
    }
    //uart0_puts("End Handling Interrupts");

}
/**
 * Unless you design Singleton class, we need a global instance of our class because
 * the asynchronous Eint3Handler() will need to invoke our C++ class instance callback
 * WARNING: You must use this same instance while testing your main()
 */
LabGpioInterrupts gpio_interrupt;

/* Since we have a C++ class handle an interrupt, we need to setup a C function delegate to invoke it
 * So here is the skeleton code that you can reference.
 * This function will simply delegate the interrupt handling to our C++ class
 * The CPU interrupt should be attached to this function through isr_register()
 */
void Eint3Handler(void)
{
    //u0_dbg_printf("*");
    gpio_interrupt.HandleInterrupt();
}

/**
 * main() should register C function as callback for the EINT3
 * This is because we cannot register a C++ function as a callback through isr_register()
 *
 * There are workarounds, such as static functions inside a class, but that design is
 * not covered in this assignment.
 */

//This is where you define your ISRs
//void Port0Pin0ISR(void)
//{
//
//}

//void Port0Pin1ISR(void)
//{
//    uart0_puts("Port 0 Pin 1 ISR");
//}
//void Port2Pin6ISR(void)
//{
//    uart0_puts("Port 2 Pin 6 ISR");
//}

/* //Do stuff like this in main();
int main(void)
{
    // Init things once
    gpio_interrupt.Initialize();

    // Register C function which delegates interrupt handling to your C++ class function
    isr_register(EINT3_IRQn, Eint3Handler);

    // Create tasks and test your interrupt handler
    IsrPointer port0pin0 = Port0Pin0ISR;
    IsrPointer port0pin1 = Port0Pin1ISR;
    IsrPointer port2pin6 = Port2Pin6ISR;

    gpio_interrupt.AttachInterruptHandler(0, 0, port0pin0, kRisingEdge);//need to write port0pin0isr
    gpio_interrupt.AttachInterruptHandler(0, 1, port0pin1, kFallingEdge);//need to write port0pin1isr
    gpio_interrupt.AttachInterruptHandler(1, 6, port2pin6, kBothEdges);//need to write port2pin6isr

    while(1)
    {
        continue; //Empty loop for testing interrupts.
    }

}
*/
