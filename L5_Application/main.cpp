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

//static int numSongs = 0;
//static LCDTask lcd;
//static LabUART uart1;

#if 0
bool InitLCD();
void ClearStar();
double ConvertVolume(uint8_t vol);

//LCDTask lcd;
//LabUART uart1;
//LabGPIO settingsButton(0,0);            // for scrolling through settings pages
//LabGPIO playPauseButton(0,1);           // for pause/play song
//LabGPIO upButton(0,29);                 // for scrolling up
//LabGPIO downButton(0,30);               // for scrolling down

//LabGPIO settingsButton(0,30);            // for scrolling through settings pages
//LabGPIO playPauseButton(0,29);           // for pause/play song
//LabGPIO upButton(0,1);                  // for scrolling up
//LabGPIO downButton(0,0);                  // for scrolling down

//bool (LCDTask::*line_ptr_arr[])() = {&LCDTask::LCDLine1, &LCDTask::LCDLine2, &LCDTask::LCDLine3, &LCDTask::LCDLine4};
////char *songNames[10];
//char songNames[10][_MAX_LFN];
//char fileNames[10][_MAX_LFN];
//char playingSong[_MAX_LFN];
//char tempBuff[_MAX_LFN];
//
//int songName_pos = 0;                           // song name's position
//int line_pos = 0;                               // lcd line's position
//int fileName_pos = 0;                           // file name's position
//bool toggleDirection = false;                   // false = down, true = up
//bool selectFlag = false;                        //
//bool highlighted_has_star = false;              // true when the current song highlighted in LCD is playing
//char star[] = {"*"};
//char space[] = {" "};
//int settingsPage = 0;                           // 0 = song list, 1 = volume, 2 = bass/treble
//char savedScreenLine[4][_MAX_LFN];               // buffer to save song list when switching settings
//bool changedSettings = false;                   // true when settingsButton is toggled

QueueHandle_t name_queue_handle;
SemaphoreHandle_t name_queue_filled_handle;

//void sendLCDData(char data[]){
//    char check = '\0';
//    int counter = 0;
//    while(data[counter] != check){
//        uart1.Transmit(data[counter]);
//        counter++;
//        delay_ms(1);
//    }
//}

