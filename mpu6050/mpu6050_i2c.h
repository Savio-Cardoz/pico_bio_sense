#ifndef __MPU6050_I2C_H__
#define __MPU6050_I2C_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void mpu6050_reset();
    void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp);

#ifdef __cplusplus
}
#endif

#endif