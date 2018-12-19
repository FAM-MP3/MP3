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
#include <stdlib.h>
//#include "gpioInterrupts.hpp"
#include "eint.h"
#include "InterruptLab.h"

//static volatile bool pauseSong, pauseToggled; //need to add settingsOpen
//SemaphoreHandle_t volume_down_handle = xSemaphoreCreateBinary();
//static uint8_t volume = 20;
/*
volatile SemaphoreHandle_t pause_toggle_handle = NULL;
volatile SemaphoreHandle_t pause_player_handle = NULL;
volatile SemaphoreHandle_t play_player_handle = NULL;
*/

volatile SemaphoreHandle_t pause_reader_semaphore_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t volume_down_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t volume_up_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t bass_down_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t bass_up_handle = xSemaphoreCreateBinary();
//volatile SemaphoreHandle_t treble_down_handle = xSemaphoreCreateBinary();
//volatile SemaphoreHandle_t treble_up_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t button_ready_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t scroll_down_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t scroll_up_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t lcd_new_song_handle = xSemaphoreCreateBinary(); // either semaphoregive this or pause_reader_semaphore_handle
volatile SemaphoreHandle_t volume_changed_handle = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t bass_changed_handle = xSemaphoreCreateBinary();


static volatile bool pauseButtonReady = true;
static volatile bool selectButtonReady = true;
static volatile bool volumeUpButtonReady = true;
static volatile bool volumeDownButtonReady = true;
static volatile bool scrollUpButtonReady = true;
static volatile bool scrollDownButtonReady = true;
static volatile bool buttonReady = true;
static volatile bool songPaused = false;
static volatile bool bassDownButtonReady = true;
static volatile bool bassUpButtonReady = true;

static bool (LCDTask::*line_ptr_arr[])() = {&LCDTask::LCDLine1, &LCDTask::LCDLine2, &LCDTask::LCDLine3, &LCDTask::LCDLine4};
int numSongs = 0;
char songNames[100][_MAX_LFN];
char fileNames[100][_MAX_LFN];
char playingSong[_MAX_LFN];
char tempBuff[_MAX_LFN];

int songName_pos = 0;                           // song name's position
int line_pos = 0;                               // lcd line's position
int fileName_pos = 0;                           // file name's position
bool toggleDirection = false;                   // false = down, true = up
bool selectFlag = false;                        // -------------------- MIGHT NOT NEED AFTER SEMAPHORE CONVERTION
volatile bool highlighted_has_star = false;              // true when the current song highlighted in LCD is playing
volatile int settingsPage = 0;                           // 0 = song list, 1 = volume, 2 = bass/treble
char savedScreenLine[4][_MAX_LFN];               // buffer to save song list when switching settings

char star[] = {"*"};
char space[] = {" "};
char selectLine1[] = {" ___Your songs___"};
char volumeScreen[] = {" __Volume Settings__"};
char bassTrebleScreen[] = {"Bass/Treble Settings"};
char test[] = {"test"};
int line_offset = 0;

static LCDTask lcd;
//static LabUART uart1;
LabGPIO settingsButton(0,30);            // for scrolling through settings pages

LabGpioInterrupts gpio_interrupt;

static volatile uint8_t volume = 40;
static volatile uint8_t bass = 2;

void Eint3Handler(void)
{
    gpio_interrupt.HandleInterrupt();
}

//static volatile bool pauseSong = false;

//static volatile bool pauseToggled; //need to add settingsOpen

//uint mask = 0x7;
//uint i;

void Port0Pin29ISR(void) //yellow
{
    taskENTER_CRITICAL();
    if(pauseButtonReady && buttonReady && selectButtonReady)
    {


        buttonReady = false;
        u0_dbg_printf("*");//debug for bouncing
        long yield = 0;
        if(highlighted_has_star) //toggle pause for current song
        {
            pauseButtonReady = false;
            xSemaphoreGiveFromISR(pause_reader_semaphore_handle, &yield);
        }
        else //play new song
        {
            selectButtonReady = false;
            xSemaphoreGiveFromISR(lcd_new_song_handle, &yield);
            if(songPaused)
            {
                pauseButtonReady = false;
                xSemaphoreGiveFromISR(pause_reader_semaphore_handle, &yield);
            }
        }
        xSemaphoreGiveFromISR(button_ready_handle, &yield);
        portYIELD_FROM_ISR(yield);
    }
    taskEXIT_CRITICAL();
}

