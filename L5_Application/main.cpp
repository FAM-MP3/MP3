#include <stdint.h>
#include <stdio.h>
#include <string.h>         //strcat
#include "storage.hpp"
#include "ssp1.h"
#include "player.h"
#include "queue.h"
#include "ff.h"
#include "task.h"
#include "semphr.h"             //semaphores to signal reader task from playCmd handler
#include "uart0_min.h"          //UART0 output in Hercules
#include "scheduler_task.hpp"   //add shared objects
#include "mp3Tasks.hpp"         //tasks for MP3 player
#include "tasks.hpp"
#include "examples/examples.hpp"
#include "LabUART.hpp"
#include "LCDTask.hpp"
#include "utilities.h"
#include "io.hpp"
#include "LabGPIO.hpp"
#include "printf_lib.h"

int main(void)
{
    char splashScreen[] = {"Welcome to FAM MP3"};
    /*      INIT functions for decoder      */
    ssp1_init();                            // initialize SPI1
    int success = 0;
    success &= InitVS10xx();               // initialize pins for VS1053B
    bool lcd_success = InitLCD();

   if(!lcd_success)
       printf("Failed initializing LCD, exiting\n");

    if (success != 0) {
        printf("Failed initializing VS10xx, exiting\n");
        return -1;
    }
    else
    {
        printf("Init success!\n");
    }

    scheduler_add_task(new terminalTask(PRIORITY_HIGH));

    /* Consumes very little CPU, but need highest priority to handle mesh network ACKs */
    scheduler_add_task(new wirelessTask(PRIORITY_CRITICAL));
        scheduler_add_task(new buttons());

        scheduler_add_task(new LCD_UI());
        scheduler_add_task(new LCD_Select());
        scheduler_add_task(new LCD_Settings());
        scheduler_add_task(new reader());
        scheduler_add_task(new player());
    //    scheduler_add_task(new sineTest());

    scheduler_start(); ///< This shouldn't return

    while(1);
    return 0;
}
