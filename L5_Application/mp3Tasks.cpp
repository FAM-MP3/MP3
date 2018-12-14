/*
 * mp3Tasks.cpp
 *
 *  Created on: Nov 26, 2018
 *      Author: zoiem
 */

/*
 * mp3Tasks.cpp
 *
 *  Created on: Nov 26, 2018
 *      Author: zoiem
 */

#include <mp3Tasks.hpp>
#include <string.h>         //strcat
#include "player.h"
#include "printf_lib.h"
#include "ssp1.h"
#include "uart0_min.h"
#include <stdio.h>
//#include "gpioInterrupts.hpp"
#include "eint.h"
#include "InterruptLab.h"

static volatile bool pauseSong, pauseToggled; //need to add settingsOpen
SemaphoreHandle_t volume_down_handle = xSemaphoreCreateBinary();
uint8_t volume;

volatile SemaphoreHandle_t pause_toggle_handle = NULL;
volatile SemaphoreHandle_t pause_player_handle = NULL;
volatile SemaphoreHandle_t play_player_handle = NULL;



LabGpioInterrupts gpio_interrupt;
void Eint3Handler(void)
{
    gpio_interrupt.HandleInterrupt();
}

//static volatile bool pauseSong = false;

//static volatile bool pauseToggled; //need to add settingsOpen


void Port0Pin1ISR(void)
{
    //toggle pause song
    pauseToggled = true;
}

void Port0Pin0ISR(void)
{
    //toggle volume
    //pauseToggled = true;
    long yield = 0;
    xSemaphoreGiveFromISR(pause_toggle_handle, &yield);
    portYIELD_FROM_ISR(yield);
}

// ------------------------------------------- READER -------------------------------------------
reader::reader() :
    scheduler_task("MP3 Reader Task", STACK_BYTES(8192), PRIORITY_LOW)
{
    //Initialize handles to null (they'll be defined in run)


    /* Register work area to the default drive */
    f_mount(&Fs, "", 0);

}

bool reader::init(void)
{
    //TASK 1: Create queue for filename of song.
    //Fixed 32-byte song name: 32 chars because each char is 1 byte.
    name_queue_handle = xQueueCreate(1, sizeof(char)*32);//create queue
    data_queue_handle = xQueueCreate(2, sizeof(char)*512); //create queue TODO: double check size of queue
    if(name_queue_handle == NULL || data_queue_handle == NULL)//check that queue is created
    {
        uart0_puts("ERROR: initPlayCmd() failed to create queues.\n");
        return false;
    }
    if(!addSharedObject("name_queue", name_queue_handle) || !addSharedObject("data_queue", data_queue_handle))//try to and check sharing queue
    {
        uart0_puts("ERROR: initPlayCmd() failed to create shared objects for queue.\n");
        return false;
    }

    //TASK 2: Create semaphore for play song request.

    if(name_queue_filled_handle == NULL || data_queue_filled_handle == NULL || spi_bus_lock == NULL)//check create semaphore and mutex
    {
        uart0_puts("ERROR: initPlayCmd() failed to create semaphores.\n");
        return false;
    }
    if(!addSharedObject("name_queue_filled", name_queue_filled_handle) || !addSharedObject("data_queue_filled", data_queue_filled_handle) || !addSharedObject("spi_bus_lock", spi_bus_lock)) //try to and check share semaphore
    {
        uart0_puts("ERROR: initPlayCmd() failed to create shared objects for semaphores.\n");
        return false;
    }

    return true;
}

