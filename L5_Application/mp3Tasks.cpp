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

reader::reader() :
    scheduler_task("MP3 Reader Task", STACK_BYTES(8192), PRIORITY_MEDIUM)
{
    //Initialize handles to null (they'll be defined in run)
    name_queue_handle= NULL;
    name_queue_filled_handle = NULL;

    /* Register work area to the default drive */
    f_mount(&Fs, "", 0);

}

bool reader::init(void)
{
    //TASK 1: Create queue for filename of song.
    //Fixed 32-byte song name: 32 chars because each char is 1 byte.
    name_queue_handle = xQueueCreate(1, sizeof(char)*32);//create queue
    data_queue_handle = xQueueCreate(10, sizeof(char)*512); //create queue TODO: double check size of queue
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
    name_queue_filled_handle = xSemaphoreCreateBinary(); //create semaphore
    data_queue_filled_handle = xSemaphoreCreateBinary(); //create semaphore
    spi_bus_lock = xSemaphoreCreateMutex();              //create mutex

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
    name_queue_handle = getSharedObject("name_queue");
    name_queue_filled_handle = getSharedObject("name_queue_filled");
    data_queue_handle = getSharedObject("data_queue");
    data_queue_filled_handle = getSharedObject("data_queue_filled");
    spi_bus_lock = getSharedObject("spi_bus_lock");
    if (name_queue_handle == NULL)
    {
       uart0_puts("ERROR: Reader task failed to get handle for name queue.\n");
    }
    if (name_queue_filled_handle == NULL)
    {
      uart0_puts("ERROR: Reader task failed to get handle for name queue semaphore.\n");
    }
    if (data_queue_handle == NULL)
    {
       uart0_puts("ERROR: Reader task failed to get handle for data queue.\n");
    }
    if (spi_bus_lock == NULL)
    {
        uart0_puts("ERROR: Reader task failed to get handle for spi_bus_lock.\n");
    }

    char song_name[32];
    char prefix[3] = "1:";
    char file[35] = { 0 };              // [35] so that it could handle 32 bytes + 2 bytes for '1:'
    uint8_t data[512] = { 0 };
//    char data[100];
    unsigned long filesize;
    unsigned long offset = 0;
//    uint br;
//    uint br_left;
//    FILINFO* fno;
//    TCHAR* f_get_result;
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
//                if((filesize - offset) > 32)
//                {
                    // read 512-byte chunk
                    Storage::read(file, &data, sizeof(data), offset);
                    offset += 512;
                    // send 512-byte chunk
                    xQueueSend(data_queue_handle, &data, portMAX_DELAY);
//                    memset(data,0,sizeof(data));

//                }
//                else
//                {
//                    Storage::read(file, &data, (filesize - offset), offset);
//                    offset += (filesize - offset);
//
//                    xQueueSend(data_queue_handle, &data, portMAX_DELAY);
//                    memset(data,0,sizeof(data));
//                }
            }

        }

