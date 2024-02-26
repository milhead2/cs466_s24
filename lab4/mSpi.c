#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "mSpi.h"

//
// Macros to set the ChipSelect, Clock, MasterOut and read the MasterIn signals..
// Needed a delay so my tools could see it.
// 
const uint32_t spiDelayUs = 500;  //

#define setCS(x)   do { gpio_put(CS_PIN, ((x) != 0));     sleep_us(spiDelayUs); } while(0)
#define setSCK(x)  do { gpio_put(CLK_PIN, ((x) != 0));     sleep_us(spiDelayUs); } while(0)
#define setMOSI(x) do { gpio_put(MOSI_PIN, ((x) != 0));     sleep_us(spiDelayUs); } while(0)
#define getMISO()  gpio_get(MISO_PIN)


void mSpiInit(void)
{
    // initalize GPIO's
    gpio_init(CS_PIN);
    gpio_init(CLK_PIN);
    gpio_init(MOSI_PIN);
    gpio_init(MISO_PIN);

    gpio_set_dir(CS_PIN, GPIO_OUT);
    gpio_set_dir(CLK_PIN, GPIO_OUT);
    gpio_set_dir(MOSI_PIN, GPIO_OUT);
    gpio_set_dir(MISO_PIN, GPIO_IN);

    //gpio_set_drive_strength(CS_PIN, GPIO_DRIVE_STRENGTH_4MA);
    //gpio_set_drive_strength(CLK_PIN, GPIO_DRIVE_STRENGTH_4MA);
    //gpio_set_drive_strength(MOSI_PIN, GPIO_DRIVE_STRENGTH_4MA);

    setSCK(LOW);
    setCS(HIGH);
}

//
// spiTransfer():
//
// This is the private bit-bang master transfer command.  The same function is used to write 
// and read SPI data.  On complex devices you can perform both operations at once but 
// for the use of this lab we are reading or writing, See it's use below.
//
uint8_t mSpiTransfer( uint8_t outData)
{
    uint8_t count, in = 0;

    setSCK(LOW); 
    for (count = 0; count < 8; count++)
    {
        in <<= 1;
        setMOSI(outData & 0x80);    // set Output bit
        setSCK(HIGH);               // Clock Rising Edge
        in += getMISO();            // Read the data bit
        setSCK(LOW);                // Clock Rising Edge
        outData <<= 1;              // shift read bit
    }
    setMOSI(0); 

    return (in);
}

void mSpiStart(void)
{
    setCS(LOW);
}

void mSpiComplete(void)
{
    setCS(HIGH);
}



