
#include <stdint.h>
#include <stdio.h>
//#include <string.h>         //strcat
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

#if 0
FATFS Fs;
QueueHandle_t name_queue_handle;
QueueHandle_t data_queue_handle;
SemaphoreHandle_t name_queue_filled_handle;
SemaphoreHandle_t data_queue_filled_handle;


void vReader(void *p) {
    name_queue_handle = scheduler_task::getSharedObject("name_queue");
    name_queue_filled_handle = scheduler_task::getSharedObject("name_queue_filled");

    char song_name[32];
    char prefix[3] = "1:";
    char file[35] = { 0 };              // [35] so that it could handle 32 bytes + 2 bytes for '1:'
    uint8_t data[512] = { 0 };
    unsigned long filesize;
    unsigned long offset = 0;
    FRESULT fr;     /* FatFs return code */
    FILE * fp;
    FIL fil;        /* File object */

    while(1)
    {
        if(xSemaphoreTake(name_queue_filled_handle, portMAX_DELAY))
        {
            if (xQueueReceive(name_queue_handle, &song_name, portMAX_DELAY))
            {
                strcpy(file, prefix);       // file = '1:'
                strcat(file, song_name);    // file = '1:<song_name>'

                fr = f_open(&fil, file, FA_READ);
                filesize = fil.fsize;
                f_close(&fil);

                while(offset < filesize)
                {
                        Storage::read(file, &data, sizeof(data), offset);
                        offset += 512;
                        // send 512-byte chunk
                        xQueueSend(data_queue_handle, &data, portMAX_DELAY);
                }

            }
        }

    }
}

void vPlayer(void *p) {
    unsigned char *bufP;
    unsigned char data[512];

    while(1)
    {
        if(xQueueReceive(data_queue_handle, &data, 5000))
        {
            bufP = &data[0];
            int success = 0;

            DREQ_low();

            for(int j = 0; j < 512; j+=32)
            {
                while (!getDREQ_lvl())
                {
                    continue;
                }
        //        ; // Wait until DREQ is high
                for(int i = 0; i < 32; i++)
                {
                   ssp1_exchange_byte(*bufP++);
                }
            }

            DREQ_high();

            if(success != 0){
                //error
                uart0_puts("WriteSdiChar failed!\n");
            }
        }
        else
        {
            uart0_puts("ERROR: Player task failed to receive data from queue.\n");
        }
    }
}
#endif

int main(void)
{
#if 0
    name_queue_handle = xQueueCreate(1, sizeof(char)*32);//create queue
    data_queue_handle = xQueueCreate(10, sizeof(char)*512); //create queue TODO: double check size of queue
    name_queue_filled_handle = xSemaphoreCreateBinary(); //create semaphore

    if(!scheduler_task::addSharedObject("name_queue_filled", name_queue_filled_handle)) //try to and check share semaphore
    {
        uart0_puts("ERROR: initPlayCmd() failed to create shared objects for semaphores.\n");
    }
#endif
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


//    vTaskStartScheduler();

    scheduler_add_task(new terminalTask(PRIORITY_HIGH));

    /* Consumes very little CPU, but need highest priority to handle mesh network ACKs */
    scheduler_add_task(new wirelessTask(PRIORITY_CRITICAL));

    scheduler_add_task(new buttons());
    scheduler_add_task(new reader());
    scheduler_add_task(new player());

//    scheduler_add_task(new sineTest());
//    xTaskCreate(vReader, "reader", 1024, NULL, 2, NULL);
//    xTaskCreate(vPlayer, "player", 1024, NULL, 3, NULL);
    scheduler_start(); ///< This shouldn't return

    while(1);
    return 0;
}
