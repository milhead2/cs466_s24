#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "mGpio.h"
#include "mSpi.h"

void mGpioInit(void)
{
    mSpiInit();   
}

//
// mGpioWriteByte()
//
// will write the 'data' to an 'address' in the device.  In the datasheet for the 
// device various registers have various addresses.
//
void mGpioWriteByte( uint8_t address, uint8_t data)
{
    uint8_t preWrite = 0x40;
   	mSpiStart();                  
    mSpiTransfer(preWrite);           // send the 8 bit preamble  
    mSpiTransfer(address);            // send the 8 bit dest address 
    mSpiTransfer(data);               // The data to go at dest address
   	mSpiComplete();   
}

//
// mGpioReadByte()
//
// Similar to write this will interrogate the device for the contents of an 8 bit 
// register
//
uint8_t mGpioReadByte(uint8_t address)
{
    uint8_t value;
    uint8_t preRead = 0x41;
    
   	mSpiStart();    
    mSpiTransfer(preRead);
    mSpiTransfer(address);
    value = mSpiTransfer(0);
   	mSpiComplete(); 
    
    return value;
}