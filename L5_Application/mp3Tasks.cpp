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
#include "gpioInterrupts.hpp"

static volatile bool pauseSong, pauseToggled; //need to add settingsOpen

void Port0Pin1ISR(void)
{
    //toggle pause song
    pauseToggled = true;
}
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

        xQueueSend(data_queue_handle, &data, portMAX_DELAY);
        if(xSemaphoreTake(name_queue_filled_handle, 1))
       {
           tookSemaphore = true;
           offset = filesize; //stop playing this song

       }
    }
    return true;
}


/* ------------------------------------------- PLAYER------------------------------------------- */

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

//    volume = 0x00;                                       // This is the default volume in InitVS10xx()
//    setVolume(volume, volume);


    if(success == 0)
        return true;
    else
        return false;


}


bool player::run(void *p)
{
    //    static uint8_t data[512];
    //unsigned char data[512];
    unsigned char *bufP;



//    while(NULL == getSharedObject("data_queue"))

//        ;
//    volume_down_handle = getSharedObject("volume_down");

//    if (volume_down_handle == NULL)
//    {
//        u0_dbg_printf("ERROR: Player task failed to get handle for volume_down semaphore.\n");
//    }
//    else
//    {
//        u0_dbg_printf("SUCCESS: Player task got handle for volume_down semaphore.\n");
//    }

    if((data_queue_handle == NULL) || (data_queue_filled_handle == NULL))
    {
        data_queue_handle = getSharedObject("data_queue");

    //    while(NULL == getSharedObject("data_queue_filled"))
    //            ;
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


        while(pauseSong)//if we paused, wait here until unpaused
        {
            vTaskDelay(2); //wait here until song is unpaused.
        }

        if(xQueueReceive(data_queue_handle, &data, 5000))
        {

            // TODO: stream to mp3
            bufP = &data[0];
            DREQ_low();
            for(int j = 0; j < 512; j+=32)
            {
                taskENTER_CRITICAL();
                while (!getDREQ_lvl())
                {
                    continue;
                }// Wait until DREQ is high
                for(int i = 0; i < 32; i++)
                {
                   ssp1_exchange_byte(*bufP++);
                }
                taskEXIT_CRITICAL();

            }
            DREQ_high();

//            if(xSemaphoreTake(volume_down_handle, 1))
//            {
//
//                //set volume down
//                if(volume < 0xFE) // 0xFE is silence, so only lower if louder than silence
//                {
//                    volume += 5; //adding decreases volume
//                    if(volume >= 0xFE)//if we decrease past silence, set volume to silence
//                    {
//                        volume = 0xFE;
//                    }
//                }
//                setVolume(volume, volume);
//                u0_dbg_printf("volume lowered to %d", volume);
//            }

            }
            else
            {
                uart0_puts("ERROR: Player task failed to receive data from queue.\n");
            }

    vTaskDelay(1);
    return true;
}

//Buttons Task
buttons::buttons() :
    scheduler_task("Button Task", STACK_BYTES(4096), PRIORITY_HIGH)
{
    //Initialize handles to null (they'll be defined in run)
}
bool buttons::init(void)
{
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

    pauseToggled = false;
    pauseSong = false;

    // Init things once
    gpio_interrupt.Initialize();

    // Register C function which delegates interrupt handling to your C++ class function
    isr_register(EINT3_IRQn, Eint3Handler);

    // Create tasks and test your interrupt handler
    IsrPointer port0pin1 = Port0Pin1ISR;
    gpio_interrupt.AttachInterruptHandler(0, 1, port0pin1, kRisingEdge);//need to write port0pin0isr

    return true;
}

bool buttons::run(void *p)
{
    while(!pauseToggled)//wait until we need to address a button press
    {
        vTaskDelay(2);//must be greater than 0 to allow lower priority tasks to run
    }

    if(pauseToggled)//if we need to, address pause
    {
        vTaskDelay(250);//wait for bouncing to stop
        taskENTER_CRITICAL();//don't allow toggles while we're reacting to them
        if(pauseSong)
           {
               pauseSong = false;
               u0_dbg_printf("song unpaused\n");
           }
           else
           {
               pauseSong = true;
               u0_dbg_printf("song paused\n");
           }
        pauseToggled = false;
        taskEXIT_CRITICAL();
    }
    return true;
}

//sineTest Task
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
