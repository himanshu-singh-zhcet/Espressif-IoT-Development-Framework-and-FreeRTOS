// You'll likely need this on vanilla FreeRTOS
//#include "semphr.h"

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
static const int num tasks = 5;  // Number of tasks to create

// Example struct for passing a string as a parameter

typedef struct Message {
  char body[20];
  uint8_t len;
} Message;

// Globals
static SemaphoreHandle_t sem_params;  // Counts down when parameter read 

// Tasks
void myTask(void *parameters){
  // Copy the message struct from the parameter to a local variable
  Message msg = *(Message *)parameters;

  // Increment Semaphore to indicate that the parameter has been read
  xSemaphoreGive(sem_params);

  // Print out message contents 
  Serial.print("Received: ");
  Serial.print(msg.body);
  Serial.print(" | len: ");
  Serial.println(msg.len);

  // wait for a while and delete self
  vTaskDelay(1000/portTICK_PERIOD_MS);
  vTaskDelete(NULL);


}

void setup(){
  char task_name[12];
  Message msg;
  char text[20] = "All your Base";

  // Configure Serial

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Counting Semaphore Demo---");

  // create semaphore (initialize at 0)
  sem_params = xSemaphoreCreateCounting(num_tasks,0);

  // Create message to use as argument common to all tasks
  strcpy(msg.body, text);
  msg.len = strlen(text);

  // Start tasks
  for(int i = 0; i < num_tasks; i++){
    // Generate unique name string for task
    sprintf(task_name,"Task %i",i);

    // start task and pass argument (common Message struct)
    xTaskCreatePinnedToCore(msg,
                          "task_name",
                          1024,
                          (void *)&msg,
                          1,
                          NULL,
                          app_cpu);
  }

  // Wait for all the tasks to read shared memory
  for(int i = 0;i<num_tasks; i++ ){
    xSemaphoreTake(sem_params,portMAX_DELAY);
  }

  // Notify that all task have been created 
  Serial.println("All task created");
}

void loop() {
  // Do nothing but allow yielding to lower priority tasks
  vTaskDelay(1000/portTICK_PERIOD_MS);
}