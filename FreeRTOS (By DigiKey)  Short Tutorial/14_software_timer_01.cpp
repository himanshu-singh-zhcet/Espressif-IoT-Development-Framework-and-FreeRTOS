// you will likely need this in vanilla free rtos
// #include "timer.h"

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Globals
static TimerHandle_t one_shot_timer = NULL;
static TimerHandle_t periodic_timer = NULL;

// Callbacks

// Called when one of timer expires
void myTimerCallback(TimerHandle_t xTimer){
	// print the message if timer 0 expired
	if((uint32_t)pvTimerGetTimerID(xTimer) == 0){
		Serial.println("One Shot Timer Expired");
	}
    
    // print the message if timer 1 expired
	if((uint32_t)pvTimerGetTimerID(xTimer) == 1){
		Serial.println("Periodic Timer Expired");
	}	
}

void setup{
  	// Configure Serial
  	Serial.begin(115200);

  	// Wait a moment to start (so we don't miss Serial output)
  	vTaskDelay(1000 / portTICK_PERIOD_MS);
  	Serial.println();
  	Serial.println("---FreeRTOS Timer Demo---");

  	// create one shot timer
  	one_shot_timer = xTimerCreate("one_shot_timer",2000/portTICK_PERIOD_MS,pdFALSE,(void*)0,myTimerCallback);
    // create periodic timers
  	periodic_timer = xTimerCreate("periodic_timer",1000/portTICK_PERIOD_MS,pdTRUE,(void*)1,myTimerCallback);
  	// check to make sure timers were created
  	if(one_shot_timer == NULL || periodic_timer == NULL){
  		Serial.println("could not create one of the timers");
  	} else{
  		// wait and then printout a message that we are starting the timer
  		vTaskDelay(1000/portTICK_PERIOD_MS);
  		Serial.println("Starting the timer");

  		// Staet the timer (max block time if command queue is full);
  		xTimerStart(one_shot_timer,portMAX_DELAY);
  		xTimerStart(periodic_timer,portMAX_DELAY);
  	}
    
    // Delete self task to show that timers will work with no user tasks
  	vTaskDelete(NULL);
}

void loop(){

}