#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/i2c.h"

#define SAMPLE_PERIOD_MS		200

#define I2C_SCL_IO				21               /*!< gpio number for I2C master clock */
#define I2C_SDA_IO				22               /*!< gpio number for I2C master data  */
#define I2C_FREQ_HZ				100000           /*!< I2C master clock frequency */
#define I2C_PORT_NUM			I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_TX_BUF_DISABLE  	0                /*!< I2C master do not need buffer */
#define I2C_RX_BUF_DISABLE  	0                /*!< I2C master do not need buffer */

// I2C common protocol defines
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


void i2c_master_init(){
  // configure the i2c controller 0 in master mode, normal speed
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_SCL_IO;
	conf.scl_io_num = I2C_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT_NUM, &conf));
	printf("- i2c controller configured\r\n");
	
	// install the driver
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT_NUM, I2C_MODE_MASTER, 0, 0, 0));
	printf("- i2c driver installed\r\n\r\n");
    
}
/**
 * @brief test code to read i2c slave device with registered interface
 * _______________________________________________________________________________________________________
 * | start | slave_addr + rd_bit +ack | register + ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|--------------------------|----------------|----------------------|--------------------|------|
 *
 */
static esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size)
{
  //uint8_t throwaway=0;
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // first, send device address (indicating write) & register to be read
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ), ACK_CHECK_EN);
    // send register we want
    i2c_master_write_byte(cmd,0x80| i2c_reg, ACK_CHECK_EN);
    // Send repeated start
    i2c_master_start(cmd);
    // now send device address (indicating read) & read data
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Test code to write i2c slave device with registered interface
 *        Master device write data to slave(both esp32),
 *        the data will be stored in slave buffer.
 *        We can read them out from slave buffer.
 * ____________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | register + ack | write n bytes + ack  | stop |
 * --------|---------------------------|----------------|----------------------|------|
 *
 */
static esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // first, send device address (indicating write) & register to be written
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    // send register we want
    i2c_master_write_byte(cmd, 0x80|i2c_reg, ACK_CHECK_EN);
    // write the data
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
/* Read contents of a tcs34725 register
---------------------------------------------------------------------------*/
esp_err_t rdtcs34725x( uint8_t reg, uint8_t *pdata, uint8_t count )
{
	return( i2c_master_read_slave_reg( I2C_PORT_NUM, TCS34725_ADDRESS,  reg, pdata, count ) );
}

/* Write value to specified tcs34725 register
---------------------------------------------------------------------------*/
esp_err_t wrtcs34725x( uint8_t reg, uint8_t *pdata, uint8_t count )
{
	return( i2c_master_write_slave_reg( I2C_PORT_NUM, TCS34725_ADDRESS,  reg, pdata, count ) );
}
void tcs34725_enable(){
  uint8_t value;
    // Select enable register(0x00)
	  // Power ON, RGBC enable, wait time disable(0x03)
    value= (0x01);
    wrtcs34725x( TCS34725_ENABLE,&(value),1);
    vTaskDelay(3 / portTICK_RATE_MS);
    value= (0x03);
    wrtcs34725x( TCS34725_ENABLE,&(value),1);
}
void tcs34725_init(){
  uint8_t value;
  // Select ALS time register(0x01)
	  // Atime = 700 ms(0x00)
    value= (0x00);
    wrtcs34725x( TCS34725_ATIME,&(value),1);
    // Select control register(0x0F)
	  // AGAIN = 1x(0x00)
    value= (0x00);
    wrtcs34725x( TCS34725_CONTROL,&(value),1);
}


void app_main(void){ 
    i2c_master_init();
    tcs34725_enable();
    tcs34725_init();
    uint8_t cl=0;
    uint8_t rl=0;
    uint8_t gl=0;
    uint8_t bl=0;
    uint8_t ch=0;
    uint8_t rh=0;
    uint8_t gh=0;
    uint8_t bh=0;
    uint8_t e;
    uint16_t c;
    uint16_t r;
    uint16_t g;
    uint16_t b;
    while(1){
	  // cData lsb, cData msb, red lsb, red msb, green lsb, green msb, blue lsb, blue msb
    //wrtcs34725x(TCS34725_ENABLE,0x03,1);
    rdtcs34725x(TCS34725_ENABLE,&e,1);
    //wrtcs34725x(TCS34725_CDATAH,0x00,1);
    rdtcs34725x( TCS34725_CDATAL,&cl,1);
    rdtcs34725x( TCS34725_CDATAH,&ch,1);
    //wrtcs34725x(TCS34725_CDATAH,0x00,1);
    rdtcs34725x( TCS34725_RDATAL,&rl,1);
    rdtcs34725x( TCS34725_RDATAH,&rh,1);
    //wrtcs34725x(TCS34725_CDATAH,0x00,1);
    rdtcs34725x( TCS34725_GDATAL,&gl,1);
    rdtcs34725x( TCS34725_GDATAH,&gh,1);
    rdtcs34725x( TCS34725_BDATAL,&bl,1);
    rdtcs34725x( TCS34725_BDATAH,&bh,1);
    c = ((uint16_t) ch << 8) | (uint16_t) cl;
    r = ((uint16_t) rh << 8) | (uint16_t) rl;
    g = ((uint16_t) gh << 8) | (uint16_t) gl;
    b = ((uint16_t) bh << 8) | (uint16_t) bl;
    printf("Enable Reg: %d \n",e);
    printf("clear :%d\n",c);
    printf("red:%d\n", r);
    printf("green :%d\n", g);
    printf("blue :%d\n", b);
    vTaskDelay(800 / portTICK_RATE_MS);
  
    }
}
