#include <stdio.h>
#include <stdbool.h>

#include <esp_err.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "freertos/semphr.h"

#include <driver/gpio.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>


static esp_timer_handle_t timer_handle;

void timer_callback(void *arg){
	printf("in the ecp timer callback \n");
}
void init_esp_timer(void){
	esp_timer_create_args_t timer_args = {
        .callback = &timer_callback, // Callback function
        .arg = NULL,                 // Argument passed to the callback
        .dispatch_method = ESP_TIMER_TASK, // Run in a dedicated task
        .name = "debug_timer"   // Timer name
    };

    esp_err_t err = esp_timer_create(&timer_args, &timer_handle);
    if (err != ESP_OK) {
        // ESP_LOGE(TAG, "Failed to create timer: %s", esp_err_to_name(err));
        return;
    }

    err = esp_timer_start_periodic(timer_handle, 1000000); // Period in microseconds
    if (err != ESP_OK) {
        // ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(err));
        return;
    }
}