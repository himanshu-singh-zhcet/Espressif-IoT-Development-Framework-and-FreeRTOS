#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TASK1_TAG  "TASK_1"
#define TASK2_TAG  "TASK_2"

TaskHandle_t myTask1Handle = NULL;
TaskHandle_t myTask2Handle = NULL;


void task1(void *args){
	while(1){
		ESP_LOGI(TASK1_TAG,"Hello from Task 1");
		vTaskDelay(pdMS_TO_TICKS(1000)); 
	}
}

void task2(void *args){
	for(int i =0;i++;i<5){
		ESP_LOGI(TASK2_TAG,"Hello from Task 2");
		vTaskDelay(pdMS_TO_TICKS(1000)); 
	}
}


void app_main(){
	xTaskCreate(task1, "task1", 4096, NULL, 10, &myTask1Handle);
	xTaskCreatePinnedToCore(task2, "task2", 4096, NULL, 10, &myTask2Handle,1); 
}
