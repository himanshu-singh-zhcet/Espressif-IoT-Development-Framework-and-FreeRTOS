#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"


#define BLE_UUID_SERVICE 0x0180
#define BLE_UUID_READ_CHAR 0xFEF4
#define BLE_UUID_WRITE_CHAR 0xDEAD

char *TAG = "BLE-Server";
uint8_t ble_addr_type;
void ble_app_advertise(void);
/**  BLE Server **/
/* Write data */
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg){
    int data_len = OS_MBUF_PKTLEN(ctxt->om);  // Get total data length
    uint8_t buffer[data_len + 1];  // Allocate buffer for full data
    memset(buffer, 0, sizeof(buffer));

    int offset = 0;
    struct os_mbuf *om = ctxt->om;
    
    while (om) {
        memcpy(&buffer[offset], om->om_data, om->om_len);
        offset += om->om_len;
        om = SLIST_NEXT(om, om_next);
    }

    buffer[data_len] = '\0';  // Null-terminate for printing
    printf("Data from the client: %s\n", buffer);
    
    return 0;
}
/* Read data*/
static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg){
    os_mbuf_append(ctxt->om, "Data from the server", strlen("Data from the server"));
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x0180),                 // Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(BLE_UUID_READ_CHAR),           // Define UUID for reading
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(BLE_UUID_WRITE_CHAR),           // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};

/* BLE event handling */
static int ble_gap_event_cb(struct ble_gap_event *event, void *arg){
    switch (event->type){
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

void ble_app_advertise(void){
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); // Read the BLE device name
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
}

/**** Callbacks for BLE Client  *****/
static void bleprph_on_reset(int reason){
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

static void bleprph_on_sync(void){
    ble_hs_id_infer_auto(0, &ble_addr_type);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ble_app_advertise();                         
}

/** Function to Init BLE Server ***/
static esp_err_t ble_server_init(void *thing_handle) {
    tThingInfo *thingInfo= thing_handle;
    if(thingInfo == NULL) {
        ESP_LOGE(TAG, "DEVICE NULL in blecent_host_init");
        return ESP_FAIL;
    }
    ESP_LOGI("I_BLUFI", "Starting BLE Server...");

    ESP_ERROR_CHECK(esp_nimble_hci_init());
    nimble_port_init();

    /* Set the default device name. */
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char *name_payload = NULL;
    int ret = asprintf(&name_payload, "BLUAC_%02X%02X%02X%02X%02X%02X",
                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if(ret ==  -1) {
        ESP_LOGE(TAG, "name_payload failed");
        return ESP_FAIL;
    }

    ret = ble_svc_gap_device_name_set(name_payload);
    if(ret != 0) {
        ESP_LOGE(TAG, "ble_svc_gap_device_name_set failed");
        return ESP_FAIL;
    }

    free(name_payload);

    /* Configure the host. */
    ble_hs_cfg.reset_cb = bleprph_on_reset;
    ble_hs_cfg.sync_cb = bleprph_on_sync;

    ble_svc_gap_init();                       
    ble_svc_gatt_init();                      
    ble_gatts_count_cfg(gatt_svcs);          
    ble_gatts_add_svcs(gatt_svcs);                          
    nimble_port_freertos_init(ble_host_task); 

    ble_mode = true;          
    return ESP_OK;
}


void app_main(){
    nvs_flash_init();
    ble_server_init();
}