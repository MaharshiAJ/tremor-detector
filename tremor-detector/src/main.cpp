#include <mbed.h>

#include "dft.cpp"
#include "detection.cpp"

// Defining register locations
#define CTRL_REG1 0x20
#define CTRL_REG3 0x22
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
CTRL_REG3_CONFIG defined with bits
I1_Int1|I1_Boot|H_Lactive|PP_OD|I2_DRDY|I2_WTM|I2_ORun|I2_Empty

I1_Int1 = 0 : Disable interrupt on INT1 pin
I1_Boot = 0 : Disable boot status on INT1 pin
H_Lactive = 0 : High active configuration on INT1 pin
PP_OD = 0 : push-pull drain
I2_DRDY = 1 : Data ready on INT2 enabled
I2_WTM = 0 : Disable FIFO watermark interrupt on INT2
I2_ORun = 0 : Disable FIFO overrun interrupt on INT2
I2_Empty = 0 : Disable FIFO empty interrupt on INT2
*/
#define CTRL_REG3_CONFIG 0b0'0'0'0'1'000

/*
CTRL_REG4_CONFIG defined with bits
0|BLE|FS1|FS0|-|ST1|ST0|SIM

BLE = 0 : Data LSB @ lower address
FS1,FS0 = 01 : 500 dps (Measurement Range)
ST1,ST0 = 00 : Self-Test disabled
SIM = 0 : 4-Wire interface
*/
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0

#define SPI_FLAG 1        // Flag for SPI communication completion
#define DATA_READY_FLAG 2 // Data ready flag

// Factor used when converting raw sensor data
#define SCALING_FACTOR (17.5f * 0.0174532925199432957692236907684886f / 1000.0f)
#define SAMPLE_RATE 200 // Hz

EventFlags flags;

// Call back for when SPI transfer is completed
void spi_cb(int event)
{
  flags.set(SPI_FLAG);
}
// Call back for data ready
void data_cb()
{
  flags.set(DATA_READY_FLAG);
}

int main()
{
  SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);

  uint8_t write_buf[32], read_buf[32];

  InterruptIn int2(PA_2, PullDown);
  int2.rise(&data_cb);

  spi.format(8, 3);
  spi.frequency(1'000'000);

  write_buf[0] = CTRL_REG1;
  write_buf[1] = CTRL_REG1_CONFIG;
  spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  flags.wait_all(SPI_FLAG);

  write_buf[0] = CTRL_REG3;
  write_buf[1] = CTRL_REG3_CONFIG;
  spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  flags.wait_all(SPI_FLAG);

  write_buf[0] = CTRL_REG4;
  write_buf[1] = CTRL_REG4_CONFIG;
  spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  flags.wait_all(SPI_FLAG);

  // Dummy data
  write_buf[1] = 0xFF;

  while (1)
  {
  }
}
