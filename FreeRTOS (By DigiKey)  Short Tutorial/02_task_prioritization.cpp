
// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Some string to print
const char msg[] = "This is Himanshu Singh"

// Task handles
static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;

// Task: print to Serial Terminal With Lower Priority
void startTask1(void *parameter){
	// count the number of characters in string
	int msg_len = strlen(msg);
    
    // print the string to the terminal
    while(1){
    	Serial.println();
    	for(int i = 0; i<msg_len;i++){
    		Serial.print(msg[i]);
    	}
    	Serial.println();
    	vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

// Task: print to Serial Terminal With Higher Priority
void startTask2(void *parameter){
    while(1){
    	Serial.print('*');
    	vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void setup(){
	Serial.begin(300);

	// wait a moment to start (so we don't miss a serial output)
	vTaskDelay(1000/portTICK_PERIOD_MS);
	Serial.println();
	Serial.println("FreeRTOS Task Demo");

	// print self priority
	Serial.print("Setup and loop task running on core ");
	Serial.print(xPortGetCoreID());
	Serial.print(" With Priority ");
	Serial.println(uxTaskPriorityGet(NULL));

	// Task to run Forever
	xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
        startTask1,   // Function to be called
        "Task 1",     // Name of task
        1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
        NULL,         // Parameter to pass to function
        1,            // Task priority (0 to configMAX_PRIORITIES - 1)
        &task_1,         // Task handle
        app_cpu);     // Run on one core for demo purposes (ESP32 only)

    // Task to run once with higher priority
    xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
        startTask2,   // Function to be called
        "Task 2",     // Name of task
        1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
        NULL,         // Parameter to pass to function
        1,            // Task priority (0 to configMAX_PRIORITIES - 1)
        &task_2,         // Task handle
        app_cpu);     // Run on one core for demo purposes (ESP32 only)

}

void loop(){
	// Suspend the higher priority task for some intervals
	for(int i = 0;i<3;i++){
		vTaskSuspend(task_2);
		vTaskDelay(2000/portTICK_PERIOD_MS);
		vTaskResume(task_2);
		vTaskDelay(2000/portTICK_PERIOD_MS);
	}

	// Delete the lower Priority Task
	if(task_1 != NULL){
		vTaskDelete(task_1);
		task_1 = NULL;
	}
}