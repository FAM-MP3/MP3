/*
 * VS1053.cpp
 *
 *  Created on: Dec 6, 2018
 *      Author: kimfl
 */

#include "VS1053.h"


void VS1053::init(LPC1758_GPIO_Type dreq, LPC1758_GPIO_Type xcs, LPC1758_GPIO_Type xdcs)
{
    DREQ = new GPIO(dreq);
    DREQ->setAsInput();

    xCS  = new GPIO(xcs);
    xCS->setAsOutput();
    xCS->setHigh();

    xDCS = new GPIO(xdcs);
    xDCS->setAsOutput();
    xDCS->setHigh();

    ssp1_init();                // initialize spi
}

void VS1053::WriteSpiByte(uint8_t data)
{
    ssp1_dma_transfer_block(&data, 1, 1);
}

uint8_t VS1053::ReadSpiByte(void)
{
    uint8_t data = 0;
    ssp1_dma_transfer_block(&data, 1, 0);

    return data;
}

void VS1053::WriteSci(uint8_t addr, uint16_t data)
{
    while (!DREQ->read())
    ;                       // Wait until DREQ is high

    xCS->setLow(); // Activate xCS

    WriteSpiByte(2); // Write command code
    WriteSpiByte(addr); // SCI register number

    WriteSpiByte((uint8_t)(data >> 8));
    WriteSpiByte((uint8_t)(data & 0xFF));

    xCS->setHigh(); // Deactivate xCS
}

uint16_t VS1053::ReadSci(uint8_t addr)
{
    uint16_t data;
    while (!DREQ->read())
    ; // Wait until DREQ is high

    xCS->setLow(); // Activate xCS

    WriteSpiByte(3); // Read command code
    WriteSpiByte(addr); // SCI register number

    data = (uint16_t)ReadSpiByte() << 8;
    data |= ReadSpiByte();

    xCS->setHigh(); // Deactivate xCS

    return data;
}

void VS1053::WriteSdi(uint8_t *buffer, uint16_t byte_size)
{
    while (byte_size--)
    {
      //SPI0.transfer(c[0]);
        ssp1_exchange_byte(buffer[0]);
        buffer++;
    }
}

void VS1053::soft_reset(void){
    uint16_t mode = ReadSci(SCI_MODE);
    mode |= SM_RESET;
    WriteSci(SCI_MODE, mode);
}

void VS1053::sineTest(uint8_t n, uint16_t ms)
{
    soft_reset();
    uint16_t mode = ReadSci(SCI_MODE);
    mode |= 0x0020;
    WriteSci(SCI_MODE, mode);

    while (DREQ->read()==0);

    //begin
    xDCS->setLow();
    WriteSpiByte(0x53);
    WriteSpiByte(0xEF);
    WriteSpiByte(0x6E);
    WriteSpiByte(n);
    WriteSpiByte(0x00);
    WriteSpiByte(0x00);
    WriteSpiByte(0x00);
    WriteSpiByte(0x00);
    xDCS->setHigh();

    vTaskDelay(ms);

    //end
    xDCS->setLow();
    WriteSpiByte(0x45);
    WriteSpiByte(0x78);
    WriteSpiByte(0x69);
    WriteSpiByte(0x74);
    WriteSpiByte(0x00);
    WriteSpiByte(0x00);
    WriteSpiByte(0x00);
    WriteSpiByte(0x00);
    xDCS->setHigh();

    vTaskDelay(500);
}

void VS1053::setVolume(uint8_t left, uint8_t right)
{
    uint16_t volume;

    volume = left;
    volume <<= 8;
    volume |= right;

    WriteSci(SCI_VOL, volume);
}

VS1053::~VS1053()
{

}
VS1053::VS1053()
{

}


