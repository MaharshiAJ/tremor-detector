#include <mbed.h>

#include "dft.h"
#include "detection.h"

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
// #define CTRL_REG3_CONFIG 0b0'0'0'0'1'000

/*
CTRL_REG4_CONFIG defined with bits
0|BLE|FS1|FS0|-|ST1|ST0|SIM

BLE = 0 : Data LSB @ lower address
FS1,FS0 = 01 : 500 dps (Measurement Range)
ST1,ST0 = 00 : Self-Test disabled
SIM = 0 : 4-Wire interface
*/
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0

#define SPI_FLAG 1 // Flag for SPI communication completion
// #define DATA_READY_FLAG 2 // Data ready flag

// Factor used when converting raw sensor data
#define SCALING_FACTOR (17.5f * 0.0174532925199432957692236907684886f / 1000.0f)
#define SAMPLE_RATE 200 // Hz
#define WINDOW_SIZE 3   // Seconds
#define WINDOW_ARR_SIZE (SAMPLE_RATE * WINDOW_SIZE)

EventFlags flags;

// Call back for when SPI transfer is completed
void spi_cb(int event)
{
  flags.set(SPI_FLAG);
}
// Call back for data ready
// void data_cb()
// {
//   flags.set(DATA_READY_FLAG);
// }

float compute_magnitude(float x, float y, float z)
{
  return sqrt((x * x) + (y * y) + (z * z));
}

DigitalOut led(LED1);

void init_arr(float *arr, int size)
{
  for (int i = 0; i < size; i++)
  {
    arr[i] = 0;
  }
}

void print_arr(float *arr, int size)
{
  for (int i = 0; i < size; i++)
  {
    printf("%f\n", arr[i]);
  }
}

float gyro_data[WINDOW_ARR_SIZE];
int data_index = 0;

int main()
{
  SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);

  uint8_t write_buf[32], read_buf[32];

  // InterruptIn int2(PA_2, PullDown);
  // int2.rise(&data_cb);

  spi.format(8, 3);
  spi.frequency(1'000'000);

  write_buf[0] = CTRL_REG1;
  write_buf[1] = CTRL_REG1_CONFIG;
  spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  flags.wait_all(SPI_FLAG);

  write_buf[0] = CTRL_REG4;
  write_buf[1] = CTRL_REG4_CONFIG;
  spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  flags.wait_all(SPI_FLAG);

  // write_buf[0] = CTRL_REG3;
  // write_buf[1] = CTRL_REG3_CONFIG;
  // spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  // flags.wait_all(SPI_FLAG);

  // Dummy data
  // write_buf[1] = 0xFF;

  float power[WINDOW_ARR_SIZE];
  init_arr(gyro_data, WINDOW_ARR_SIZE);

  while (1)
  {
    uint16_t raw_gx, raw_gy, raw_gz;
    float gx, gy, gz;

    // flags.wait_all(DATA_READY_FLAG);

    write_buf[0] = OUT_X_L | 0x80 | 0x40;

    spi.transfer(write_buf, 7, read_buf, 7, spi_cb);
    flags.wait_all(SPI_FLAG);

    // Convert from 2's complement to 16 bit raw data
    raw_gx = (((uint16_t)read_buf[2]) << 8) | ((uint16_t)read_buf[1]);
    raw_gy = (((uint16_t)read_buf[4]) << 8) | ((uint16_t)read_buf[3]);
    raw_gz = (((uint16_t)read_buf[6]) << 8) | ((uint16_t)read_buf[5]);

    // Convert to actual data
    gx = ((float)raw_gx) * SCALING_FACTOR;
    gy = ((float)raw_gy) * SCALING_FACTOR;
    gz = ((float)raw_gz) * SCALING_FACTOR;

    // Convert to a magnitude and store in window
    gyro_data[data_index] = compute_magnitude(gx, gy, gz);

    // Reset index to zero when the window is fully filled out and perform dft
    data_index += 1;

    if (data_index >= WINDOW_ARR_SIZE)
    {
      data_index = 0;

      dft(gyro_data, WINDOW_ARR_SIZE, WINDOW_ARR_SIZE, power);

      int intensity = detectPeakIntensity(power, WINDOW_ARR_SIZE, SAMPLE_RATE);

      printf("Intensity: %d", intensity);

      init_arr(gyro_data, WINDOW_ARR_SIZE);
    }

    thread_sleep_for(5);

    // if (intensity > 0)
    // {
    //   int delay = 1000 - intensity;
    // }

    // // Wait before reading next point
    // thread_sleep_for(5);
  }
}
