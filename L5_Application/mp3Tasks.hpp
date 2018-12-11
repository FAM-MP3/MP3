/*
 * mp3Tasks.h
 *
 *  Created on: Nov 26, 2018
 *      Author: zoiem
 */

#ifndef MP3TASKS_HPP_
#define MP3TASKS_HPP_

#include "scheduler_task.hpp"   //shared objects
#include "queue.h"              //queues to send filename of song to play
#include "semphr.h"             //semaphores to signal reader task from playCmd handler
#include "uart0_min.h"          //UART0 output in Hercules
#include <stdio.h>              //print f
#include "ff.h"                 //file system
#include "storage.hpp"

class reader : public scheduler_task
{
    public :
        reader();
        bool init(void);
        bool run(void *p);
    private:
        SemaphoreHandle_t name_queue_filled_handle;
        SemaphoreHandle_t data_queue_filled_handle;
        SemaphoreHandle_t spi_bus_lock;
        QueueHandle_t name_queue_handle;
        QueueHandle_t data_queue_handle;
        FATFS Fs;        /* Work area (filesystem object) for logical drive */
        FIL fil;        /* File object */
        FRESULT fr;     /* FatFs return code */
        FILE * fp;
//        unsigned char data[512];
};

class player : public scheduler_task
{
    public :
        player();
        bool init(void);
        bool run(void *p);
    private:
        SemaphoreHandle_t data_queue_filled_handle;         // didn't manage to implement this semaphore. TODO: verify if it's needed
        QueueHandle_t data_queue_handle;
        unsigned char data[512];
};

class sineTest : public scheduler_task
{
    public :
        sineTest();
        bool init(void);
        bool run(void *p);
};

#endif /* MP3TASKS_HPP_ */