#if 0
        //Add code here for reader task to attempt to open the file and start to transfer 512 byte chunks to the player task

        //Test code
        if (xQueueReceive(name_queue_handle, &song_name, portMAX_DELAY))
        {
            fflush(stdout); //this only works for printf, not uart0puts
            //vTaskDelay(1); //wait for output buffer to flush, since I don't know how to do that for uart0_puts
//            u0_dbg_printf("SUCCESS: Got song name \"%s\"\n\n", song_name);

            strcpy(file, prefix);       // file = '1:'
            strcat(file, song_name);    // file = '1:<song_name>'

            fr = f_open(&fil, file, FA_READ);

//            label:
//            FILE *fp1 = fopen("1:message.txt", "r");
//            if(fp1 == NULL){
//                u0_dbg_printf("ERROR READING FILE");
//                goto label;
//            }

//            u0_dbg_printf("opened file: %s\n", file);
//            int x = fread(data, 512, 1, fp1);
//            u0_dbg_printf("------------%x\n",data[0]);
//            fclose(fp1);
//            u0_dbg_printf("file closed\n");

            if (fr)
                u0_dbg_printf("Error opening file. FR: %i\n", (int)fr);
            else {

//                f_get_result = f_gets((TCHAR*)data, sizeof(data), &fil);
//                if(f_get_result == 0)
//                    u0_dbg_printf("f_get error\n");
//                while (f_get_result) {
////                    fr = f_stat(file, fno);
////                    if(fr != FR_OK)
////                       u0_dbg_printf("Error reading file status. FR: %i\n", (int)fr);
////                    u0_dbg_printf("file size: %d\n", fno->fsize);
////                    u0_dbg_printf("file date: %s\n", fno->fdate);
////                    printf(data);
//                    xQueueSend(data_queue_handle, &data, portMAX_DELAY);
////                    xSemaphoreGive(data_queue_filled_handle);
//                    f_get_result = f_gets((TCHAR*)data, sizeof(data), &fil);
//                    if(f_get_result == 0)
//                        u0_dbg_printf("f_get error\n");
//                }
                fr = f_read(&fil, data, 32, &br);

                if(fr != FR_OK){
                    u0_dbg_printf("Error reading file. FR: %i\n", (int)fr);
//                    fr = f_stat(file, fno);
//                    if(fr != FR_OK)
//                       u0_dbg_printf("Error reading file status. FR: %i\n", (int)fr);
                }
                //printf("%c %x, bytes read %d",data[br-1],data[br-1],br);
                /*if((br != 32) && (*data != -1)){
                    br_left = 32-br;
                    u0_dbg_printf("read data<32 beginning of while\n");
                    while((br_left != 0) && (*data != -1)){
                       fr = f_read(&fil, data+(32-br_left), br_left, &br);
                       if(fr != FR_OK){
                           u0_dbg_printf("Error reading file. FR: %i\n", (int)fr);
//                           fr = f_stat(file, fno);
//                           if(fr != FR_OK)
//                              u0_dbg_printf("Error reading file status. FR: %i\n", (int)fr);

                       }
                       u0_dbg_printf("br_left: %d\n", br_left);
                       u0_dbg_printf("br: %d\n", br);
                       u0_dbg_printf("data: %s\n", *data);
                        br_left -= br;

                    }
                    u0_dbg_printf("read data<32 end of while\n");
                }*/
                xQueueSend(data_queue_handle, &data, portMAX_DELAY);
                memset(data,0,sizeof(data));
                vTaskDelay(1);
                do{
//                    u0_dbg_printf("br: %d\n", br);
                    if(br == 0) //Empty file, break.
                    {
                      break;
                    }
                    else
                    {
                        //Push music into Queue
//                        u0_dbg_printf("s\n");

                        //Get next block of mp3 data
                        fr = f_read(&fil, data, 32, &br);
                        if(fr != FR_OK){
                            u0_dbg_printf("Error reading file. FR: %i\n", (int)fr);
//                            fr = f_stat(file, fno);
//                            if(fr != FR_OK)
//                               u0_dbg_printf("Error reading file status. FR: %i\n", (int)fr);

                        }
                       // printf("%c %x, bytes read %d",data[br-1],data[br-1],br);
                        /*if((br != 32) && (*data != -1)){
                            br_left = 32-br;
                            u0_dbg_printf("read data<32 beginning of while\n");
                            while((br_left != 0) && (*data != -1)){
                               fr = f_read(&fil, data+(32-br_left), br_left, &br);
                               if(fr != FR_OK){
                                   u0_dbg_printf("Error reading file. FR: %i\n", (int)fr);
//                                   fr = f_stat(file, fno);
//                                   if(fr != FR_OK)
//                                      u0_dbg_printf("Error reading file status. FR: %i\n", (int)fr);

                               }
                               u0_dbg_printf("br_left: %d\n", br_left);
                               u0_dbg_printf("br: %d\n", br);
                               u0_dbg_printf("data: %s\n", *data);
                                br_left -= br;
                            }
                            u0_dbg_printf("read data<32 end of while\n");
                        }*/
                    }
                    xQueueSend(data_queue_handle, &data, portMAX_DELAY);
                    memset(data,0,sizeof(data));
                    vTaskDelay(1);

                } while (br != 0); // Loops while data still in file.

                /* Close the file */
                f_close(&fil);
            }

        }
        else
        {
            uart0_puts("ERROR: Reader task received semaphore, but failed to receive song_name from queue.\n");
        }
#endif

    }
    return true;
}


/* ------------------------------------------- PLAYER------------------------------------------- */

player::player() :
    scheduler_task("MP3 Player Task", STACK_BYTES(4096), PRIORITY_HIGH)
{
    //Initialize handles to null (they'll be defined in run)
    data_queue_handle= NULL;
    data_queue_filled_handle = NULL;
}

