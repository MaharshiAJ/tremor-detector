#include <mbed.h>

#include "dft.cpp"
#include "detection.cpp"

// Defining register locations
#define CTRL_REG1 0x20
#define CTRL_REG4 0x23
#define OUT_X_L 0x28
/*
CTRL_REG1_CONFIG defined with bits
DR1|DR0|BW1|BW0|PD|Zen|Yen|Xen

DR1,DR0,BW1,BW0 = 0110 : Sets Data rate to 200 hz
PD = 1 : Normal Mode
Zen = 1 : Z axis enabled
Yen = 1 : Y axis enabled
Xen = 1 : X axis enabled
*/
#define CTRL_REG1_CONFIG 0b01'10'1'1'1'1
/*
CTRL_REG4_CONFIG defined with bits
0|BLE|FS1|FS0|-|ST1|ST0|SIM

BLE = 0 : Data LSB @ lower address
FS1,FS0 = 01 : 500 dps (Measurement Range)
ST1,ST0 = 00 : Self-Test disabled
SIM = 0 : 4-Wire interface
*/
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0

int main()
{
  while (1)
  {
  }
}
