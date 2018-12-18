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
        QueueHandle_t name_queue_handle = xQueueCreate(1, sizeof(char)*32);//create queue
        QueueHandle_t data_queue_handle = xQueueCreate(2, sizeof(char)*512); //create queue TODO: double check size of queue
        SemaphoreHandle_t name_queue_filled_handle = xSemaphoreCreateBinary(); //create semaphore
        SemaphoreHandle_t data_queue_filled_handle = xSemaphoreCreateBinary(); //create semaphore
        //SemaphoreHandle_t pause_reader_semaphore_handle = xSemaphoreCreateBinary(); //create semaphore
        SemaphoreHandle_t pause_player_semaphore_handle = NULL; //create semaphore
        SemaphoreHandle_t spi_bus_lock = xSemaphoreCreateMutex();              //create mutex
        FATFS Fs;        /* Work area (filesystem object) for logical drive */
        FIL fil;        /* File object */
        FRESULT fr;     /* FatFs return code */
//        unsigned char data[512];
        char song_name[32];
        char prefix[3] = "1:";
        char file[35] = { 0 };              // [35] so that it could handle 32 bytes + 2 bytes for '1:'
        uint8_t data[512] = { 0 };
        unsigned long filesize = 0;
        unsigned long offset = 0;
        //bool EndOfFile = true;
        bool tookSongQSemaphore = false, tookPauseSemaphore = false;
};

class player : public scheduler_task
{
    public :
        player();
        bool init(void);
        bool run(void *p);
    private:
        SemaphoreHandle_t pause_player_handle = xSemaphoreCreateBinary();
        QueueHandle_t data_queue_handle= NULL;
//        SemaphoreHandle_t volume_down_handle = xSemaphoreCreateBinary();
        unsigned char data[512];
        uint8_t volume = 50;
//        bool interrupted;
        bool initialized = false;
        unsigned char *bufP = NULL;
};

class sineTest : public scheduler_task
{
    public :
        sineTest();
        bool init(void);
        bool run(void *p);
};

class buttons : public scheduler_task
{
    public :
        buttons();
        bool init(void);
        bool run(void *p);
    private:
//        SemaphoreHandle_t volume_down_handle;
//        uint8_t volume;
};

#endif /* MP3TASKS_HPP_ */