bool player::init(void)
{
    int success = WriteSci(SCI_DECODE_TIME, 0x00);         // Reset DECODE_TIME

//    while(NULL == getSharedObject("data_queue"))
//        ;
//    data_queue_handle = getSharedObject("data_queue");
//
//    while(NULL == getSharedObject("data_queue_filled"))
//            ;
//    data_queue_filled_handle = getSharedObject("data_queue_filled");
//
//    if (data_queue_handle == NULL)
//    {
//        printf("ERROR: Player task failed to get handle for data queue.\n");
//    }
//    else
//    {
//        printf("SUCCESS: Player task got handle for data queue.\n");
//    }
//    if (data_queue_filled_handle == NULL)
//    {
//        printf("ERROR: Player task failed to get handle for data queue semaphore.\n");
//    }
//    else
//    {
//        printf("SUCCESS: Player task got handle for data queue semaphore.\n");
//    }

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


    if(xQueueReceive(data_queue_handle, &data, 5000))
    {
        // TODO: stream to mp3
        bufP = &data[0];
        int success = 0;
//        for(int i=0; (i<512 && success == 0); i+=32){
//            success |= WriteSdiChar(bufP, 32);
//            bufP += 32;
////            printf((const char*)data);
//        }
//        int trackSize = 0;
//
//        while(trackSize < 32)
//        {

            DREQ_low();

            for(int j = 0; j < 512; j+=32)
            {
                while (!getDREQ_lvl())
                {
//                    vTaskDelay(2);
    //                uart0_puts("d");
                    continue;
                }
        //        ; // Wait until DREQ is high
                for(int i = 0; i < 32; i++)
                {
                   ssp1_exchange_byte(*bufP++);
                }
            }
//            trackSize += 32;
//        }
        DREQ_high();

//        play(data, 512);
        if(success != 0){
            //error
            uart0_puts("WriteSdiChar failed!\n");
//            printf()

        }
//        uart0_puts("r");
//        int len = strlen((const char*)data);
//        for(int i=0; i<len; i++){
//            printf("%c", data[i]);
//        }

//               for(int i=0; i<32; i++){
//                   uart0_putchar(data[i]);
//               }
//        printf("\n");
//        uart0_puts((const char*) data);
//        u0_dbg_printf("data: %s\n", *data);

        //Test code
//        if (xSemaphoreTake(data_queue_filled_handle, portMAX_DELAY))
//        {
//            fflush(stdout); //this only works for printf, not uart0puts
//            //vTaskDelay(1); //wait for output buffer to flush, since I don't know how to do that for uart0_puts
//            printf(data);
//        }
//        else
//        {
//            uart0_puts("ERROR: Player task received semaphore, but failed to receive data from queue.\n");
//        }

    }
    else
    {
        uart0_puts("ERROR: Player task failed to receive data from queue.\n");
    }
    vTaskDelay(1);
    return true;
}

//--

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
#if 0
#include <mp3Tasks.hpp>
#include <string.h>         //strcat
#include "player.h"
#include "printf_lib.h"

reader::reader() :
    scheduler_task("MP3 Reader Task", STACK_BYTES(8192), PRIORITY_MEDIUM)
{
    //Initialize handles to null (they'll be defined in run)
    name_queue_handle= NULL;
    name_queue_filled_handle = NULL;

    /* Register work area to the default drive */
    f_mount(&Fs, "", 0);

}

bool reader::init(void)
{
    //TASK 1: Create queue for filename of song.
    //Fixed 32-byte song name: 32 chars because each char is 1 byte.
    name_queue_handle = xQueueCreate(1, sizeof(char)*32);//create queue
    data_queue_handle = xQueueCreate(32, sizeof(uint8_t)*512); //create queue TODO: double check size of queue
    if(name_queue_handle == NULL || data_queue_handle == NULL)//check that queue is created
    {
        uart0_puts("_ERROR_: initPlayCmd() failed to create queues.\n");
        return false;
    }
    if(!addSharedObject("name_queue", name_queue_handle) || !addSharedObject("data_queue", data_queue_handle))//try to and check sharing queue
    {
        uart0_puts("_ERROR_: initPlayCmd() failed to create shared objects for queue.\n");
        return false;
    }

    //TASK 2: Create semaphore for play song request.
    name_queue_filled_handle = xSemaphoreCreateBinary(); //create semaphore
    data_queue_filled_handle = xSemaphoreCreateBinary(); //create semaphore

    if(name_queue_filled_handle == NULL || data_queue_filled_handle == NULL)//check create semaphore
    {
        uart0_puts("_ERROR_: initPlayCmd() failed to create semaphores.\n");
        return false;
    }
    if(!addSharedObject("name_queue_filled", name_queue_filled_handle) || !addSharedObject("data_queue_filled", data_queue_filled_handle)) //try to and check share semaphore
    {
        uart0_puts("_ERROR_: initPlayCmd() failed to create shared objects for semaphores.\n");
        return false;
    }

    return true;
}

