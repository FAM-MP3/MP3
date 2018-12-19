#ifndef PLAYER_RECORDER_H
#define PLAYER_RECORDER_H

#include "vs10xx_uc.h"
#include "LCDTask.hpp"
//#include "def.hpp"
//#include "def.hpp"
//int VSTestInitHardware(void);
//int VSTestInitSoftware(void);
int VSTestHandleFile(const char *fileName);

//int WriteSci(u_int8 addr, u_int16 data);
//u_int16 ReadSci(u_int8 addr);
//int WriteSdi(const u_int8 *data, u_int8 bytes);
void SaveUIState(void);
void RestoreUIState(void);
int GetUICommand(void);

int WriteSpiByte(uint8_t data);                        // You need to provide this
uint8_t ReadSpiByte(void);                              // You need to provide this
//void SetGpioLow(int registerName); // You need to provide this
//void SetGpioHigh(int registerName); // You need to provide this
void WaitMicroseconds(int microseconds);                // You need to provide this
int WriteSci(uint8_t addr, uint16_t data);             // Examples in this document
uint16_t ReadSci(uint8_t addr);                         // Examples in this document
int WriteSdi(const uint8_t *data, uint8_t bytes);    // Examples in this document
int VSTestInitSoftware(void);                           // Initialize software                                // Initialize pins
void WriteVS10xxMem(uint16_t addr, uint16_t data);      // Write 16-bit value to given VS10xx address
void LoadPlugin(const uint16_t *d, uint16_t len);       // Load plugin
int InitVS10xx(void);
UINT my_fread(void *ptr, size_t size, size_t nmemb, FIL *stream);

void sineTest_begin();                                  // function to begin sine test
void sineTest_end();                                    // function to end sine test
//int WriteSdiChar(uint8_t *data, uint16_t bytes);         // new WriteSdi
int WriteSdiChar(unsigned char *data, uint16_t bytes);         // new WriteSdi

//void InitSoftware();                                    // software init
void setVolume(uint8_t left, uint8_t right);
void play(unsigned char* buffer, int size);               // play song
void xDCS_setLow();                                          // set DREQ low
void xDCS_setHigh();                                         // set DREQ high
bool getDREQ_lvl();                                       // get DREQ lvl

void sendLCDData(char data[]);                              // writes LCD Data
//void ClearStar();                                           // clears * in every line
double ConvertVolume(uint8_t vol);                          // converts volume to %
bool InitLCD();                                             // initializes LCD with song names
double ConvertBass(uint8_t bass);                           // converts bass to %
void setBass(uint8_t enhancement);


//bool InitLCD();                                           // initializes LCD with song names
//
//
///* GPIO pins */
//GPIO GPIO1(P0_26);              // reset
//GPIO GPIO2(P1_31);              // DREQ
//GPIO GPIO3(P1_30);              // xCS
//GPIO GPIO4(P1_29);              // xDCS

#endif
