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

//static volatile bool pauseSong, pauseToggled; //need to add settingsOpen
//SemaphoreHandle_t volume_down_handle = xSemaphoreCreateBinary();
//uint8_t volume;
/*
volatile SemaphoreHandle_t pause_toggle_handle = NULL;
volatile SemaphoreHandle_t pause_player_handle = NULL;
volatile SemaphoreHandle_t play_player_handle = NULL;
*/

volatile SemaphoreHandle_t pause_reader_semaphore_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t volume_down_handle = xSemaphoreCreateBinary();

static volatile bool buttonReady = true;
LabGpioInterrupts gpio_interrupt;
void Eint3Handler(void)
{
    gpio_interrupt.HandleInterrupt();
}

//static volatile bool pauseSong = false;

//static volatile bool pauseToggled; //need to add settingsOpen

uint mask = 0x7;
uint i;

void Port0Pin29ISR(void)
{
    taskENTER_CRITICAL();
    if(buttonReady)
    {
        buttonReady = false;
        u0_dbg_printf("*");//debug for bouncing
        long yield = 0;
        xSemaphoreGiveFromISR(pause_reader_semaphore_handle, &yield);
        portYIELD_FROM_ISR(yield);
    }
    taskEXIT_CRITICAL();
}

void Port0Pin0ISR(void)
{
    //toggle volume
    long yield = 0;
    xSemaphoreGiveFromISR(volume_down_handle, &yield);
    portYIELD_FROM_ISR(yield);
}

// ------------------------------------------- READER -------------------------------------------
reader::reader() :
    scheduler_task("Reader Task", STACK_BYTES(8192), PRIORITY_LOW)
{
    //Initialize handles to null (they'll be defined in run)


    /* Register work area to the default drive */
    f_mount(&Fs, "", 0);

}

bool reader::init(void)
{
    //TASK 1: Create queue for filename of song.
    //Fixed 32-byte song name: 32 chars because each char is 1 byte.
//    name_queue_handle = xQueueCreate(1, sizeof(char)*32);//create queue
//    data_queue_handle = xQueueCreate(2, sizeof(char)*512); //create queue TODO: double check size of queue
    if(name_queue_handle == NULL || data_queue_handle == NULL)//check that queue is created
    {
        uart0_puts("ERROR: initPlayCmd() failed to create queues.\n");
        return false;
    }
    if((!addSharedObject("name_queue", name_queue_handle)) || (!addSharedObject("data_queue", data_queue_handle)))//try to and check sharing queue
    {
        uart0_puts("ERROR: initPlayCmd() failed to create shared objects for queue.\n");
        return false;
    }

    //TASK 2: Create semaphore for play song request.

    if(name_queue_filled_handle == NULL || data_queue_filled_handle == NULL || spi_bus_lock == NULL /*|| pause_reader_semaphore_handle == NULL*/)//check create semaphore and mutex
    {
        uart0_puts("ERROR: initPlayCmd() failed to create semaphores.\n");
        return false;
    }
    if((!addSharedObject("name_queue_filled", name_queue_filled_handle)) || (!addSharedObject("data_queue_filled", data_queue_filled_handle)) || (!addSharedObject("spi_bus_lock", spi_bus_lock)) /*|| (!addSharedObject("pause_reader_semaphore", pause_reader_semaphore_handle))*/) //try to and check share semaphore
    {
        uart0_puts("ERROR: initPlayCmd() failed to create shared objects for semaphores.\n");
        return false;
    }

    return true;
}

bool reader::run(void *p)
{
    while(pause_player_semaphore_handle == NULL)
    {
        pause_player_semaphore_handle = getSharedObject("pause_player_semaphore");
        vTaskDelay(2);
    }

    if(!tookSongQSemaphore)//Take semaphore if we need to (first song or after EOF)
    {
        xSemaphoreTake(name_queue_filled_handle, portMAX_DELAY);
    }

    if (xQueueReceive(name_queue_handle, &song_name, portMAX_DELAY))//Whenever we start a new song, initialize variables
    {
        tookSongQSemaphore = false;
        tookPauseSemaphore = false;
        offset = 0; //reset offset
        strcpy(file, prefix);       // file = '1:'
        strcat(file, song_name);    // file = '1:<song_name>'

        fr = f_open(&fil, file, FA_READ);
        filesize = fil.fsize;
        f_close(&fil);

    }

    while((offset < filesize))//play the current song while not at EOF or new semaphore
    {
        Storage::read(file, &data, sizeof(data), offset); //read song data from file
        offset += 512;
        // send 512-byte chunk
//       while(pauseSong)//if we paused, wait here until unpaused
//       {
//           vTaskDelay(1); //wait here until song is unpaused.
//       }

        xQueueSend(data_queue_handle, &data, portMAX_DELAY); //send it to the decoder
       if(xSemaphoreTake(name_queue_filled_handle, 0))//If we get a semaphore for a new song while a song is currently playing
       {
            tookSongQSemaphore = true; //we don't need to take the semaphore again
           offset = filesize; //stop playing this song

       }
       else if(xSemaphoreTake(pause_reader_semaphore_handle, 0)) //if we get a semaphore to pause the song
       {
            //PAUSE
            //tell the player to pause
            taskENTER_CRITICAL();
            xSemaphoreGive(pause_player_semaphore_handle);
            taskEXIT_CRITICAL();
            //debounce
            vTaskDelay(1000 / portTICK_PERIOD_MS); //wait 1 second for button bounce
            taskENTER_CRITICAL();
            buttonReady = true;
            taskEXIT_CRITICAL();

            //WAIT FOR UNPAUSE
            xSemaphoreTake(pause_reader_semaphore_handle, portMAX_DELAY); //do nothing until the song is unpaused

            //UNPAUSE
            //tell player to unpause
            taskENTER_CRITICAL();
            xSemaphoreGive(pause_player_semaphore_handle);
            taskEXIT_CRITICAL();
            //debounce
            vTaskDelay(500 / portTICK_PERIOD_MS); //wait 1/2 second for button bounce
            taskENTER_CRITICAL();
            buttonReady = true;
            taskEXIT_CRITICAL();
       }
    }
    vTaskDelay(2);
    return true;
}


