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

bool InitLCD();

LCDTask lcd;
LabUART uart1;
LabGPIO switch0(0,1);   // for scrolling through songs
LabGPIO switch1(0,0);   // for selecting song
bool (LCDTask::*line_ptr_arr[])() = {&LCDTask::LCDLine1, &LCDTask::LCDLine2, &LCDTask::LCDLine3, &LCDTask::LCDLine4};
//char *songNames[10];
char songNames[10][_MAX_LFN];
char fileNames[10][_MAX_LFN];
int songName_pos = 0;
int line_pos = 0;
int fileName_pos = 0;
bool selectFlag = false;

QueueHandle_t name_queue_handle;
SemaphoreHandle_t name_queue_filled_handle;

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

void sendLCDData(char data[]){
    char check = '\0';
    int counter = 0;
    while(data[counter] != check){
        uart1.Transmit(data[counter]);
        counter++;
        delay_ms(1);
    }
}
//
//void sendLCDData(volatile char* data){
//    char check = '\0';
//    int counter = 0;
////    u0_dbg_printf("%s\n", data[0]);
//    while(data[counter] != check){
//        uart1.Transmit(data[counter]);
//        counter++;
//        delay_ms(1);
//    }
//}

void vLCDUI(void *p)
{
    char selectLine1[] = {"___Your songs___"};
    (lcd.*line_ptr_arr[0])();
    sendLCDData(selectLine1);

    (lcd.*line_ptr_arr[1])();
//    char temp[20];
//    temp = songNames[songName_pos];
    sendLCDData(songNames[songName_pos]);

    (lcd.*line_ptr_arr[2])();
    sendLCDData(songNames[++songName_pos]);

    (lcd.*line_ptr_arr[3])();
    sendLCDData(songNames[++songName_pos]);

//    int cursor_pos = 1;
        while(1)
        {
            if(switch0.getLevel())
            {
                selectFlag = false;
                line_pos++;

                if(line_pos == 4)
                {
                    line_pos = 0;
                    lcd.LCDClearDisplay();
                    for(int i=0; i<4; i++)
                    {
                        songName_pos++;
                        if(songName_pos == 10)
                            songName_pos = 0;
                        (lcd.*line_ptr_arr[i])();
                        sendLCDData(songNames[songName_pos]);
                        lcd.LCDSet_Cursor(i+1, 0);
                    }
                }
                (lcd.*line_ptr_arr[line_pos])();
//                lcd.LCDLine2();
                lcd.LCDBlinkCursor();

                fileName_pos++;
                if(fileName_pos == 10)
                    fileName_pos = 0;
//                line_pos++;

            }
            vTaskDelay(1);
        }
}
void vLCDSelectButton(void *p)
{
    char select[] = {"*"};
    char space[] = {" "};
    int star_pos=0;

    while(1)
    {
            //Selecting song button
            if(switch1.getLevel())
            {
                selectFlag = true;
                if(selectFlag)
                {
                    (lcd.*line_ptr_arr[star_pos])();
                    sendLCDData(space);
                    lcd.LCDSet_Cursor(star_pos+1, 0);

                    star_pos = line_pos;
                    u0_dbg_printf("selected song: %s\n", fileNames[fileName_pos-1]);
                    // BUG: this writes * to the next line
                    (lcd.*line_ptr_arr[line_pos])();
                    sendLCDData(select);
    //                lcd.LCDBlinkCursor();
                    lcd.LCDSet_Cursor(line_pos+1, 0);
                    selectFlag = false;
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
            vTaskDelay(500);
    }
}

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
    bool lcd_success = InitLCD();

//    for(int i=0; i<10; i++)
//    {
//        u0_dbg_printf("%s\n", songNames[i]);
//    }

   uart1.Initialize(LabUART::Uart2, 9600);
   lcd.LCDSetBaudRate();
   lcd.LCDTurnDisplayOn();
   lcd.LCDBackLightON();
   lcd.LCD20char();
   lcd.LCD4Lines();
   lcd.LCDCursorPosition();
   lcd.LCDClearDisplay();

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
    //    scheduler_add_task(new buttons());
        scheduler_add_task(new reader());
        scheduler_add_task(new player());
    //    scheduler_add_task(new sineTest());

    xTaskCreate(vLCDUI, "lcd", 1024, NULL, 1, NULL);
//    xTaskCreate(vLCD_cursor, "lcd_cursor", 1024, NULL, 1, NULL);
    xTaskCreate(vLCDSelectButton, "lcd_select", 1024, NULL, 1, NULL);


//    scheduler_add_task(new sineTest());
//    xTaskCreate(vReader, "reader", 1024, NULL, 2, NULL);
//    xTaskCreate(vPlayer, "player", 1024, NULL, 3, NULL);
    scheduler_start(); ///< This shouldn't return

    while(1);
    return 0;
}

bool InitLCD()
{
    DIR Dir;
    FILINFO Finfo;
    FRESULT returnCode;
    const char dirPath[] = "1:";
//    std::string songNames[10];
    int count = 0;
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
//        #if _USE_LFN
//            Finfo.lfname = Lfname;
//            Finfo.lfsize = sizeof(Lfname);
//        #endif

        returnCode = f_readdir(&Dir, &Finfo);
        if ( (FR_OK != returnCode) || !Finfo.fname[0]) {
            break;
        }
//        u0_dbg_printf("%s\n", &(Finfo.fname));
    #if _USE_LFN
        if(Finfo.lfname[strlen(Finfo.lfname)-1] == '3')
        {
            // store the file names as "1:<song name.mp3>" to the fileNames array
            sprintf(fileNamesBuff, "1:%s", Finfo.lfname);
            strncpy(fileNames[count], fileNamesBuff, sizeof(songNamesBuff));

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
                strncpy(songNames[count], small_buff, sizeof(small_buff));
            }
            else
                strncpy(songNames[count], songNamesBuff, sizeof(songNamesBuff));

            count++;
            if(count == 10)
                count = 0;
        }
    #endif
    }
//    for(int i=0;i<10;i++){
//        u0_dbg_printf("name: %s\n", songNames[i]);
//    }

    return true;

}
