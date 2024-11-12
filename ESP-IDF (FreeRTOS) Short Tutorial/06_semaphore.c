#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

TaskHandle_t myTask1Handle = NULL;
TaskHandle_t myTask2Handle = NULL;


void task1(void *args){
	while(1){
        printf("sent message! [%d] \n",xTaskGetTickCount());
        xSemaphoreGive(xSemaphore);
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

void task2(void *args){
	while(1){
		if(xSemaphoreTake(xSemaphore,portMAX_DELAY)) {
			printf("got message! [%d] \n",xTaskGetTickCount());
		}
	}
}


void app_main(){
    xSemaphore = xSemaphoreCreateBinary();

	xTaskCreate(task1, "task1", 4096, NULL, 10, &myTask1Handle);
	xTaskCreatePinnedToCore(task2, "task2", 4096, NULL, 10, &myTask2Handle,1); 
}



