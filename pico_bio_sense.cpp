#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico_max3010x/max3010x.hpp"

#include "bitmap.h"
#include "lib/OLED/OLED.h"
#include "lib/OLED/font/Cherry_Cream_Soda_Regular_16.h"

#include "mpu6050/mpu6050_i2c.h"

#include "onewire_library.h"
#include "ow_rom.h"
#include "ds18b20.h"

const uint8_t PIN_WIRE_SDA = 4;
const uint8_t PIN_WIRE_SCL = 5;
i2c_inst_t *I2C_PORT = i2c0;
MAX3010X heartSensor(I2C_PORT, PIN_WIRE_SDA, PIN_WIRE_SCL, I2C_SPEED_STANDARD);

const uint8_t POWER_LEVEL = 0x1F;    // Options: 0=Off to 255=50mA
const uint8_t SAMPLE_AVERAGE = 0x04; // Options: 1, 2, 4, 8, 16, 32
const uint8_t LED_MODE = 0x03;       // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
const int SAMPLE_RATE = 400;         // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
const int PULSE_WIDTH = 411;         // Options: 69, 118, 215, 411
const int ADC_RANGE = 4096;          // Options: 2048, 4096, 8192, 16384

const int MAX_DEVS = 1;
const uint GPIO_ONEWIRE = 26;

static float convertIntervalUsToBeatsPerSecond(uint64_t intervalUs)
{
    if (intervalUs == 0)
        return 0.0f;
    return 1000000.0f / (float)intervalUs;
}

static float getBeatsPerSecondFromPeaks(uint32_t currentIR, uint32_t &prevIR, uint32_t &prevPrevIR, uint64_t &lastPeakTimeUs)
{
    float beatsPerSecond = 0.0f;

    if (prevPrevIR < prevIR && prevIR > currentIR)
    {
        const uint64_t nowUs = time_us_64();
        if (lastPeakTimeUs != 0)
        {
            const uint64_t intervalUs = nowUs - lastPeakTimeUs;
            beatsPerSecond = convertIntervalUsToBeatsPerSecond(intervalUs);
        }
        lastPeakTimeUs = nowUs;
    }

    prevPrevIR = prevIR;
    prevIR = currentIR;
    return beatsPerSecond;
}

int main(void)
{
    int16_t acceleration[3], gyro[3], temp;

    stdio_init_all();

    busy_wait_ms(500);

    PIO pio = pio0;
    const uint gpio = GPIO_ONEWIRE;
    int num_devs;
    const int maxdevs = MAX_DEVS;
    uint64_t romcode[maxdevs];

    OW ow;
    uint offset;

    if (pio_can_add_program(pio, &onewire_program))
    {
        offset = pio_add_program(pio, &onewire_program);

        // claim a state machine and initialise a driver instance
        if (ow_init(&ow, pio, offset, gpio))
        {

            // find and display 64-bit device addresses

            num_devs = ow_romsearch(&ow, romcode, maxdevs, OW_SEARCH_ROM);

            printf("Found %d devices\n", num_devs);
            for (int i = 0; i < num_devs; i += 1)
            {
                printf("\t%d: 0x%llx\n", i, romcode[i]);
            }
            putchar('\n');
        }
        else
        {
            puts("could not initialise the driver");
        }
    }
    else
    {
        puts("could not add the program");
    }

    while (!heartSensor.begin())
    {
        printf("MAX30102 not connect r fail load calib coeff \r\n");
        busy_wait_ms(500);
    }

    heartSensor.setup(POWER_LEVEL, SAMPLE_AVERAGE, LED_MODE, SAMPLE_RATE, PULSE_WIDTH, ADC_RANGE);

    uint32_t prevIR = 0;
    uint32_t prevPrevIR = 0;
    uint64_t lastPeakTimeUs = 0;

    mpu6050_reset();

    OLED oled(PIN_WIRE_SCL, PIN_WIRE_SDA, 128, 64, 400000, false, I2C_PORT);

    while (1)
    {
        tight_loop_contents();

        uint64_t timestamp = time_us_64();

        mpu6050_read_raw(acceleration, gyro, &temp);
        float temp_mpu = (temp / 340.0) + 36.53;

        uint32_t currentIR = heartSensor.getIR();
        uint32_t red = heartSensor.getRed();
        float beatsPerSecond = getBeatsPerSecondFromPeaks(currentIR, prevIR, prevPrevIR, lastPeakTimeUs);

        float ds_temps[MAX_DEVS] = {0.0f};

        if (num_devs > 0)
        {
            // start temperature conversion in parallel on all devices
            // (see ds18b20 datasheet)
            ow_reset(&ow);
            ow_send(&ow, OW_SKIP_ROM);
            ow_send(&ow, DS18B20_CONVERT_T);

            // wait for the conversions to finish
            while (ow_read(&ow) == 0)
            {
                ;
            }

            // read the result from each device
            for (int i = 0; i < num_devs; i += 1)
            {
                ow_reset(&ow);
                ow_send(&ow, OW_MATCH_ROM);
                for (int b = 0; b < 64; b += 8)
                {
                    ow_send(&ow, romcode[i] >> b);
                }
                ow_send(&ow, DS18B20_READ_SCRATCHPAD);
                int16_t ds_temp_raw = 0;
                ds_temp_raw = ow_read(&ow) | (ow_read(&ow) << 8);
                ds_temps[i] = ds_temp_raw / 16.0f;
            }
        }

        // Output in CSV format: timestamp,accx,accy,accz,gyrox,gyroy,gyroz,tempmpu,ir,red,bpm,ds1,ds2,...,ds10
        printf("%llu,%d,%d,%d,%d,%d,%d,%.2f,%lu,%lu,%.2f",
               timestamp, acceleration[0], acceleration[1], acceleration[2],
               gyro[0], gyro[1], gyro[2], temp_mpu, currentIR, red, beatsPerSecond);

        for (int i = 0; i < MAX_DEVS; i++)
        {
            printf(",%.2f", ds_temps[i]);
        }
        printf("\n");

        // Optional: delay or adjust loop speed
        // busy_wait_ms(1); // Adjust as needed for sampling rate
    }
    return 0;
}
