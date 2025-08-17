#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "driver/gpio.h"

#define TAG "CAN"

#define TX_GPIO_NUM 5
#define RX_GPIO_NUM 4

#define CAN_TASK_NAME      "can_task"
#define CAN_TASK_SIZE      4096
#define CAN_TASK_PRIO      7

static TaskHandle_t can_task_handle = NULL;

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);

// Your custom CAN transmit structure
typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    bool is_extended;
    bool is_remote;
} can_message_t;

void init_can(void){
    // Install CAN driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "Driver installed");
    } else {
        ESP_LOGE(TAG, "Failed to install driver");
        return;
    }

    // Start CAN driver
    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "CAN started");
    } else {
        ESP_LOGE(TAG, "Failed to start CAN");
        return;
    }
}

void can_task(void *arg) {
    twai_message_t rx_msg;
    uint8_t count = 0;

    while (1) {
        // Wait for a CAN message
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(500)) == ESP_OK) {
            count = count +1;
            bool is_extended = (rx_msg.flags & TWAI_MSG_FLAG_EXTD) != 0;
            bool is_remote = (rx_msg.flags & TWAI_MSG_FLAG_RTR) != 0;

            ESP_LOGI(TAG, "Received %s %s frame: ID=0x%X, DLC=%d",
                     is_extended ? "Extended" : "Standard",
                     is_remote ? "Remote" : "Data",
                     rx_msg.identifier,
                     rx_msg.data_length_code);

            if (!is_remote) {
                for (int i = 0; i < rx_msg.data_length_code; i++) {
                    printf("  Byte %d: 0x%02X\n", i, rx_msg.data[i]);
                }
            }

            can_message_t tx_msg;
            tx_msg.id = 0x04008000;
            tx_msg.dlc = 5;  // Sending only 4 bytes (ID)
            tx_msg.is_extended = is_extended;
            tx_msg.is_remote = false;


            if(is_extended){
                tx_msg.data[0] = (rx_msg.identifier >> 24) & 0xFF;
                tx_msg.data[1] = (rx_msg.identifier >> 16) & 0xFF;
                tx_msg.data[2] = (rx_msg.identifier >> 8) & 0xFF;
                tx_msg.data[3] = (rx_msg.identifier) & 0xFF;
                tx_msg.data[4] = count;
            } else{
                tx_msg.data[0] = (rx_msg.identifier >> 8) & 0xFF;
                tx_msg.data[1] = (rx_msg.identifier) & 0xFF;
                tx_msg.data[2] = count;
                tx_msg.data[3] = 0;
                tx_msg.data[4] = 0;
            }

            twai_message_t send_msg = {
                .identifier = tx_msg.id,
                .data_length_code = tx_msg.dlc,
                .flags = tx_msg.is_extended ? TWAI_MSG_FLAG_EXTD : 0
            };
            memcpy(send_msg.data, tx_msg.data, tx_msg.dlc);

            if (twai_transmit(&send_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
                ESP_LOGI(TAG, "Replied to ID=0x%X with ID in payload", tx_msg.id);
            } else {
                ESP_LOGW(TAG, "Failed to send response");
            }
        } else {
            ESP_LOGW(TAG, "No message received");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void init_can_task(){
    if (xTaskCreate(can_task, CAN_TASK_NAME, CAN_TASK_SIZE, NULL, CAN_TASK_PRIO, &can_task_handle) == pdPASS) {
        ESP_LOGI(TAG, "CAN RX/TX task created");
    } else {
        ESP_LOGE(TAG, "Failed to create CAN RX/TX task");
    }
}

void app_main(void){
    ESP_LOGI(TAG,"app_main");
    init_can();
    init_can_task();
}