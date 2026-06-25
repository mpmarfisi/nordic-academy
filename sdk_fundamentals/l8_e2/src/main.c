/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <string.h>

#define THREAD0_STACKSIZE 1024
#define THREAD1_STACKSIZE 1024

// Equal priority threads to demonstrate timeslicing
#define THREAD0_PRIORITY        4 
#define THREAD1_PRIORITY        4

// Shared variables
#define COMBINED_TOTAL   40
int32_t increment_count = 0; 
int32_t decrement_count = COMBINED_TOTAL; 

K_MUTEX_DEFINE(test_mutex);

// Shared code run by both threads
void shared_code_section(void)
{
	uint8_t race_condition = 0;
	int32_t increment_count_copy = 0;
	int32_t decrement_count_copy = 0;
	
	k_mutex_lock(&test_mutex, K_FOREVER);

	increment_count += 1;
	increment_count = increment_count % COMBINED_TOTAL; 

	decrement_count -= 1;
	if (decrement_count == 0) {
		decrement_count = COMBINED_TOTAL;
	}

	if (increment_count + decrement_count != COMBINED_TOTAL) {
        race_condition = 1;
		increment_count_copy = increment_count;
        decrement_count_copy = decrement_count;
    }

	k_mutex_unlock(&test_mutex);

	if( race_condition ){
        printk("Race condition happened!\n");
        printk("Increment_count (%d) + Decrement_count (%d) = %d \n", increment_count_copy,
            decrement_count_copy, (increment_count_copy + decrement_count_copy));
        k_msleep(400 + sys_rand32_get() % 10);
    }

}

void thread0(void)
{
	printk("Thread 0 started\n");
	while (1) {
		shared_code_section(); 
	}
}

void thread1(void)
{
	printk("Thread 1 started\n");
	while (1) {
		shared_code_section(); 
	}
}

// Define and initialize threads
K_THREAD_DEFINE(thread0_id, THREAD0_STACKSIZE, thread0, NULL, NULL, NULL, THREAD0_PRIORITY, 0, 5000);
K_THREAD_DEFINE(thread1_id, THREAD1_STACKSIZE, thread1, NULL, NULL, NULL, THREAD1_PRIORITY, 0, 5000);