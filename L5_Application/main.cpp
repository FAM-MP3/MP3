
#include <stdint.h>
#include <stdio.h>
#include "storage.hpp"
#include "ssp1.h"
//#include "gpio.hpp"
//#include "LabGPIO.hpp"
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
//#include "def.hpp"
//#include "player.cpp"
#if 0
void sineTest_begin(void *p) {
//    reset();  // hardware is already init

    uint16_t mode = ReadSci(SCI_MODE);
    mode |= 0x0020;
    WriteSci(SCI_MODE, mode);

    while(1)
    {
        while (GPIO2.read() == 0)
        ; // Wait until DREQ is high

        GPIO4.setLow(); // Activate xDCS
        WriteSpiByte(0x53);
        WriteSpiByte(0xEF);
        WriteSpiByte(0x6E);
        WriteSpiByte(0x7E); //n
        WriteSpiByte(0x00);
        WriteSpiByte(0x00);
        WriteSpiByte(0x00);
        WriteSpiByte(0x00);
        GPIO4.setHigh(); // Deactivate xDCS

        vTaskDelay(1000);
    }
}

void sineTest_end(void *p) {

    while(1)
    {
        GPIO4.setLow(); // Activate xDCS
        WriteSpiByte(0x45);
        WriteSpiByte(0x78);
        WriteSpiByte(0x69);
        WriteSpiByte(0x74);
        WriteSpiByte(0x00);
        WriteSpiByte(0x00);
        WriteSpiByte(0x00);
        WriteSpiByte(0x00);
        GPIO4.setHigh(); // Deactivate xDCS
    }
}
#endif

int main(void)
{
    /*      INIT functions for decoder      */
    ssp1_init();                            // initialize SPI1
//    ssp1_set_max_clock(12);                 // sets clock to 12MHz
//    printf("%d\n", sys_get_cpu_clock()/4);
    int success = 0;
    success &= InitVS10xx();               // initialize pins for VS1053B

//    success &= VSTestInitSoftware();
//    InitSoftware();

    if (success != 0) {
        printf("Failed initializing VS10xx, exiting\n");
        return -1;
    }
    else
    {
        printf("Init success!\n");
    }

//    xTaskCreate(sineTest_begin, "sineTest_begin", 1024, NULL, 3, NULL);
//    xTaskCreate(sineTest_end, "sineTest_end", 1024, NULL, 2, NULL);
//    vTaskStartScheduler();

    scheduler_add_task(new terminalTask(PRIORITY_HIGH));

    /* Consumes very little CPU, but need highest priority to handle mesh network ACKs */
    scheduler_add_task(new wirelessTask(PRIORITY_CRITICAL));

    scheduler_add_task(new reader());
    scheduler_add_task(new player());
//    scheduler_add_task(new sineTest());
    scheduler_start(); ///< This shouldn't return

    while(1);
    return 0;
}