void Port0Pin0ISR(void)
{
    taskENTER_CRITICAL();
    if(volumeDownButtonReady && buttonReady && scrollDownButtonReady && bassDownButtonReady)
    {

        buttonReady = false;
        u0_dbg_printf("v");//debug for bouncing
        long yield = 0;
        //settingsPage = 0 => song list, 1 => volume, 2 => bass
        if(settingsPage == 2)
        {
            bassDownButtonReady = false;
            xSemaphoreGiveFromISR(bass_down_handle, &yield);
        }
        else if(settingsPage == 1)
        {
            volumeDownButtonReady = false;
            xSemaphoreGiveFromISR(volume_down_handle, &yield);

        }
        else if(settingsPage == 0)
        {
            scrollDownButtonReady = false;
            xSemaphoreGiveFromISR(scroll_down_handle, &yield);
        }
        else
            u0_dbg_printf("ERROR Port0Pin0ISR!\n");
        xSemaphoreGiveFromISR(button_ready_handle, &yield);
        portYIELD_FROM_ISR(yield);
    }
    taskEXIT_CRITICAL();
}

void Port0Pin1ISR(void)
{
    taskENTER_CRITICAL();
    if(volumeUpButtonReady && buttonReady && scrollUpButtonReady && bassUpButtonReady)
    {
        buttonReady = false;
        u0_dbg_printf("^");//debug for bouncing
        long yield = 0;

        //settingsPage = 0 => song list, 1 => volume, 2 => bass
        if(settingsPage == 2)
        {
            bassUpButtonReady = false;
            xSemaphoreGiveFromISR(bass_up_handle, &yield);
        }
        else if(settingsPage == 1)
        {
            volumeUpButtonReady = false;
            xSemaphoreGiveFromISR(volume_up_handle, &yield);
        }
        else if(settingsPage == 0)
        {
            scrollUpButtonReady = false;
            xSemaphoreGiveFromISR(scroll_up_handle, &yield);
        }
        else
            u0_dbg_printf("ERROR Port0Pin1ISR!\n");
        xSemaphoreGiveFromISR(button_ready_handle, &yield);
        portYIELD_FROM_ISR(yield);
    }
    taskEXIT_CRITICAL();
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
        u0_dbg_printf("received song! %s\n", song_name);
//        tookSemaphore = false;
        offset = 0; //reset offset
//        strcpy(file, prefix);       // file = '1:'
//        strcat(file, song_name);    // file = '1:<song_name>'
        strcpy(file, song_name);       // file = '1:'
        u0_dbg_printf("file: %s\n", file);
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
        vTaskDelay(10);

       if(!volumeDownButtonReady)
       {
           while(!volumeDownButtonReady)
           {
               vTaskDelay(10);
           }
       }
       else if(!volumeUpButtonReady)
       {
           while(!volumeUpButtonReady)
           {
               vTaskDelay(10);
           }
       }
       if(!bassDownButtonReady)
       {
           while(!bassDownButtonReady)
           {
               vTaskDelay(10);
           }
       }
       else if(!bassUpButtonReady)
       {
           while(!bassUpButtonReady)
           {
               vTaskDelay(10);
           }
       }
       else if(xSemaphoreTake(name_queue_filled_handle, 0))//If we get a semaphore for a new song while a song is currently playing
       {
            tookSongQSemaphore = true; //we don't need to take the semaphore again
           offset = filesize; //stop playing this song
           vTaskDelay(2);

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
            pauseButtonReady = true;
            songPaused = true;
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
            pauseButtonReady = true;
            songPaused = false;
            taskEXIT_CRITICAL();
       }
    }
    vTaskDelay(50);