void vLCDUI(void *p)
{
    char selectLine1[] = {" ___Your songs___"};
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
//    u0_dbg_printf("songName_pos: %d\n", songName_pos);
//
//    (lcd.*line_ptr_arr[1])();
////    char temp[20];
////    temp = songNames[songName_pos];
//    sendLCDData(songNames[songName_pos]);
//    saveScreenLine[1] = songNames[songName_pos];
//
//    (lcd.*line_ptr_arr[2])();
//    sendLCDData(songNames[++songName_pos]);
//
//    ++songName_pos;
//    (lcd.*line_ptr_arr[3])();
//    sendLCDData(songNames[songName_pos]);

//    int cursor_pos = 1;
        while(1)
        {
            if(settingsPage == 0)
            {
                if(downButton.getLevel())
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
                                if(songName_pos == 10)
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
                            if(songName_pos == 10)
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
                            if(strcmp(songNames[songName_pos], playingSong) == 0)
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
                        if(fileName_pos == 10)
                            fileName_pos = 0;
                    }

                    u0_dbg_printf("file name: %s playing song: %s\n", songNames[fileName_pos], playingSong);
                    if(strcmp(songNames[fileName_pos], playingSong) == 0)
                    {
                        u0_dbg_printf("Highlighted song %s %s\n", songNames[fileName_pos], playingSong);
                        highlighted_has_star = true;
                    }


                    fileName_pos++;
                    if(fileName_pos == 10)
                        fileName_pos = 0;
    //                line_pos++;
                    toggleDirection = false;
                }

                // BUG: highlight is true for the previous line
                // BUG: song names arent in sync when going up/down
                // BUG: lcd not displaying correct songs, but pos is correct
                if(upButton.getLevel())
                {
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
                                    songName_pos = 9;
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

                            if(strcmp(songNames[songName_pos], playingSong) == 0)
                            {
    //                            u0_dbg_printf("similar song %s %s\n", songNames[songName_pos], playingSong);
                                sendLCDData(star);
                                lcd.LCDSetCursor(i+1, 0);
                            }

                            songName_pos--;
                            if(songName_pos == -1)
                                songName_pos = 9;
//                            u0_dbg_printf("songName_pos: %d\n", songName_pos);
                        }
                    }
                    (lcd.*line_ptr_arr[line_pos])();
    //                lcd.LCDLine2();
                    lcd.LCDBlinkCursor();

                    fileName_pos--;
                    if(fileName_pos == -1)
                        fileName_pos = 9;
    //                line_pos++;
                    if(!toggleDirection)
                    {
                        fileName_pos--;
                        if(fileName_pos == -1)
                            fileName_pos = 9;
                    }

    //                u0_dbg_printf("file name: %s\n", songNames[fileName_pos-1]);
                    u0_dbg_printf("file name: %s playing song: %s\n", songNames[fileName_pos], playingSong);
                    if(strcmp(songNames[fileName_pos], playingSong) == 0)
                    {
                        u0_dbg_printf("Highlighted song %s %s\n", songNames[fileName_pos], playingSong);
                        highlighted_has_star = true;
                    }

                    toggleDirection = true;
                }
            }

            vTaskDelay(1);
        }
}
void vLCDSelectButton(void *p)
{
//    char select[] = {"*"};
//    char space[] = {" "};

    while(1)
    {
            //Selecting song button
            if(playPauseButton.getLevel())
            {
                selectFlag = true;
                if(selectFlag)
                {
                    ClearStar();

                    //write * to current playing song
                    (lcd.*line_ptr_arr[line_pos])();
                    sendLCDData(star);
                    sprintf(tempBuff, "%s", songNames[fileName_pos-1]);
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
                        xQueueSend(name_queue_handle, &fileNames[fileName_pos-1], 3000);
                        xSemaphoreGive(name_queue_filled_handle); //signal request to play song
                    }
                }
            }
            vTaskDelay(100);
    }
}
void vLCDSettingsButton(void *p)
{
    char volumeScreen[] = {" __Volume Settings__"};
    char bassTrebleScreen[] = {"Bass/Treble Settings"};
    char vol[20];
    while(1)
    {
        if(settingsButton.getLevel())
        {
            settingsPage++;
            if(settingsPage == 3)
                settingsPage = 0;

            u0_dbg_printf("settingsPage: %d\n", settingsPage);
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
                sprintf(vol, "%.2f%%", ConvertVolume(volume));
//                vol = ConvertVolume(volume);
                sendLCDData(vol);
                (lcd.*line_ptr_arr[line_pos])();
            }
            if(settingsPage == 2)
            {
                changedSettings = true;
                lcd.LCDClearDisplay();
                sendLCDData(bassTrebleScreen);
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
        vTaskDelay(200);
    }
}
#endif

int main(void)
{
    char splashScreen[] = {"Welcome to FAM MP3"};
    /*      INIT functions for decoder      */
    ssp1_init();                            // initialize SPI1
//    ssp1_set_max_clock(12);                 // sets clock to 12MHz
//    printf("%d\n", sys_get_cpu_clock()/4);
    int success = 0;
    success &= InitVS10xx();               // initialize pins for VS1053B

//    success &= VSTestInitSoftware();
//    InitSoftware();
//    bool lcd_success = InitLCD();

//    for(int i=0; i<10; i++)
//    {
//        u0_dbg_printf("%s\n", songNames[i]);
//    }

    // buttons init
//    settingsButton.setAsInput();
//    playPauseButton.setAsInput();
//    upButton.setAsInput();
//    downButton.setAsInput();
//    volume = scheduler_task::getSharedObject("volume");

//   uart1.Initialize(LabUART::Uart2, 9600);
//   lcd.LCDSetBaudRate();
//   lcd.LCDTurnDisplayOn();
//   lcd.LCDBackLightON();
////   lcd.LCDSetSplashScreen();
//   lcd.LCD20char();
////   lcd.LCD4Lines();
////   lcd.LCDCursorPosition();
////   lcd.LCDClearDisplay();
//   sendLCDData(splashScreen);
    bool lcd_success = InitLCD();
//
//   // TODO: set "splash screen"
////   (lcd.*line_ptr_arr[1])();
////   sendLCDData(splashScreen);

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


//    vTaskStartScheduler();

    scheduler_add_task(new terminalTask(PRIORITY_HIGH));

    /* Consumes very little CPU, but need highest priority to handle mesh network ACKs */
    scheduler_add_task(new wirelessTask(PRIORITY_CRITICAL));
        scheduler_add_task(new buttons());

//        scheduler_add_task(new LCD_Settings());
        scheduler_add_task(new LCD_UI());
        scheduler_add_task(new LCD_Select());
        scheduler_add_task(new LCD_Settings());
        scheduler_add_task(new reader());
        scheduler_add_task(new player());
    //    scheduler_add_task(new sineTest());

//    xTaskCreate(vLCDUI, "lcd", 1024, NULL, 2, NULL);
//    xTaskCreate(vLCDSelectButton, "lcd_select", 1024, NULL, 2, NULL);
//    xTaskCreate(vLCDSettingsButton, "lcd_settings", 1024, NULL, 2, NULL);
//    scheduler_add_task(new sineTest());
//    xTaskCreate(vReader, "reader", 1024, NULL, 2, NULL);
//    xTaskCreate(vPlayer, "player", 1024, NULL, 3, NULL);
    scheduler_start(); ///< This shouldn't return

    while(1);
    return 0;
}

//bool InitLCD()
//{
//    DIR Dir;
//    FILINFO Finfo;
//    FRESULT returnCode;
//    const char dirPath[] = "1:";
////    std::string songNames[10];
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
////        #if _USE_LFN
////            Finfo.lfname = Lfname;
////            Finfo.lfsize = sizeof(Lfname);
////        #endif
//
//        returnCode = f_readdir(&Dir, &Finfo);
//        if ( (FR_OK != returnCode) || !Finfo.fname[0]) {
//            break;
//        }
////        u0_dbg_printf("%s\n", &(Finfo.fname));
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
////    for(int i=0;i<10;i++){
////        u0_dbg_printf("name: %s\n", songNames[i]);
////    }
//
//    return true;
//
//}

//void ClearStar()
//{
//    for(int i=0; i<4; i++)
//    {
//        (lcd.*line_ptr_arr[i])();
//        lcd.LCDSetCursor(i+1, 0);
//        sendLCDData(space);
//        lcd.LCDSetCursor(i+1, 0);
//    }
//}

//double ConvertVolume(uint8_t vol)
//{
//    uint16_t v;
//    v = vol;
//    v <<= 8;
//    v |= vol;
//    u0_dbg_printf("volume: %x\n", v);
//    return (v*100/65278);
//}
