#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint8_t msg_queue_len = 5;

// globals
static QueueHandle_t msg_queue;

// Task: Wait for item on queue and print it
void printMessages(void *parameters){
	int item;
	while(1){
		// see if there is a message in queue (do not block)
		if(xQueueReceive(msg_queue,(void *)&item,0) == pdTRUE){
			Serial.println(item);
		}

		vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}

void setup(){
	Serial.begin(115200);
    // wait a moment to start (so we dont miss serial output)
	vTaskDelay(1000/portTICK_PERIOD_MS);

	msg_queue = xQueueCreate(msg_queue_len,sizeof(int));

	xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
              printMessages,  // Function to be called
              "Print Message",   // Name of task
              1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
              NULL,         // Parameter to pass to function
              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
              NULL,         // Task handle
              app_cpu);     // Run on one core for demo purposes (ESP32 only)
}

void loop(){
	static int num = 0;

	// Try to add item to the queue for 10 ticks, fail if queue is full
	if(xQueueSend(msg_queue,(void *)&num,10)!= pdTRUE){
		Serial.println("Queue is Full");
	}
	num++;

	vTaskDelay(1000/portTICK_PERIOD_MS);
}