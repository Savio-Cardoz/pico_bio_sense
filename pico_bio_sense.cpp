#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico_max3010x/max3010x.hpp"

#define MAX3010X_ADDRESS 0x57

#define PIN_WIRE_SDA 4
#define PIN_WIRE_SCL 5
#define I2C_PORT i2c0
MAX3010X heartSensor(I2C_PORT, PIN_WIRE_SDA, PIN_WIRE_SCL, I2C_SPEED_STANDARD);

int main(void)
{

    stdio_init_all();

    busy_wait_ms(500);
    while (heartSensor.begin() != true)
    {
        printf("MAX30102 not connect r fail load calib coeff \r\n");
        busy_wait_ms(500);
    }

    uint8_t powerLevel = 0x1F;    // Options: 0=Off to 255=50mA
    uint8_t sampleAverage = 0x04; // Options: 1, 2, 4, 8, 16, 32
    uint8_t ledMode = 0x03;       // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    int sampleRate = 400;         // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411;         // Options: 69, 118, 215, 411
    int adcRange = 4096;          // Options: 2048, 4096, 8192, 16384
    heartSensor.setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

    while (1)
    {
        printf("%lu,%lu\n", heartSensor.getIR(), heartSensor.getRed());
    }
    return 0;
}

// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "pico/binary_info.h"
// #include "pico_max3010x/max30102.hpp"

// uint32_t approxRollingAverage(uint32_t avg, uint32_t new_sample, uint32_t N)
// {
//     avg -= avg / N;
//     avg += new_sample / N;
//     return avg;
// }

// int main()
// {

//     // init uart
//     stdio_init_all();
//     printf("Start\n");

//     max30102 device;
//     uint32_t sample;
//     uint32_t avg;
//     uint32_t samples[32];

//     while (true)
//     {

//         // read red and ir data from fifo
//         device.read_fifo();
//         uint8_t read = device.read_fifo_read_ptr();
//         sample = device.spo2;
//         samples[read] = sample;

//         // print smoothed out data
//         uint32_t sum = 0;
//         for (uint8_t i = 0; i < 31; i++)
//         {
//             sum += samples[i];
//         }
//         uint32_t avg = sum / 32;

//         // print die temperature
//         double temp = device.read_temperature();
//         printf("%d | %f°c\n", avg, temp);
//     }

//     return 0;
// }