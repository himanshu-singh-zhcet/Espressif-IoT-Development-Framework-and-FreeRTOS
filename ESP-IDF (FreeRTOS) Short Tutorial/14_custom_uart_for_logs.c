#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/stat.h>

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

#define LOGS_BUFFER_SIZE                    512
#define ESP_LOG_PIN 						GPIO_NUM_8
#define DEBUG_QUEUE_SIZE 					50

void init_softserial_logs(void);
// int DEBUG_LOGS(const char *format, va_list args);

#define LOGS_BUFFER_SIZE                    512
#define ESP_LOG_PIN 						GPIO_NUM_8
#define DEBUG_QUEUE_SIZE 					50

void init_softserial_logs(void);
// int DEBUG_LOGS(const char *format, va_list args);			

static char tx_buffer[LOGS_BUFFER_SIZE]; // Buffer to hold the data to be transmitted
static size_t tx_index = 0; // Current bit index being transmitted
static size_t bit_position = 0; // Current bit index being transmitted
static size_t tx_length = 0; // Length of the data being transmitted


static QueueHandle_t debug_queue_handle = NULL;
static SemaphoreHandle_t semaphore;
static BaseType_t xHigherPriorityTaskWoken;
static esp_timer_handle_t timer_handle;


typedef struct debug_data {
	char* data;
	size_t len;
}debug_data;

void timer_callback(void *arg){
    if (tx_index < tx_length) {
        bit_position++;  // Move to the next bit
        if (bit_position == 1) {
            gpio_set_level(ESP_LOG_PIN, 0);
        } else if (bit_position > 1 && bit_position <= 9) {
            // Send data bits (inverted logic)
            gpio_set_level(ESP_LOG_PIN, ((tx_buffer[tx_index] >> (bit_position - 2)) & 0x01));  // Invert the logic for TX
        } else if (bit_position == 10) {
            gpio_set_level(ESP_LOG_PIN, 1);
        } else if (bit_position > 10) {
            // End of the byte, move to the next byte
            tx_index++;
            bit_position = 0;  // Reset the bit position for the next byte
        }
    } else if (tx_length > 0){
        tx_length = 0;
        tx_index = 0;
        xSemaphoreGiveFromISR(semaphore, &xHigherPriorityTaskWoken);
    }

}

static int DEBUG_LOGS(const char *format, va_list args) {
	char temp_buff[LOGS_BUFFER_SIZE];
    size_t temp_length;
    
    // Use vsnprintf to format the string and store it in tx_buffer
    temp_length = vsnprintf(temp_buff, LOGS_BUFFER_SIZE, format, args);
    
    debug_data* tx_data = malloc(sizeof(debug_data));
    if (tx_data == NULL) {
        // ESP_LOGE(TAG, "Malloc Failed");
    }
    tx_data->len = temp_length;
    tx_data->data = malloc(tx_data->len*sizeof(char));
    memset(tx_data->data, '\0',tx_data->len*sizeof(char));
    memcpy(tx_data->data,temp_buff,temp_length);
    xQueueSend(debug_queue_handle,(void*)&tx_data, 0);

    return temp_length;
}


static void debug_task(void *params) {
    debug_data* rx_data;

	while (1) {
		if(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE){
			if(xQueueReceive(debug_queue_handle,(void*)&rx_data, portMAX_DELAY)){
	            tx_length = rx_data->len;
                memset(tx_buffer, 0, sizeof(tx_buffer));
	            memcpy(tx_buffer,rx_data->data,tx_length);
	            
	            tx_index = 0;
	            bit_position = 0;

	            if(rx_data->data != NULL) {
	                free(rx_data->data);
	            }
	            if(rx_data != NULL) {
	                free(rx_data);
	            }

	        }
	    }
	}
}


void init_softserial_logs(void){
    // Create a timer configuration
    gpio_set_direction(ESP_LOG_PIN, GPIO_MODE_OUTPUT);

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

    err = esp_timer_start_periodic(timer_handle, 52); // Period in microseconds
    if (err != ESP_OK) {
        // ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(err));
        return;
    }

    semaphore = xSemaphoreCreateBinary();
    if(semaphore == NULL) {
    }
    xSemaphoreGive(semaphore);

    if (debug_queue_handle == NULL) {
        debug_queue_handle = xQueueCreate(DEBUG_QUEUE_SIZE, sizeof(debug_data*));
        if (debug_queue_handle == NULL) {
        }
    }

    xTaskCreate(debug_task, "debug_task", 3072, NULL, 10, NULL);
    esp_log_set_vprintf(DEBUG_LOGS);

}