//    vTaskDelay(100);
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

//    if(!addSharedObject("volume", &volume))
//    {
//        return false;
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
        vTaskDelay(100);
    }

    if(xSemaphoreTake(pause_player_handle, 0) == pdTRUE)//if we get a request to pause the song
    {
        xSemaphoreTake(pause_player_handle, portMAX_DELAY); //wait until unpaused
    }
    else if((xSemaphoreTake(volume_down_handle, 0) == pdTRUE))//if we get a request to lower volume
    {
//        u0_dbg_printf("\nvol down start\n");//debug for bouncing
        vTaskDelay(300 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        volume = ((volume + 15 + 100) > 0x162)?0xfe:(volume + 15);
        setVolume(volume, volume);

        u0_dbg_printf("\nvol is %d\n", volume);//debug for bouncing
  //      vTaskDelay(100 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        taskENTER_CRITICAL();
        xSemaphoreGive(volume_changed_handle);
        taskEXIT_CRITICAL();

    }
    else if((xSemaphoreTake(volume_up_handle, 0) == pdTRUE))//if we get a request to increase volume
    {
 //       u0_dbg_printf("\nvol up start\n");//debug for bouncing
        vTaskDelay(300 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        volume = ((volume - 15 + 100) < 0x64)?0x00:(volume-15);
        setVolume(volume, volume);

        u0_dbg_printf("\nvol is %d\n", volume);
  //      u0_dbg_printf("\nvol up end\n");//debug for bouncing
  //      vTaskDelay(100 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        taskENTER_CRITICAL();

        xSemaphoreGive(volume_changed_handle);
        taskEXIT_CRITICAL();

    }
    else if((xSemaphoreTake(bass_down_handle, 0) == pdTRUE))//if we get a request to increase bass
    {
        vTaskDelay(300 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        bass = ((bass - 1) < 2)?2:(bass - 1);
        setBass(bass);

        u0_dbg_printf("\nbass is %d\n", bass);//debug for bouncing
  //      vTaskDelay(100 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        taskENTER_CRITICAL();
        xSemaphoreGive(bass_changed_handle);
        taskEXIT_CRITICAL();
    }
    else if((xSemaphoreTake(bass_up_handle, 0) == pdTRUE))//if we get a request to increase bass
    {
        vTaskDelay(300 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        bass = ((bass + 1) > 15)?15:(bass + 1);
        setBass(bass);

        u0_dbg_printf("\nbass is %d\n", bass);//debug for bouncing
  //      vTaskDelay(100 / portTICK_PERIOD_MS); //wait 1 second for button bounce
        taskENTER_CRITICAL();
        xSemaphoreGive(bass_changed_handle);
        taskEXIT_CRITICAL();
    }
    else if(xQueueReceive(data_queue_handle, &data, 5000))//if we can get song data from the queue
    {
        bufP = &data[0];
        play(bufP, 512);
    }
    else
    {
//        uart0_puts("WARNING: Player task waited more than 5 seconds without receiving data from queue.\n");
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
        eint3_enable_port0(0, eint_rising_edge, Port0Pin0ISR); //volume up
        eint3_enable_port0(1, eint_rising_edge, Port0Pin1ISR); //volume down
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
    if(xSemaphoreTake(button_ready_handle, 0)) //if we get a semaphore
       {
//        static volatile bool pauseButtonReady = true;
//        static volatile bool selectButtonReady = true;
//        static volatile bool volumeUpButtonReady = true;
//        static volatile bool volumeDownButtonReady = true;
//        static volatile bool scrollUpButtonReady = true;
//        static volatile bool scrollDownButtonReady = true;
//        static volatile bool buttonReady = true;
//        static volatile bool songPaused = false;

            while(!(pauseButtonReady && volumeUpButtonReady && volumeDownButtonReady && selectButtonReady && scrollUpButtonReady && scrollDownButtonReady && bassUpButtonReady && bassDownButtonReady))
            {
                vTaskDelay(2);
            }
            vTaskDelay(700 / portTICK_PERIOD_MS); //wait 1 second for button bounce
            //u0_dbg_printf("button is ready\n");
            taskENTER_CRITICAL();
            buttonReady = true;
            taskEXIT_CRITICAL();
       }
    vTaskDelay(300);
    return true;
}

LCD_UI::LCD_UI() :
            scheduler_task("LCD_UI Task", STACK_BYTES(4096), PRIORITY_HIGH)
{

}

bool LCD_UI::init(void)
{
    // THIS NEEDS TO RUN BEFORE ANYTHING ELSE
//        uart1.Initialize(LabUART::Uart2, 9600);
//        lcd.LCDSetBaudRate();
//        lcd.LCDTurnDisplayOn();
//        lcd.LCDBackLightON();
//     //   lcd.LCDSetSplashScreen();
//     //   sendLCDData(splashScreen);
//        lcd.LCD20char();
//        lcd.LCD4Lines();
//        lcd.LCDCursorPosition();
//        lcd.LCDClearDisplay();

        settingsButton.setAsInput();

        // Initializes LCD song buffer
        DIR Dir;
        FILINFO Finfo;
        FRESULT returnCode;
        const char dirPath[] = "1:";
//        int count = 0;
        char *filename;
        char songNamesBuff [_MAX_LFN] = {0};
        char fileNamesBuff [_MAX_LFN] = {0};
        char small_buff[20];

        #if _USE_LFN
            char Lfname[_MAX_LFN];
        #endif

        #if _USE_LFN
            Finfo.lfname = Lfname;
            Finfo.lfsize = sizeof(Lfname);
        #endif

        if (FR_OK != (returnCode = f_opendir(&Dir, dirPath))) {
            u0_dbg_printf("Invalid directory: |%s| (Error %i)\n", dirPath, returnCode);
            return false;
        }

        for (; ;)
        {
            returnCode = f_readdir(&Dir, &Finfo);
            if ( (FR_OK != returnCode) || !Finfo.fname[0]) {
                break;
            }
        #if _USE_LFN
            if(Finfo.lfname[strlen(Finfo.lfname)-1] == '3')
            {
                // store the file names as "1:<song name.mp3>" to the fileNames array
                sprintf(fileNamesBuff, "1:%s", Finfo.lfname);
                strncpy(fileNames[numSongs], fileNamesBuff, sizeof(songNamesBuff));

                filename = strtok(Finfo.lfname, ".");

                // put a space in front of the song name and store it to the songNames array
                sprintf(songNamesBuff, " %s", filename);

                // if song name is more than 20 characters, truncate
                if(strlen(songNamesBuff) > 20)
                {
                    // trunctate songNamesBuff so that it's only 17 characters long
                    songNamesBuff[17] = '\0';
                    sprintf(small_buff, "%s...", songNamesBuff);
                    // save the truncated song name into songNames array
                    strncpy(songNames[numSongs], small_buff, sizeof(small_buff));
                }
                else
                    strncpy(songNames[numSongs], songNamesBuff, sizeof(songNamesBuff));

                numSongs++; // increments the number of songs in the fileNames and songNames buffers
//                count++;
//                if(count == 10)
//                    count = 0;
            }
        #endif
        }
        // End initializing LCD song buffer

        (lcd.*line_ptr_arr[0])();
        sendLCDData(selectLine1);
        //    saveScreenLine[0] = selectLine1;
        sprintf(tempBuff, "%s", selectLine1);
        strncpy(savedScreenLine[0], tempBuff, sizeof(tempBuff));

        // write the next 3 lines
        for(int i=0; i<3; i++)
        {
            songName_pos = i;
            (lcd.*line_ptr_arr[i+1])();
            sendLCDData(songNames[songName_pos]);

        //        saveScreenLine[i+1] = songNames[songName_pos+i];
            sprintf(tempBuff, "%s", songNames[songName_pos]);
            strncpy(savedScreenLine[i+1], tempBuff, sizeof(tempBuff));
        }
//        lcd.LCDClearDisplay();
        return true;
}

bool LCD_UI::run(void *p)
{
    // TODO: convert to semaphores
//    if(settingsPage == 0)
//    {
//        if(xSemaphoreTake(scroll_down_handle, 0))
//    if(settingsButton.getLevel())
//    {
////        sendLCDData(test);
////        lcd.LCDClearDisplay();
//        (lcd.*line_ptr_arr[0])();
//        sendLCDData(test);
//        vTaskDelay(100);
//    }

//    if(settingsPage == 0)
//    {
        if(xSemaphoreTake(scroll_down_handle, 0))
        {

            selectFlag = false;
            highlighted_has_star = false;
            line_pos++;

            if(line_pos == 4)
            {
                line_pos = 0;
                lcd.LCDClearDisplay();

                if(toggleDirection)
                {
                    for(int j=0; j<4; j++) // point to the last line's songName_pos
                    {
                        songName_pos++;
                        if(songName_pos == numSongs+1)
                            songName_pos = 0;
                    }
//                        fileName_pos++;
//                        if(fileName_pos == 10)
//                            fileName_pos = 0;
//                            u0_dbg_printf("songName_pos: %d\n", songName_pos);
                }

                for(int i=0; i<4; i++)
                {
                    songName_pos++;
                    if(songName_pos == numSongs+1)
                        songName_pos = 0;
//                        u0_dbg_printf("songName_pos = %d\n", songName_pos);
//                        u0_dbg_printf("line # %d, song_pos = %d songName = %s\n", i, songName_pos, songNames[songName_pos]);
                    (lcd.*line_ptr_arr[i])();
                    sendLCDData(songNames[songName_pos]);

                    // save lines to buffer in case of settings switch
                    sprintf(tempBuff, "%s", songNames[songName_pos]);
                    strncpy(savedScreenLine[i], tempBuff, sizeof(tempBuff));

//                        u0_dbg_printf("writing %s\n", songNames[songName_pos]);
                    lcd.LCDSetCursor(i+1, 0);
//                        u0_dbg_printf("song %s, current %s\n", songNames[songName_pos], playingSong);
//                        if(songNames[songName_pos] == playingSong)
                    if((strcmp(songNames[songName_pos], playingSong) == 0) && (playingSong != NULL))
                    {
//                            u0_dbg_printf("similar song %s %s\n", songNames[songName_pos], playingSong);
                        sendLCDData(star);
                        lcd.LCDSetCursor(i+1, 0);
                    }

//                            u0_dbg_printf("songName_pos: %d\n", songName_pos);
                }
            }
            (lcd.*line_ptr_arr[line_pos])();
//                lcd.LCDLine2();
            lcd.LCDBlinkCursor();

            if(toggleDirection)
            {
                fileName_pos++;
                if(fileName_pos == numSongs+1)
                    fileName_pos = 0;
                line_offset = 1;
            }

            u0_dbg_printf("file name: %s playing song: %s\n", songNames[fileName_pos], playingSong);
            if(strcmp(songNames[fileName_pos], playingSong) == 0)
            {
                u0_dbg_printf("Highlighted song %s %s\n", songNames[fileName_pos], playingSong);
                highlighted_has_star = true;
            }


            fileName_pos++;
            if(fileName_pos == numSongs+1)
                fileName_pos = 0;
//                line_pos++;
            toggleDirection = false;
            scrollDownButtonReady = true;
            line_offset = 1;
        }

        if(xSemaphoreTake(scroll_up_handle, 0))
        {
//            u0_dbg_printf("song[numSongs]: %s\n", songNames[numSongs-1]);
            selectFlag = false;
            highlighted_has_star = false;
            line_pos--;

            if(line_pos == -1) // if you end up in the previous page
            {
                line_pos = 3;
                lcd.LCDClearDisplay();

                // point to the first line's songName_pos before decrementing
                if(!toggleDirection)
                {
                    for(int j=0; j<4; j++)
                    {
                        songName_pos--;
                        if(songName_pos == -1)
                            songName_pos = numSongs-1;
//                                u0_dbg_printf("songName_pos: %d\n", songName_pos);
                    }
//                        fileName_pos--;
//                        if(fileName_pos == -1)
//                            fileName_pos = 9;
                }

                for(int i=3; i>-1; i--)
                {
//                        u0_dbg_printf("line # %d, song_pos = %d songName = %s\n", i, songName_pos, songNames[songName_pos]);
                    (lcd.*line_ptr_arr[i])();
                    sendLCDData(songNames[songName_pos]);
//                        u0_dbg_printf("writing %s\n", songNames[songName_pos]);
                    lcd.LCDSetCursor(i+1, 0);
//                        u0_dbg_printf("song %s, current %s\n", songNames[songName_pos], playingSong);
//                        if(songNames[songName_pos] == playingSong)

                    // save lines to buffer in case of settings switch
                    sprintf(tempBuff, "%s", songNames[songName_pos]);
                    strncpy(savedScreenLine[i], tempBuff, sizeof(tempBuff));

                    if((strcmp(songNames[songName_pos], playingSong) == 0) && (playingSong != NULL))
                    {
//                            u0_dbg_printf("similar song %s %s\n", songNames[songName_pos], playingSong);
                        sendLCDData(star);
                        lcd.LCDSetCursor(i+1, 0);
                    }

                    songName_pos--;
                    if(songName_pos == -1)
                        songName_pos = numSongs-1;
//                            u0_dbg_printf("songName_pos: %d\n", songName_pos);
                }
            }
            (lcd.*line_ptr_arr[line_pos])();
//                lcd.LCDLine2();
            lcd.LCDBlinkCursor();

            fileName_pos--;
            if(fileName_pos == -1)
                fileName_pos = numSongs-1;
//                line_pos++;
            if(!toggleDirection)
            {
                fileName_pos--;
                if(fileName_pos == -1)
                    fileName_pos = numSongs-1;
                line_offset = 0;
            }

//                u0_dbg_printf("file name: %s\n", songNames[fileName_pos-1]);
            u0_dbg_printf("file name: %s playing song: %s\n", songNames[fileName_pos], playingSong);
            if(strcmp(songNames[fileName_pos], playingSong) == 0)
            {
                u0_dbg_printf("Highlighted song %s %s\n", songNames[fileName_pos], playingSong);
                highlighted_has_star = true;
            }

            toggleDirection = true;
            scrollUpButtonReady = true;
        }
//    }


//    }

    vTaskDelay(300);
    return true;
}

LCD_Select::LCD_Select() :
        scheduler_task("LCD_Select Task", STACK_BYTES(4096), PRIORITY_HIGH)
{

}

bool LCD_Select::init(void)
{
//    // THIS NEEDS TO RUN BEFORE ANYTHING ELSE
//    uart1.Initialize(LabUART::Uart2, 9600);
//    lcd.LCDSetBaudRate();
//    lcd.LCDTurnDisplayOn();
//    lcd.LCDBackLightON();
// //   lcd.LCDSetSplashScreen();
// //   sendLCDData(splashScreen);
//    lcd.LCD20char();
//    lcd.LCD4Lines();
//    lcd.LCDCursorPosition();
//    lcd.LCDClearDisplay();
//
//    settingsButton.setAsInput();
    return true;
}

bool LCD_Select::run(void *p)
{
    //TODO: convert to semaphore
    if(xSemaphoreTake(lcd_new_song_handle, portMAX_DELAY))
    {
       selectFlag = true;
       if(selectFlag)
       {
//           ClearStar();
           // clear the *
           for(int i=0; i<4; i++)
           {
               (lcd.*line_ptr_arr[i])();
               lcd.LCDSetCursor(i+1, 0);
               sendLCDData(space);
               lcd.LCDSetCursor(i+1, 0);
           }

           //write * to current playing song
           (lcd.*line_ptr_arr[line_pos])();
           sendLCDData(star);
           sprintf(tempBuff, "%s", songNames[fileName_pos-line_offset]);
           strncpy(playingSong, tempBuff, sizeof(tempBuff));
           u0_dbg_printf("playing song: %s\n", playingSong);
           highlighted_has_star = true;
    //                lcd.LCDBlinkCursor();
           lcd.LCDSetCursor(line_pos+1, 0);
           selectFlag = false;

           // send to queue
           name_queue_handle = scheduler_task::getSharedObject("name_queue");
           name_queue_filled_handle = scheduler_task::getSharedObject("name_queue_filled");
           if(name_queue_handle == NULL)
           {
               uart0_puts("\n_ERROR_: vLCDSelectButton() failed to get handle for name queue.\n");
           }
           if(name_queue_filled_handle == NULL)
           {
               uart0_puts("\n_ERROR_: vLCDSelectButton() failed to get handle for semaphore.\n");
           }

           if((name_queue_handle != NULL) && (name_queue_filled_handle != NULL))
           {
               xQueueReset( name_queue_handle ); //clear the queue before we fill it again
               xQueueSend(name_queue_handle, &fileNames[fileName_pos-line_offset], 3000);
               xSemaphoreGive(name_queue_filled_handle); //signal request to play song
           }
       }
       selectButtonReady = true;
    }
//    vTaskDelay(100);
    return true;
}

LCD_Settings::LCD_Settings() :
        scheduler_task("LCD_Settings Task", STACK_BYTES(4096), PRIORITY_HIGH)
{

}

bool LCD_Settings::init(void)
{
//    // THIS NEEDS TO RUN BEFORE ANYTHING ELSE
//    uart1.Initialize(LabUART::Uart2, 9600);
//    lcd.LCDSetBaudRate();
//    lcd.LCDTurnDisplayOn();
//    lcd.LCDBackLightON();
// //   lcd.LCDSetSplashScreen();
// //   sendLCDData(splashScreen);
//    lcd.LCD20char();
//    lcd.LCD4Lines();
//    lcd.LCDCursorPosition();
//    lcd.LCDClearDisplay();
//
//    settingsButton.setAsInput();
//
//    // Initializes LCD song buffer
//    DIR Dir;
//    FILINFO Finfo;
//    FRESULT returnCode;
//    const char dirPath[] = "1:";
//    int count = 0;
//    char *filename;
//    char songNamesBuff [_MAX_LFN] = {0};
//    char fileNamesBuff [_MAX_LFN] = {0};
//    char small_buff[20];
//
//    #if _USE_LFN
//        char Lfname[_MAX_LFN];
//    #endif
//
//    #if _USE_LFN
//        Finfo.lfname = Lfname;
//        Finfo.lfsize = sizeof(Lfname);
//    #endif
//
//    if (FR_OK != (returnCode = f_opendir(&Dir, dirPath))) {
//        u0_dbg_printf("Invalid directory: |%s| (Error %i)\n", dirPath, returnCode);
//        return false;
//    }
//
//    for (; ;)
//    {
//        returnCode = f_readdir(&Dir, &Finfo);
//        if ( (FR_OK != returnCode) || !Finfo.fname[0]) {
//            break;
//        }
//    #if _USE_LFN
//        if(Finfo.lfname[strlen(Finfo.lfname)-1] == '3')
//        {
//            // store the file names as "1:<song name.mp3>" to the fileNames array
//            sprintf(fileNamesBuff, "1:%s", Finfo.lfname);
//            strncpy(fileNames[count], fileNamesBuff, sizeof(songNamesBuff));
//
//            filename = strtok(Finfo.lfname, ".");
//
//            // put a space in front of the song name and store it to the songNames array
//            sprintf(songNamesBuff, " %s", filename);
//
//            // if song name is more than 20 characters, truncate
//            if(strlen(songNamesBuff) > 20)
//            {
//                // trunctate songNamesBuff so that it's only 17 characters long
//                songNamesBuff[17] = '\0';
//                sprintf(small_buff, "%s...", songNamesBuff);
//                // save the truncated song name into songNames array
//                strncpy(songNames[count], small_buff, sizeof(small_buff));
//            }
//            else
//                strncpy(songNames[count], songNamesBuff, sizeof(songNamesBuff));
//
//            count++;
//            if(count == 10)
//                count = 0;
//        }
//    #endif
//    }
//    // End initializing LCD song buffer
    return true;
}

bool LCD_Settings::run(void *p)
{

    // TODO: convert to semaphore take
    if(settingsButton.getLevel())
    {
        settingsPage++;
        if(settingsPage == 3)
            settingsPage = 0;

//        u0_dbg_printf("settingsPage: %d\n", settingsPage);
        if(settingsPage == 0)
        {
            if(changedSettings)
            {
                lcd.LCDClearDisplay();
                for(int i=0; i<4; i++)
                {
                    (lcd.*line_ptr_arr[i])();
                    sendLCDData(savedScreenLine[i]);
                    lcd.LCDSetCursor(i+1, 0);
                    if(strcmp(savedScreenLine[i], playingSong) == 0)
                    {
                        sendLCDData(star);
                        lcd.LCDSetCursor(i+1, 0);
                    }
                }
                lcd.LCDSetCursor(1, 0);
                changedSettings = false;
            }
        }
        if(settingsPage == 1)
        {
            changedSettings = true;
            lcd.LCDClearDisplay();
            sendLCDData(volumeScreen);
            (lcd.*line_ptr_arr[2])();                   // go to line 3
            percentage = (100 - ConvertVolume(volume));
            sprintf(vol, "       %.2f%%", percentage);
//                vol = ConvertVolume(volume);
            sendLCDData(vol);
            (lcd.*line_ptr_arr[line_pos])();

        }
        if(settingsPage == 2)
        {
            changedSettings = true;
            lcd.LCDClearDisplay();
            sendLCDData(bassTrebleScreen);
            (lcd.*line_ptr_arr[2])();                   // go to line 3
            bass_percentage = (ConvertBass(bass));
            sprintf(bass_line, "       %.2f%%", bass_percentage);
            sendLCDData(bass_line);
            (lcd.*line_ptr_arr[line_pos])();
        }

//            if(settingsPage == 0)
//            {
//                char selectLine1[] = {" ___Your songs___"};
//                (lcd.*line_ptr_arr[0])();
//                sendLCDData(selectLine1);
//
//                (lcd.*line_ptr_arr[1])();
//            //    char temp[20];
//            //    temp = songNames[songName_pos];
//                sendLCDData(songNames[songName_pos]);
//
//                (lcd.*line_ptr_arr[2])();
//                sendLCDData(songNames[++songName_pos]);
//
//                (lcd.*line_ptr_arr[3])();
//                sendLCDData(songNames[++songName_pos]);
//            }
    }

        if(xSemaphoreTake(volume_changed_handle, 0))
        {
            (lcd.*line_ptr_arr[2])();                   // go to line 3
            percentage = (100.0 - ConvertVolume(volume));
            sprintf(vol, "       %.2f%%", percentage);
            u0_dbg_printf("percentage: %s\n", vol);
//                vol = ConvertVolume(volume);
            sendLCDData(vol);
            (lcd.*line_ptr_arr[line_pos])();
            taskENTER_CRITICAL();
            volumeUpButtonReady = true;
            volumeDownButtonReady = true;
            taskEXIT_CRITICAL();
        }

        if(xSemaphoreTake(bass_changed_handle, 0))
        {
            (lcd.*line_ptr_arr[2])();                   // go to line 3
            bass_percentage = (ConvertBass(bass));
            sprintf(bass_line, "       %.2f%%", bass_percentage);
            sendLCDData(bass_line);
            (lcd.*line_ptr_arr[line_pos])();
            taskENTER_CRITICAL();
            bassUpButtonReady = true;
            bassDownButtonReady = true;
            taskEXIT_CRITICAL();
        }


    vTaskDelay(100);
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

