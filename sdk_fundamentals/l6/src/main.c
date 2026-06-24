/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#define SLEEP_TIME_MS   	1000

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);


#define MPU6050_WHO_AM_I_REG 		0x75
#define MPU6050_WHO_AM_I_VAL 		0x68
#define MPU6050_ACCEL_XOUT_H_REG 	0x3B

#define MPU6050_PWR_MGMT_1_REG 		0x6B
#define MPU6050_SLEEP_EN       		BIT(6)

#define I2C_NODE DT_NODELABEL(mysensor)
static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C_NODE);

int main(void)
{
	int ret;

	if (!device_is_ready(led0.port)){
		printk("GPIO device is not ready\r\n");
		return 1;
	}

	if (!device_is_ready(dev_i2c.bus)) {
		printk("I2C bus %s is not ready!\n\r",dev_i2c.bus->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 1 ; 
	}
	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 1 ;
	}
	ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 1 ;
	}

	uint8_t id = 0;
	uint8_t regs[] = {MPU6050_WHO_AM_I_REG};

	ret = i2c_write_read_dt(&dev_i2c, regs, 1, &id, 1);

	if (ret != 0) {
		printk("Failed to read register %x \n", regs[0]);
		return -1;
	}

	if (id != MPU6050_WHO_AM_I_VAL) {
		printk("Invalid chip id! %x \n", id);
		return -1;
	} else {
		printk("MPU6050 detected! Chip id: %x \n", id);
	}

	ret = i2c_reg_update_byte_dt(&dev_i2c,
                             MPU6050_PWR_MGMT_1_REG,
                             MPU6050_SLEEP_EN,
                             0);

	if (ret != 0) {
			printk("Failed to wake MPU6050: %d\n", ret);
			return ret;
	}

	k_msleep(10);

	uint8_t pwr_mgmt;

	ret = i2c_reg_read_byte_dt(&dev_i2c,
							MPU6050_PWR_MGMT_1_REG,
							&pwr_mgmt);

	printk("PWR_MGMT_1: 0x%02x, ret: %d\n", pwr_mgmt, ret);

	uint8_t data[6];
	int16_t x, y, z;

	while (1) {
		ret = i2c_burst_read_dt(&dev_i2c, MPU6050_ACCEL_XOUT_H_REG, data, 6);
		if (!ret) {
			x = (int16_t)sys_get_be16(&data[0]);
			y = (int16_t)sys_get_be16(&data[2]);
			z = (int16_t)sys_get_be16(&data[4]);
			printk("Accelerometer data: %d %d %d\n", x, y, z);
		} else {
			printk("Failed to read accelerometer data\n");
		}
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