bool reader::run(void *p)
{
    name_queue_handle = getSharedObject("name_queue");
    name_queue_filled_handle = getSharedObject("name_queue_filled");
    data_queue_handle = getSharedObject("data_queue");
    data_queue_filled_handle = getSharedObject("data_queue_filled");
    if (name_queue_handle == NULL)
    {
       uart0_puts("_ERROR_: Reader task failed to get handle for name queue.\n");
    }
    if (name_queue_filled_handle == NULL)
    {
      uart0_puts("_ERROR_: Reader task failed to get handle for name queue semaphore.\n");
    }
    if (data_queue_handle == NULL)
    {
       uart0_puts("_ERROR_: Reader task failed to get handle for data queue.\n");
    }

    char song_name[32];
    char prefix[3] = "1:";
    char file[35] = { 0 };              // [35] so that it could handle 32 bytes + 2 bytes for '1:'
    uint8_t data[512] = { 0 };
    uint br;                            // Counts the number of bytes read during a read operation.
    uint32_t file_size;
    if(xSemaphoreTake(name_queue_filled_handle, portMAX_DELAY))
    {
        //Add code here for reader task to attempt to open the file and start to transfer 512 byte chunks to the player task

        //Test code
        if (xQueueReceive(name_queue_handle, &song_name, portMAX_DELAY))
        {
            fflush(stdout); //this only works for printf, not uart0puts
            //vTaskDelay(1); //wait for output buffer to flush, since I don't know how to do that for uart0_puts
            u0_dbg_printf("_SUCCESS_: Got song name \"%s\"\n\n", song_name);

            strcpy(file, prefix);       // file = '1:'
            strcat(file, song_name);    // file = '1:<song_name>'

            fr = f_open(&fil, file, FA_READ);
            file_size = fil.fsize;
            u0_dbg_printf("File name: %s \t File Size: %d \n" , file, file_size);
            if (fr)
                printf("Error opening file. FR: %i\n", (int)fr);
            else {
//                while (f_gets((TCHAR*)data, sizeof(data), &fil)) {
////                    printf(data);
//                    xQueueSend(data_queue_handle, &data, portMAX_DELAY);
////                    xSemaphoreGive(data_queue_filled_handle);
//                }

                f_read(&fil, data, 512, &br);
                do{
                    if(br == 0) //Empty file, break.
                    {
                      break;
                    }
                    else
                    {
                        //Push music into Queue
                        xQueueSend(data_queue_handle, &data, portMAX_DELAY);
                        //Get next block of mp3 data
                        f_read(&fil, data, 512, &br);
                    }
                } while (br != 0);

                /* Close the file */
                f_close(&fil);
            }

        }
        else
        {
            uart0_puts("_ERROR_: Reader task received semaphore, but failed to receive song_name from queue.\n");
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

    if(success == 0)
        return true;
    else
        return false;
}


bool player::run(void *p)
{
    data_queue_handle = getSharedObject("data_queue");
    data_queue_filled_handle = getSharedObject("data_queue_filled");
    if (data_queue_handle == NULL)
    {
       uart0_puts("_ERROR_: Player task failed to get handle for data queue.\n");
    }
    if (data_queue_filled_handle == NULL)
    {
      uart0_puts("_ERROR_: Player task failed to get handle for data queue semaphore.\n");
    }

//    static uint8_t data[512];
    unsigned char data[512];
    unsigned char* bufP = data;

    if(xQueueReceive(data_queue_handle, &data, portMAX_DELAY))
    {
        // TODO: stream to mp3
//        bufP = &data;
        int success = 0;
//        for(int i=0; (i<512 && success == 0); i+=32){
//            success |= WriteSdiChar(bufP, 32);
//            bufP += 32;
////            printf((const char*)data);
//        }
        play(bufP, 512);
        if(success != 0){
            //error
            uart0_puts("WriteSdiChar failed!\n");
//            printf()

        }

//        uart0_put((const char*) data);
//        uart0_puts((const char*) data);

        //Test code
//        if (xSemaphoreTake(data_queue_filled_handle, portMAX_DELAY))
//        {
//            fflush(stdout); //this only works for printf, not uart0puts
//            //vTaskDelay(1); //wait for output buffer to flush, since I don't know how to do that for uart0_puts
//            printf(data);
//        }
//        else
//        {
//            uart0_puts("_ERROR_: Player task received semaphore, but failed to receive data from queue.\n");
//        }

    }
    else
    {
        uart0_puts("_ERROR_: Player task received semaphore, but failed to receive data from queue.\n");
    }
    return true;
}

//--

sineTest::sineTest() :
    scheduler_task("MP3 Sine Test Task", STACK_BYTES(2048), PRIORITY_MEDIUM)
{

}

//bool sineTest::init(void)
//{
//    uint16_t mode = ReadSci(SCI_MODE);
//    mode |= 0x0020;
//    int result = WriteSci(SCI_MODE, mode);
//
//    return ~result;     // WriteSci has reverse logic
//}

bool sineTest::run(void *p)
{
    sineTest_begin();
    vTaskDelay(3000);
    sineTest_end();

    return true;
}
#endif
