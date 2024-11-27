// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// global variable
static int shared_var = 0;

// tasks

// incerement shared variable (the wrong way)
void incTask(void *parameters){
	int local_var;
    
    while(1){
    	// Roundout way to do "shared_var++" randomly and poorly
    	local_var = shared_var;
    	local_var++;
    	vTaskDelay(random(100,500)/portTICK_PERIOD_MS);
    	shared_var = local_var;

    	// print out new shared variable
    	Serial.println(shared_var);
    }
}

void setup(){

	// hack to kind a 
	randomSeed(analogRead(0));

}