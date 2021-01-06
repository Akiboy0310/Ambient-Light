#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "rgb_sensor.h"

void i2c_master_init(){
  // configure the i2c controller 0 in master mode, normal speed
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_SDA_IO;
	conf.scl_io_num = I2C_SCL_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT_NUM, &conf));
	printf("- i2c controller configured\r\n");
	
	// install the driver
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT_NUM, I2C_MODE_MASTER, 0, 0, 0));
	printf("- i2c driver installed\r\n\r\n");
    
}
void i2c_TCA9548_init(uint8_t* Channel){
    //enable the needed channels of the MUX
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // first, send device address (indicating write) & register to be written
    i2c_master_write_byte(cmd, ( TCA_ADDRESS << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    // write the data
    i2c_master_write(cmd, Channel, 1, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}
/**
 * @brief test code to read i2c slave device with registered interface
 * _______________________________________________________________________________________________________
 * | start | slave_addr + rd_bit +ack | register + ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|--------------------------|----------------|----------------------|--------------------|------|
 *
 */
esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size)
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
esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size)
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
void tcs34725_enable(uint8_t channel){
  uint8_t value;
  value=(channel);
  i2c_TCA9548_init(&(value));
    // Select enable register(0x00)
	  // Power ON, RGBC enable, wait time disable(0x03)
    value= (0x01);
    wrtcs34725x( TCS34725_ENABLE,&(value),1);
    vTaskDelay(3 / portTICK_RATE_MS);
    value= (0x03);
    wrtcs34725x( TCS34725_ENABLE,&(value),1);
}
void tcs34725_init(uint8_t channel){
  //With channel the rgb sensor that schould be controled is chosen 
  uint8_t value;
  value=(channel);
  i2c_TCA9548_init(&(value));
  // Select ALS time register(0x01)
	  // Atime = 700 ms(0x00)
    value= (0x00);
    wrtcs34725x( TCS34725_ATIME,&(value),1);
    // Select control register(0x0F)
	  // AGAIN = 1x(0x00)
    value= (0x00);
    wrtcs34725x( TCS34725_CONTROL,&(value),1);
}
void tcs34725(float*r1, float*g1, float*b1){
    uint8_t cl=0;
    uint8_t ch=0;
    uint8_t rl=0;
    uint8_t gl=0;
    uint8_t bl=0;
    uint8_t rh=0;
    uint8_t gh=0;
    uint8_t bh=0;
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t c;
    rdtcs34725x( TCS34725_RDATAL,&cl,1);
    rdtcs34725x( TCS34725_RDATAH,&ch,1);
    rdtcs34725x( TCS34725_RDATAL,&rl,1);
    rdtcs34725x( TCS34725_RDATAH,&rh,1);
    rdtcs34725x( TCS34725_GDATAL,&gl,1);
    rdtcs34725x( TCS34725_GDATAH,&gh,1);
    rdtcs34725x( TCS34725_BDATAL,&bl,1);
    rdtcs34725x( TCS34725_BDATAH,&bh,1);
    c = ((uint16_t) ch << 8) | (uint16_t) cl;
    r = ((uint16_t) rh << 8) | (uint16_t) rl;
    g = ((uint16_t) gh << 8) | (uint16_t) gl;
    b = ((uint16_t) bh << 8) | (uint16_t) bl;
    uint32_t sum = c;
    // Avoid divide by zero errors ... if clear = 0 return black
    if (c == 0) {
        *r1 = *g1 = *b1 = 0;
        return;
    }
    *r1 = (float)r / sum * 255.0;
    *g1 = (float)g / sum * 255.0;
    *b1 = (float)b / sum * 255.0;
}

