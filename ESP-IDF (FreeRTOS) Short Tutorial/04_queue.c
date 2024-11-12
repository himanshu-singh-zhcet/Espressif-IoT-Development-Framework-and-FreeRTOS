#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

TaskHandle_t myTask1Handle = NULL;
TaskHandle_t myTask2Handle = NULL;
QueueHandle_t queue1;


void task1(void *args){
	char txbuff[50];
	queue1= xQueueCreate(5, sizeof(txbuff));
	if(queue1 == 0){
		printf("failed to create queue1= %p \n",queue1); // Failed to create the queue.
	}
	sprintf(txbuff,"Hello World 1");
	xQueueSend(queue1, (void*)txbuff , (TickType_t)0 );
	sprintf(txbuff,"Hello World 2");
	xQueueSend(queue1, (void*)txbuff , (TickType_t)0 );
	sprintf(txbuff,"Hello World 3");
	xQueueSendToFront(queue1, (void*)txbuff , (TickType_t)0 );
	while(1){
		printf("data waiting to be read : %d  available spaces: %d \n",uxQueueMessagesWaiting(queue1),uxQueueSpacesAvailable(queue1));
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

void task2(void *args){
	char rxbuff[]
	while(1){
		if(xQueueReceive(queue1, &(rxbuff) , (TickType_t)5 )){ 
            printf("got a data from queue!  ===  %s \n",rxbuff);
        }
	}
}


void app_main(){
	xTaskCreate(task1, "task1", 4096, NULL, 10, &myTask1Handle);
	xTaskCreatePinnedToCore(task2, "task2", 4096, NULL, 10, &myTask2Handle,1); 
}
