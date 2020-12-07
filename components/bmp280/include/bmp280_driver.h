#ifndef _BMP280_DRIVER_H_
#define _BMP280_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BMP280_I2C_ADDRESS_0  0x76 //!< I2C address when SDO pin is low
#define BMP280_CHIP_ID  0x58 //!< BMP280 has chip-id 0x58

/**
 * BMP280 registers
 */
#define BMP280_REG_TEMP_XLSB   0xFC /* bits: 7-4 */
#define BMP280_REG_TEMP_LSB    0xFB
#define BMP280_REG_TEMP_MSB    0xFA
#define BMP280_REG_TEMP        (BMP280_REG_TEMP_MSB)
#define BMP280_REG_PRESS_XLSB  0xF9 /* bits: 7-4 */
#define BMP280_REG_PRESS_LSB   0xF8
#define BMP280_REG_PRESS_MSB   0xF7
#define BMP280_REG_PRESSURE    (BMP280_REG_PRESS_MSB)
#define BMP280_REG_CONFIG      0xF5 /* bits: 7-5 t_sb; 4-2 filter; 0 spi3w_en */
#define BMP280_REG_CTRL        0xF4 /* bits: 7-5 osrs_t; 4-2 osrs_p; 1-0 mode */
#define BMP280_REG_STATUS      0xF3 /* bits: 3 measuring; 0 im_update */
#define BMP280_REG_CTRL_HUM    0xF2 /* bits: 2-0 osrs_h; */
#define BMP280_REG_RESET       0xE0
#define BMP280_REG_ID          0xD0
#define BMP280_REG_CALIB       0x88
#define BMP280_REG_HUM_CALIB   0x88

#define BMP280_RESET_VALUE     0xB6

/**
 * Compensation Registers
 */

#define BMP_280_REG_COMP_T1      0x88
#define BMP_280_REG_COMP_T2      0x8a
#define BMP_280_REG_COMP_T3      0x8c
#define BMP_280_REG_COMP_P1      0x8e
#define BMP_280_REG_COMP_P2      0x90
#define BMP_280_REG_COMP_P3      0x92
#define BMP_280_REG_COMP_P4      0x94
#define BMP_280_REG_COMP_P5      0x96
#define BMP_280_REG_COMP_P6      0x98
#define BMP_280_REG_COMP_P7      0x9a
#define BMP_280_REG_COMP_P8      0x9c
#define BMP_280_REG_COMP_P9      0x9e
/**
 * Mode of BMP280 module operation.
 */
typedef enum {
    BMP280_MODE_SLEEP = 0,  //!< Sleep mode
    BMP280_MODE_FORCED = 1, //!< Measurement is initiated by user
    BMP280_MODE_NORMAL = 3  //!< Continues measurement
} BMP280_Mode;

typedef enum {
    BMP280_FILTER_OFF = 0,
    BMP280_FILTER_2 = 1,
    BMP280_FILTER_4 = 2,
    BMP280_FILTER_8 = 3,
    BMP280_FILTER_16 = 4
} BMP280_Filter;

/**
 * Oversampling settings
 */
typedef enum {
    BMP280_SKIPPED = 0,          //!< no measurement
    BMP280_ULTRA_LOW_POWER = 1,  //!< oversampling x1
    BMP280_LOW_POWER = 2,        //!< oversampling x2
    BMP280_STANDARD = 3,         //!< oversampling x4
    BMP280_HIGH_RES = 4,         //!< oversampling x8
    BMP280_ULTRA_HIGH_RES = 5    //!< oversampling x16
} BMP280_Oversampling;

/**
 * Stand by time between measurements in normal mode
 */
typedef enum {
    BMP280_STANDBY_05 = 0,      //!< stand by time 0.5ms
    BMP280_STANDBY_62 = 1,      //!< stand by time 62.5ms
    BMP280_STANDBY_125 = 2,     //!< stand by time 125ms
    BMP280_STANDBY_250 = 3,     //!< stand by time 250ms
    BMP280_STANDBY_500 = 4,     //!< stand by time 500ms
    BMP280_STANDBY_1000 = 5,    //!< stand by time 1s
    BMP280_STANDBY_2000 = 6,    //!< stand by time 2s BMP280, 10ms BME280
    BMP280_STANDBY_4000 = 7,    //!< stand by time 4s BMP280, 20ms BME280
} BMP280_StandbyTime;

/**
 * Compensation values
 */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_compensation_t;

/**
 * Configuration parameters for BMP280 module.
 */
typedef struct {
    BMP280_Mode mode;
    BMP280_Filter filter;
    BMP280_Oversampling oversampling_pressure;
    BMP280_Oversampling oversampling_temperature;
    BMP280_StandbyTime standby;
} bmp280_params_t;

//*****************************************************************************************
esp_err_t bmp280_init_default_params(bmp280_params_t *params);
esp_err_t bmp280_init(bmp280_params_t * params, BMP280_compensation_t * comp);
esp_err_t bmp280_read_data(BMP280_compensation_t *comp, float * temperature, float * pressure);


#ifdef __cplusplus
}
#endif

#endif //_BMP280_DRIVER_H_