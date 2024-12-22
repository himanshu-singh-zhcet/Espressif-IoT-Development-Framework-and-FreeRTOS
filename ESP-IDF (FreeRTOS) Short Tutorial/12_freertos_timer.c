#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timer.h"

#define SECOND_TIMER_NAME      "one_second_timer"
#define SECOND_TIMER_TIMEOUT   1000

TimerHandle_t timer_handler;

void one_second_timer(TimerHandle_t xTimer){
  printf("in one second timer \n");
}

void enable_one_second_timer(void) {
  timer_handler = xTimerCreate(SECOND_TIMER_NAME, pdMS_TO_TICKS(SECOND_TIMER_TIMEOUT), 
                       pdTRUE, NULL, one_second_timer
                      );

  // Check if the timer was created successfully
  if(timer_handler != NULL) {
    // Start the timer
    if (xTimerStart(timer_handler, 0) != pdPASS) {
      // Handle error if the timer could not be started
    }
  }
  else{
    // Handle error if the timer creation failed
  }

}