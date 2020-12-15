#include "Arduino.h"
#include "tempWire.h"

// Description: This is made to interface with the TMP461 Temperature Sensor
// PDF of Datasheet here: https://ww1.microchip.com/downloads/en/DeviceDoc/PIC32MZ%20EF%20Family%20Datasheet_DS60001320G.pdf
// I2C starts on page 365

// Create object tWire
tempWire tWire;

// Constructor, does nothing I guess
tempWire::tempWire()
{};

// Description: Function to wait for proper bits to be cleared before any communication to take place
void tempWire::Wait()
{
  // Wait for first 5 bits of I2C4CON register to be cleared
  // bit0: SEN: Start Condition Enable bit
  // bit1: RSEN: Repeated Start Condition Enable bit
  // bit2: PEN: Stop Condition Enable bit
  // bit3: RCEN: Receive Enable bit
  // bit4: ACKEN: Acknowledge Sequence Enable bit
  while(I2C4CON & 0x1F);

  // Wait for Transmit Status bit to be cleared, meaning no transmission is in progress
  while(I2C4STATbits.TRSTAT);
}

// Initializes Baud rate and disables slew rate control
void tempWire::Init(uint8_t addr)
{
  // Set slave address
  slave_addr = addr + 72;

  // Disable I2C4, set all bits to 0
  I2C4CON = 0;

  // Disable Slew Rate Control (9th bit I2C4CON)
  I2C4CONbits.DISSLW = 1;

  // Set Baud Rate to 564kHz
  // Don't mess with this number, it's chosen carefully
  I2C4BRG = 0x0050; // (int)baud;

  // Turn on I2C (15th bit I2C4CON)
  I2C4CONbits.ON = 1;
}

// Description: Sends start condition over I2C
void tempWire::Start()
{
  // Wait for all other conditions to end
  Wait();

  // Set Start Condition Enable Bit
  I2C4CONbits.SEN = 1;

  // Once start condition is finished the bit is cleared, so wait for it to be cleared
  while (I2C4CON & 1);
}

// Description: Send stop condition over I2C
void tempWire::Stop()
{
  // Wait for all other conditions to end
  Wait();

  // Turn on Stop condition enable bit
  I2C4CONbits.PEN = 1;
}

// Description: Send byte (data), if ack, wait for ack to proceed
void tempWire::Write(unsigned char data, uint8_t ack)
{
  // Send slave address with Read/Write bit cleared
  I2C4TRN = data | 0;

  // Wait until transmit buffer is empty
  while (I2C4STATbits.TBF == 1);

  // Wait for all other conditions to end
  Wait();

  // Wait for acknowledge
  if (ack) while (I2C4STATbits.ACKSTAT == 1); // Wait until ACK is received
}

// Send ack from master
void tempWire::Ack()
{
  // Wait for all other conditions to end
  Wait();

  // Send ACK for master ack
  I2C4CONbits.ACKDT = 0;

  // Acknowledge enable bit high
  I2C4CONbits.ACKEN = 1;

  // When ack is sent, it is cleared so wait for it to be cleared
  while(I2C4CONbits.ACKEN);
}

// Send nack from master
void tempWire::Nack()
{
  // Wait for all other conditions to end
  Wait();

  // Send nack
  I2C4CONbits.ACKDT = 1;
  I2C4CONbits.ACKEN = 1;

  // Wait until ACKEN bit is cleared, meaning NACK bit has been sent
  while(I2C4CONbits.ACKEN);
}

// Read byte from slave
uint8_t tempWire::Read(uint8_t ack_nack)
{
  uint8_t value;
    I2C4CONbits.RCEN = 1;               // Receive enable
    while (I2C4CONbits.RCEN);           // Wait until RCEN is cleared (automatic)
    while (!I2C4STATbits.RBF);          // Wait until Receive Buffer is Full (RBF flag)
    value = I2C4RCV | 0;                   // Retrieve value from I2C1RCV

    if (!ack_nack)                      // Do we need to send an ACK or a NACK?
        Ack();                      // Send ACK
    else
        Nack();                     // Send NACK
    return value;
}

// Read temperature from TMP461
// Specific to only this device
double tempWire::readTemp()
{
  unsigned char lower, upper;
  double lower_dec, upper_dec, temp;

  // Per the data Datasheet

  // Send Slave address and pointer register location to read from
  //
  tWire.Start();
  tWire.Write(slave_addr << 1, 1);
  tWire.Write(0x00, 1); // Pointer to register 0x00: Local temperature register high byte
  tWire.Stop();

  // Send Slave address with R/W bit high to read
  tWire.Start();
  tWire.Write( (slave_addr<<1) | 1, 1);
  upper = tWire.Read(0); // xx.00
  tWire.Stop();

  // Send slave address and pointer register
  tWire.Start();
  tWire.Write(slave_addr<<1, 1);
  tWire.Write(0x15, 1); // Pointer to register 0x15 Local Temperatur register low byte
  tWire.Stop();

  // Read from sensor
  tWire.Start();
  tWire.Write( (slave_addr<<1) | 1, 1);
  lower = tWire.Read(0);  //00.xx
  tWire.Stop();

  // In total, the temperature information is in a 16bit word.
  // Convert this information to celcius (read the data sheet)
  lower = lower >> 4;
  lower_dec = (double)lower * 0.0625;
  upper_dec = (double)upper;
  temp = lower_dec + upper_dec;


  return temp;
}
