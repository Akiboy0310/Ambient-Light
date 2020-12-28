#ifndef _INC_RGB_H
#define _INC_RGB_H

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#define SAMPLE_PERIOD_MS		200
#define I2C_SCL_IO				21               /*!< gpio number for I2C master clock */
#define I2C_SDA_IO				22               /*!< gpio number for I2C master data  */
#define I2C_FREQ_HZ				100000           /*!< I2C master clock frequency */
#define I2C_PORT_NUM			I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_TX_BUF_DISABLE  	0                /*!< I2C master do not need buffer */
#define I2C_RX_BUF_DISABLE  	0                /*!< I2C master do not need buffer */

//I2C common protocol defines
#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

// TCS34725 defines
#define TCS34725_ADDRESS        0x29             /**< I2C address **/
#define TCS34725_ENABLE         0x00             /**< Interrupt Enable register */
#define TCS34725_ENABLE_AIEN    0x10             /**< RGBC Interrupt Enable */
#define TCS34725_ENABLE_WEN                                                    \
  0x08 /**< Wait Enable - Writing 1 activates the wait timer */
#define TCS34725_ENABLE_AEN                                                    \
  0x02 /**< RGBC Enable - Writing 1 actives the ADC, 0 disables it */
#define TCS34725_ENABLE_PON                                                    \
  0x01 /**< Power on - Writing 1 activates the internal oscillator, 0        \
            disables it */
#define TCS34725_ATIME          0x01             /**< Integration time */
#define TCS34725_WTIME          0x03             /**< Wait time*/
#define TCS34725_CONTROL        0x0F             /**< Set the gain level for the sensor */
#define TCS34725_CDATAL         0x14             /**< Clear channel data low byte */
#define TCS34725_CDATAH         0x15             /**< Clear channel data high byte */
#define TCS34725_RDATAL         0x16             /**< Red channel data low byte */
#define TCS34725_RDATAH         0x17             /**< Red channel data high byte */
#define TCS34725_GDATAL         0x18             /**< Green channel data low byte */
#define TCS34725_GDATAH         0x19             /**< Green channel data high byte */
#define TCS34725_BDATAL         0x1A             /**< Blue channel data low byte */
#define TCS34725_BDATAH         0x1B             /**< Blue channel data high byte */


void i2c_master_init();
extern esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size);
extern esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size);
extern esp_err_t rdtcs34725x( uint8_t reg, uint8_t *pdata, uint8_t count );
extern esp_err_t wrtcs34725x( uint8_t reg, uint8_t *pdata, uint8_t count );
extern void tcs34725_enable();
extern void tcs34725_init();
extern void tcs34725_1(float*r1, float*g1, float*b1);
extern void tcs34725_2(float*r2, float*g2, float*b2);
#endif