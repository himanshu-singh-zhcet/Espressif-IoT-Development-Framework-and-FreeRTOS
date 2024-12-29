#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define CONFIG_BUTTON_PIN 0
#define TASK1_TAG "TASK_1"
#define TASK2_TAG "TASK_2"

TaskHandle_t myTask1Handle = NULL;
TaskHandle_t myTask2Handle = NULL;
TaskHandle_t ISR = NULL;

void IRAM_ATTR button_isr_handler(void* arg) {
   xTaskResumeFromISR(ISR);
}

// task that will react to button clicks
void button_task(void *arg){
   while(1){  
      vTaskSuspend(NULL);
      ESP_LOGE("interrupt","Button pressed!!!\n");
   }
}


void task1(void *arg){
   char buffer[20]= { 0x11,0x22,0x33,0x44,0x55,0x11,0x22,0x33,0x44,0x55,0x11,0x22,0x33,0x44,0x55,0x11,0x22,0x33,0x44,0x55};
   while(1){  
      ESP_LOGI( "TASK_1" ,"hello from task 1 \n");
      vTaskDelay(pdMS_TO_TICKS(1000));
      ESP_LOG_BUFFER_HEX("buff",buffer,20);
      printf( "timestamp= %d  tick count= %d \n" , esp_log_timestamp(), xTaskGetTickCount() );
   }
}

void task2(void *arg){
   while(1){  
      ESP_LOGW( TASK2_TAG ,"hello from task 2 \n");
      vTaskDelay(pdMS_TO_TICKS(1000));
   }
}

void app_main(){
   gpio_pad_select_gpio(CONFIG_BUTTON_PIN);
  
   // set the correct direction
   gpio_set_direction(CONFIG_BUTTON_PIN, GPIO_MODE_INPUT);
  
   // enable interrupt on falling (1->0) edge for button pin
   gpio_set_intr_type(CONFIG_BUTTON_PIN, GPIO_INTR_NEGEDGE);

  
   //Install the driver’s GPIO ISR handler service, which allows per-pin GPIO interrupt handlers.
   // install ISR service with default configuration
   gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  
   // attach the interrupt service routine
   gpio_isr_handler_add(CONFIG_BUTTON_PIN, button_isr_handler, NULL);

   xTaskCreate(task1, "task1", 4096, NULL, 10, &myTask1Handle);
   xTaskCreatePinnedToCore(task2, "task2", 4096, NULL, 10, &myTask2Handle,1);
   xTaskCreate( button_task, "button_task", 4096, NULL , 10,&ISR );

}