bool reader::run(void *p)
{
    if(!tookSemaphore)
    {
        xSemaphoreTake(name_queue_filled_handle, portMAX_DELAY);
    }

    if (xQueueReceive(name_queue_handle, &song_name, portMAX_DELAY))
    {
        tookSemaphore = false;
        offset = 0; //reset offset
        strcpy(file, prefix);       // file = '1:'
        strcat(file, song_name);    // file = '1:<song_name>'

        fr = f_open(&fil, file, FA_READ);
        filesize = fil.fsize;
        f_close(&fil);

    }

    while((offset < filesize))//play the current song while not at EOF or new semaphore
    {
        Storage::read(file, &data, sizeof(data), offset);
        offset += 512;
        // send 512-byte chunk
//       while(pauseSong)//if we paused, wait here until unpaused
//       {
//           vTaskDelay(1); //wait here until song is unpaused.
//       }

        xQueueSend(data_queue_handle, &data, portMAX_DELAY);
        if(xSemaphoreTake(name_queue_filled_handle, 0))
       {
           tookSemaphore = true;
           offset = filesize; //stop playing this song

       }
#if 0
=======
    char song_name[32];
    char prefix[3] = "1:";
    char file[35] = { 0 };              // [35] so that it could handle 32 bytes + 2 bytes for '1:'
    uint8_t data[512] = { 0 };
    unsigned long filesize;
    unsigned long offset = 0;

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
                    // read 512-byte chunk
                    Storage::read(file, &data, sizeof(data), offset);
                    offset += 512;
                    // send 512-byte chunk
                    xQueueSend(data_queue_handle, &data, portMAX_DELAY);
            }
        }
>>>>>>> Stashed changes

    }
#endif
    }
    vTaskDelay(1);
    return true;
}


// ------------------------------------------- PLAYER -------------------------------------------

player::player() :
    scheduler_task("MP3 Player Task", STACK_BYTES(4096), PRIORITY_MEDIUM)
{
    //Initialize handles to null (they'll be defined in run)
    data_queue_handle= NULL;
    data_queue_filled_handle = NULL;
}

bool player::init(void)
{
    int success = WriteSci(SCI_DECODE_TIME, 0x00);         // Reset DECODE_TIME

//    eint3_enable_port0(1, eint_rising_edge, Port0Pin0ISR);
//    volume = 0x00;                                       // This is the default volume in InitVS10xx()
    volume = 70;
    setVolume(volume, volume);
//    interrupted = false;

    if(success == 0)
        return true;
    else
        return false;
}


bool player::run(void *p)
{
    unsigned char *bufP;
    //    static uint8_t data[512];
    //unsigned char data[512];

    if((data_queue_handle == NULL) || (data_queue_filled_handle == NULL))
    {
        data_queue_handle = getSharedObject("data_queue");
        data_queue_filled_handle = getSharedObject("data_queue_filled");

        if (data_queue_handle == NULL)
        {
            u0_dbg_printf("ERROR: Player task failed to get handle for data queue.\n");
        }
        else
        {
            u0_dbg_printf("SUCCESS: Player task got handle for data queue.\n");
        }
        if (data_queue_filled_handle == NULL)
        {
            u0_dbg_printf("ERROR: Player task failed to get handle for data queue semaphore.\n");
        }
        else
        {
            u0_dbg_printf("SUCCESS: Player task got handle for data queue semaphore.\n");
        }
    }
        while(pauseSong);//if we paused, wait here until unpaused

        if(xSemaphoreTake(pause_player_handle, 0))//if we paused, wait here until unpaused
        {
            xSemaphoreTake(play_player_handle, portMAX_DELAY);
            vTaskDelay(100);
        }

        if(xQueueReceive(data_queue_handle, &data, 1000))
        {

            bufP = &data[0];
            int success = 0;

            play(bufP, 512);

//            for(int j = 0; j < 512; j+=32)
//            {
//               taskENTER_CRITICAL();
//                while (!getDREQ_lvl())
//                {
//                    continue;
//                }// Wait until DREQ is high
//                for(int i = 0; i < 32; i++)
//                {
//                   ssp1_exchange_byte(*bufP++);
//                }
//                taskEXIT_CRITICAL();
//
//                if(success != 0){
//                    //error
//                    uart0_puts("WriteSdiChar failed!\n");
//                }
//            }
        }
        else
        {
            uart0_puts("ERROR: Player task failed to receive data from queue.\n");
            vTaskDelay(2);
        }

    vTaskDelay(2);
    return true;
}

