// Parkinson's Tremor Detection Using STM32 F429 Discovery Board
#include <mbed.h>

// Timer for periodic actions
Ticker periodicTicker;
volatile int tickCount = 0;

// Increment the tick counter in a cyclic manner
void updateTickCount() {
    tickCount = (tickCount + 1) % 50000;
}

// Gyroscope configuration settings
#define GYRO_SETUP_REG1 0x20
#define SETUP_VALUE_REG1 0b01'10'1'1'1'1
#define GYRO_SETUP_REG4 0x23
#define SETUP_VALUE_REG4 0b0'0'01'0'00'0
#define SPI_TRANSACTION_COMPLETE 1
#define GYROSCOPE_READ_ADDRESS 0x28

EventFlags spiTransactionFlag;

// Notify when SPI transaction completes
void onSPIDone(int status) {
    spiTransactionFlag.set(SPI_TRANSACTION_COMPLETE);
}

#define CONVERSION_FACTOR (0.0174533f)  // Radians per degree

int main() {
    // Output indicators
    DigitalOut tremorIndicator(LED1, 0), severityIndicator(LED2, 0);

    // SPI interface configuration
    SPI spiInterface(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);
    periodicTicker.attach(&updateTickCount, 0.01);  // Tick every 10ms

    uint8_t spiOutputBuffer[32], spiInputBuffer[32];
    uint8_t isSteady;

    // Set SPI parameters
    spiInterface.format(8, 3);
    spiInterface.frequency(1000000);

    // Initialize gyroscope
    spiOutputBuffer[0] = GYRO_SETUP_REG1;
    spiOutputBuffer[1] = SETUP_VALUE_REG1;
    spiInterface.transfer(spiOutputBuffer, 2, spiInputBuffer, 2, onSPIDone);
    spiTransactionFlag.wait_all(SPI_TRANSACTION_COMPLETE);

    spiOutputBuffer[0] = GYRO_SETUP_REG4;
    spiOutputBuffer[1] = SETUP_VALUE_REG4;
    spiInterface.transfer(spiOutputBuffer, 2, spiInputBuffer, 2, onSPIDone);
    spiTransactionFlag.wait_all(SPI_TRANSACTION_COMPLETE);

    uint16_t startMarker;

    // Digital Signal Processing (DSP) coefficients
    float feedback[5] = {1.0, -0.482, 0.810, -0.227, 0.272};
    float forward[5] = {0.131, 0.0, -0.262, 0.0, 0.131};

    float yData[5] = {0};
    int16_t outputData[5] = {0};
    int16_t meanAngularY = 0;
    int16_t tremorCount = 0;

    while(true) {
        uint16_t sensorDataX, sensorDataY, sensorDataZ;
        int16_t velX, velY, velZ;
        startMarker = tickCount;

        // Read data from gyroscope
        spiOutputBuffer[0] = GYROSCOPE_READ_ADDRESS | 0x80 | 0x40;
        spiInterface.transfer(spiOutputBuffer, 7, spiInputBuffer, 7, onSPIDone);
        spiTransactionFlag.wait_all(SPI_TRANSACTION_COMPLETE);

        // Parse the SPI data into raw sensor readings
        sensorDataX = (((uint16_t)spiInputBuffer[2]) << 8) | ((uint16_t)spiInputBuffer[1]);
        sensorDataY = (((uint16_t)spiInputBuffer[4]) << 8) | ((uint16_t)spiInputBuffer[3]);
        sensorDataZ = (((uint16_t)spiInputBuffer[6]) << 8) | ((uint16_t)spiInputBuffer[5]);

        // Calculate angular velocities
        velX = ((int16_t)sensorDataX) * CONVERSION_FACTOR;
        velY = ((int16_t)sensorDataY) * CONVERSION_FACTOR;
        velZ = ((int16_t)sensorDataZ) * CONVERSION_FACTOR;

        // Update buffer for DSP
        for (int i = 4; i > 0; --i) {
            yData[i] = yData[i - 1];
            outputData[i] = outputData[i - 1];
        }
        yData[0] = velY;

        // Apply DSP to filter the signal
        outputData[0] = forward[0] * yData[0];
        for (int i = 1; i < 5; ++i) {
            outputData[0] += int16_t(forward[i] * yData[i] - feedback[i] * outputData[i]);
        }

        // Determine tremor stability
        isSteady = (abs(velX) + abs(velZ) < 50) ? 1 : 0;
        meanAngularY = int16_t((49 * meanAngularY + isSteady * abs(outputData[0])) / 50);

        // Tremor detection and signaling
        if (meanAngularY > 5) {
            tremorCount++;
            tremorIndicator = 1;
        } else {
            tremorCount = tremorCount < 10 ? 0 : tremorCount - 10;
            tremorIndicator = 0;
        }

        if (tremorCount > 200) {
            if (meanAngularY > 20) {
                severityIndicator = !severityIndicator;  // Toggle the LED for severe tremors
            } else {
                severityIndicator = 1;
            }
        } else {
            severityIndicator = 0;
        }

        // Maintain consistent timing
        thread_sleep_for(50 - (tickCount - startMarker) % 50000 * 10);
    }
}
