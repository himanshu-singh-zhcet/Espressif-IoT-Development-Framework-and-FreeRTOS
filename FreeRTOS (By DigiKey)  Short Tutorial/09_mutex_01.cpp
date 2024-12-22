// you will likely need this in vanilla free rtos
// #include "semphr.h"

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// global variable
static int shared_var = 0;
static SemaphoreHandle_t mutex;


// tasks

// incerement shared variable (the wrong way)
void incTask(void *parameters){
	int local_var;
    
    while(1){

        // take mutex prior to critcial condition
        if(xSemaphoreTake(mutex,0) == pdTRUE){ // for 0 tick value it will return immediately
        	// Roundout way to do "shared_var++" randomly and poorly
        	local_var = shared_var;
        	local_var++;
        	vTaskDelay(random(100,500)/portTICK_PERIOD_MS);
        	shared_var = local_var;
            
            // give mutex after critical section 
            xSemaphoreGive(mutex);
            
        	// print out new shared variable
        	Serial.println(shared_var);
        }
    }
}

void setup(){
	// hack to kinda get randomness
	randomSeed(analogRead(0));
	vTaskDelay(1000/portTICK_PERIOD_MS);

	// create mutex before starting tasks
	mutex = xSemaphoreCreateMutex();
	
    

	// start task 1
	xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
              incTask,  // Function to be called
              "Increment Task 1",   // Name of task
              1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
              NULL,         // Parameter to pass to function
              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
              NULL,         // Task handle
              app_cpu);     // Run on one core for demo purposes (ESP32 only)

    
    // start task 2
	xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
              incTask,  // Function to be called
              "Increment Task 2",   // Name of task
              1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
              NULL,         // Parameter to pass to function
              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
              NULL,         // Task handle
              app_cpu);     // Run on one core for demo purposes (ESP32 only)

	// delete the setup and loop task
	vTaskDelay(NULL);
}

void loop(){

}