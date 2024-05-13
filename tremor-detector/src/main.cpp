#include <mbed.h>

#include "dft.h"
#include "detection.h"
#include "drivers/stm32f429i_discovery_lcd.h"

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
#define WINDOW_SIZE 1   // Seconds
#define WINDOW_ARR_SIZE (SAMPLE_RATE * WINDOW_SIZE)
#define FILTER_COEFFICIENT 0.1f
#define INTENSITY_SCALER 5
#define DEFAULT_BLINK_SPEED 1000 // ms
#define MINIMUM_BLINK_SPEED 10   // ms

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

float compute_magnitude(float x, float y, float z)
{
  return sqrt((x * x) + (y * y) + (z * z));
}

DigitalOut led(LED1);

float gyro_data[WINDOW_ARR_SIZE] = {0};
int window_index = 0;

int blink_speed = DEFAULT_BLINK_SPEED;
Mutex blink_speed_mutex;

void blink(DigitalOut *led)
{
  int current_speed;
  while (1)
  {
    blink_speed_mutex.lock();
    current_speed = blink_speed;
    blink_speed_mutex.unlock();

    printf("Blink speed: %d\n", current_speed);

    *led = !*led;
    thread_sleep_for(current_speed);
  }
}

void set_speed(int intensity)
{
  blink_speed_mutex.lock();
  blink_speed = max(MINIMUM_BLINK_SPEED, DEFAULT_BLINK_SPEED - (intensity * INTENSITY_SCALER));
  blink_speed_mutex.unlock();
}

void print_arr(float *toprint)
{
  for (int i = 0; i < WINDOW_ARR_SIZE; i++)
  {
    printf("%f\n", toprint[i]);
  }
}


void updateLCD(float intensity, bool isTremorDetected) {
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font20);
    
    // Display tremor intensity
    char buffer[50];
    sprintf(buffer, "Intensity: %.2f", intensity);
    BSP_LCD_DisplayStringAt(0, LINE(1), (uint8_t *)buffer, CENTER_MODE);

    // Display tremor status
    sprintf(buffer, "Tremor Detected: %s", isTremorDetected ? "Yes" : "No");
    BSP_LCD_DisplayStringAt(0, LINE(3), (uint8_t *)buffer, CENTER_MODE);
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

  write_buf[0] = CTRL_REG4;
  write_buf[1] = CTRL_REG4_CONFIG;
  spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  flags.wait_all(SPI_FLAG);

  write_buf[0] = CTRL_REG3;
  write_buf[1] = CTRL_REG3_CONFIG;
  spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
  flags.wait_all(SPI_FLAG);

  // Dummy data
  write_buf[1] = 0xFF;

  float power[WINDOW_ARR_SIZE] = {0};

  if (!(flags.get() & DATA_READY_FLAG) && (int2.read() == 1))
  {
    flags.set(DATA_READY_FLAG);
  }

  float filtered_gx = 0.0f, filtered_gy = 0.0f, filtered_gz = 0.0f;
  int intensity;
  Thread tf;
  tf.start(callback(&blink, &led));

  while (1)
  {
    uint16_t raw_gx, raw_gy, raw_gz;
    float gx, gy, gz;

    flags.wait_all(DATA_READY_FLAG);

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

    filtered_gx = FILTER_COEFFICIENT * gx + (1 - FILTER_COEFFICIENT) * filtered_gx;
    filtered_gy = FILTER_COEFFICIENT * gy + (1 - FILTER_COEFFICIENT) * filtered_gy;
    filtered_gz = FILTER_COEFFICIENT * gz + (1 - FILTER_COEFFICIENT) * filtered_gz;

    gyro_data[window_index] = compute_magnitude(filtered_gx, filtered_gy, filtered_gz);

    window_index++;

    if (window_index >= WINDOW_ARR_SIZE)
    {
      dft(gyro_data, WINDOW_ARR_SIZE, WINDOW_ARR_SIZE, power);
      intensity = detectPeakIntensity(power, WINDOW_ARR_SIZE, SAMPLE_RATE);
      bool isTremorDetected = intensity > 20; // Define your threshold
      updateLCD(intensity, isTremorDetected);
      set_speed(intensity);
      window_index = 0;
    }
    // thread_sleep_for(100);
  }
}