// ------------------------------------------- BUTTONS -------------------------------------------

buttons::buttons() :
    scheduler_task("Button Task", STACK_BYTES(4096), PRIORITY_HIGH)
{
    //Initialize handles to null (they'll be defined in run)
}
bool buttons::init(void)
{
    pauseToggled = false;
    pauseSong = false;

    if(volume_down_handle == NULL)//check create semaphore and mutex
    {
        uart0_puts("ERROR: initButton() failed to create semaphores.\n");
        return false;
    }

    eint3_enable_port0(1, eint_rising_edge, Port0Pin0ISR);
//    eint3_enable_port0(1, eint_rising_edge, Port0Pin1ISR);    // for pause

    pause_toggle_handle = xSemaphoreCreateBinary();
    pause_player_handle = xSemaphoreCreateBinary();
    play_player_handle = xSemaphoreCreateBinary();

#if 0
>>>>>>> Stashed changes
=======
    uart0_puts("buttons initialized\n");
//    volume_down_handle = xSemaphoreCreateBinary(); //create semaphore
//
//
//    if(volume_down_handle == NULL)//check create semaphore and mutex
//    {
//        uart0_puts("ERROR: initButton() failed to create semaphores.\n");
//        return false;
//    }
//    if(!addSharedObject("volume_down", volume_down_handle)) //try to and check share semaphore
//    {
//        uart0_puts("ERROR: initButton() failed to create shared objects for semaphores.\n");
//        return false;
//    }

    pauseSong = false;

    pause_toggle_handle = xSemaphoreCreateBinary();
    pause_player_handle = xSemaphoreCreateBinary();
    play_player_handle = xSemaphoreCreateBinary();

>>>>>>> 21e32e695c35c7b55a3c06448f2cb8a6de6d9369
    // Init things once
    gpio_interrupt.Initialize();

    // Register C function which delegates interrupt handling to your C++ class function
    isr_register(EINT3_IRQn, Eint3Handler);

    // Create tasks and test your interrupt handler
<<<<<<< Updated upstream
    IsrPointer port0pin1 = Port0Pin1ISR;
    gpio_interrupt.AttachInterruptHandler(0, 1, port0pin1, kRisingEdge);//need to write port0pin0isr
=======
    IsrPointer port0pin0 = Port0Pin0ISR;

    gpio_interrupt.AttachInterruptHandler(0, 0, port0pin0, kRisingEdge);//need to write port0pin0isr
#endif

    return true;
}

bool buttons::run(void *p)
{
    if(xSemaphoreTake(pause_toggle_handle, portMAX_DELAY))
    {
        vTaskDelay(250);//wait for bouncing to stop
        while(xSemaphoreTake(pause_toggle_handle, 0))
            {
                   continue; //take any remaining semaphores
            }

       if(pauseSong)
          {
              pauseSong = false;
              xSemaphoreGive(play_player_handle);
              u0_dbg_printf("song unpaused\n");
          }
          else
          {
              pauseSong = true;
              xSemaphoreGive(pause_player_handle);
              u0_dbg_printf("song paused\n");
          }
    }
    vTaskDelay(2);
    return true;
}

// ------------------------------------------- SINE TEST -------------------------------------------
sineTest::sineTest() :
    scheduler_task("MP3 Sine Test Task", STACK_BYTES(2048), PRIORITY_MEDIUM)
{

}
bool sineTest::init(void)
{
    uint16_t mode = ReadSci(SCI_MODE);
    mode |= 0x0020;
    int result = WriteSci(SCI_MODE, mode);

    return ~result;     // WriteSci has reverse logic
}

bool sineTest::run(void *p)
{
    sineTest_begin();
    vTaskDelay(5000);
    sineTest_end();
    return true;
}