// ------------------------------------------- PLAYER -------------------------------------------

player::player() :
    scheduler_task("Player Task", STACK_BYTES(8192), PRIORITY_MEDIUM)
{

}

bool player::init(void)
{

    int success = WriteSci(SCI_DECODE_TIME, 0x00);         // Reset DECODE_TIME

//    eint3_enable_port0(1, eint_rising_edge, Port0Pin0ISR);
//    volume = 0x00;                                       // This is the default volume in InitVS10xx()
    setVolume(volume, volume);
//    interrupted = false;

    if(!addSharedObject("pause_player_semaphore", pause_player_handle))
    {
        return false;
    }

    if(success == 0)
        return true;
    else
        return false;
}


bool player::run(void *p)
{
    //    static uint8_t data[512];
    //unsigned char data[512];

    while(initialized == false)
    {
        data_queue_handle = getSharedObject("data_queue");

        if(data_queue_handle == NULL)
        {
            u0_dbg_printf("ERROR: Player task failed to get handle for data queue.\n");
        }
        else
        {
            u0_dbg_printf("\nSUCCESS: Player task got all queue and semaphore handles.\n");
            initialized = true;
        }
    }

    if(xSemaphoreTake(pause_player_handle, 0) == pdTRUE)//if we get a request to pause the song
    {
        xSemaphoreTake(pause_player_handle, portMAX_DELAY); //wait until unpaused
    }
    else if((xSemaphoreTake(volume_down_handle, 0) == pdTRUE) && (volume < 0xff))//if we get a request to lower volume
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        volume = ((volume - 50) > 0xff)?0xff:(volume-50);
        setVolume(volume, volume);
    }
    else if(xQueueReceive(data_queue_handle, &data, 5000))//if we can get song data from the queue
    {
        bufP = &data[0];
        play(bufP, 512);
    }
    else
    {
        uart0_puts("WARNING: Player task waited more than 5 seconds without receiving data from queue.\n");
        vTaskDelay(2);
    }



    vTaskDelay(2);
    return true;
}

// ------------------------------------------- BUTTONS -------------------------------------------

buttons::buttons() :
    scheduler_task("Button Task", STACK_BYTES(4096), PRIORITY_HIGH)
{

}
bool buttons::init(void)
{
//    pauseToggled = false;
//    pauseSong = false;
//
//    if(volume_down_handle == NULL)//check create semaphore and mutex
//    {
//        uart0_puts("ERROR: initButton() failed to create semaphores.\n");
//        return false;
//    }
//
//    eint3_enable_port0(1, eint_rising_edge, Port0Pin0ISR);
        eint3_enable_port0(29, eint_rising_edge, Port0Pin29ISR);    // for pause
//
//    pause_toggle_handle = xSemaphoreCreateBinary();
//    pause_player_handle = xSemaphoreCreateBinary();
//    play_player_handle = xSemaphoreCreateBinary();

    return true;
}

bool buttons::run(void *p)
{
//    if(xSemaphoreTake(pause_toggle_handle, portMAX_DELAY))
//    {
//        vTaskDelay(250);//wait for bouncing to stop
//        while(xSemaphoreTake(pause_toggle_handle, 0))
//            {
//                   continue; //take any remaining semaphores
//            }
//
//       if(pauseSong)
//          {
//              pauseSong = false;
//              xSemaphoreGive(play_player_handle);
//              u0_dbg_printf("song unpaused\n");
//          }
//          else
//          {
//              pauseSong = true;
//              xSemaphoreGive(pause_player_handle);
//              u0_dbg_printf("song paused\n");
//          }
//    }
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
