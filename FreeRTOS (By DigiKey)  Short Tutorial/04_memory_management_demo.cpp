// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif


// Task: Perform some Mundane task
void test_task(void *parameter){
	while(1){
		int a = 1;
		int b[100];
		// Do something with array it is not optimized out by the compiler
		for(int i = 0;i<100;i++){
			b[i] = a+1;
		}
		Serial.println(b[0]);

		// Print out the remaining stack memory (in words)
		Serial.print("High Water Mark (words): ");
		Serial.println(uxTaskGetStackHighWaterMark(NULL));

		// Print out number of free heap memory bytes before malloc
		Serial.print("Heap Before malloc (bytes): ");
		Serial.println(xPortGetFreeHeapSize());

		int *ptr = (int*)pvPortMalloc(1024*sizeof(int));
		if(ptr == NULL){
			Serial.println("Not Enough Heap");
		} else{
			for(int i = 0; i<1024; i++){
				ptr[i] = 3;
			}
		}

		// Print out number of free heap memory bytes after malloc
		Serial.print("Heap after malloc (bytes): ");
		Serial.println(xPortGetFreeHeapSize());
        
        vPortFree(ptr);
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void setup(){
	//Configure Serial
	Serial.begin(115200);

	// wait a moment to start (so we don't miss serial output)
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	Serial.println();
    Serial.println("FreeRTOS Memory Demo");

    // Start the only other task 
    xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
            test_task,      // Function to be called
            "TEST TASK",   // Name of task
            1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
            NULL,           // Parameter to pass
            1,              // Task priority
            NULL,           // Task handle
            app_cpu);       // Run on one core for demo purposes (ESP32 only)
            

}

void loop(){

}