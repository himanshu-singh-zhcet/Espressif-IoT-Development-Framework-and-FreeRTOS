#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_mac.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include <nvs_flash.h>

#include <esp_bt.h>
#include <esp_blufi_api.h>
#include <esp_blufi.h>

#include <esp_nimble_hci.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <host/util/util.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <console/console.h>


#define BLE_UUID_SERVICE 0x0180
#define BLE_UUID_READ_CHAR 0xFEF4
#define BLE_UUID_WRITE_CHAR 0xDEAD

#define TARGET_SERVER_NAME "BLUAC_B081845224CC" 
// #define TARGET_SERVER_NAME "BLE-Server H"

// Function prototypes
static void ble_app_scan(void);
static void ble_app_advertise(void);
static int ble_on_disc_svc(uint16_t conn_handle, const struct ble_gatt_error *error,const struct ble_gatt_svc *svc, void *arg);
static int ble_on_disc_chr(uint16_t conn_handle, const struct ble_gatt_error *error,const struct ble_gatt_chr *chr, void *arg);
static int ble_on_read(uint16_t conn_handle, const struct ble_gatt_error *error,struct ble_gatt_attr *attr, void *arg);

static const char* TAG = "I_BLUFI";
uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t read_char_handle;
static uint16_t write_char_handle;
uint8_t ble_addr_type;
struct ble_hs_adv_fields fields;
static bool ble_mode = false;   // true for BLE_SERVER Mode,false for BLE CLIENT


/**********  BLE   ***********/
/** BLE Client  **/
// Read callback function
static int ble_on_read(uint16_t conn_handle, const struct ble_gatt_error *error,struct ble_gatt_attr *attr, void *arg){
    if (error->status == 0)
    {
        ESP_LOGI(TAG, "Data from server: %.*s", attr->om->om_len, attr->om->om_data);
    }
    return 0;
}

// Function to send data to the server
void ble_send_data(const char *data){
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, strlen(data));
    if (om){
        ble_gattc_write_flat(conn_handle, write_char_handle, data, strlen(data), NULL, NULL);
        ESP_LOGI(TAG, "Sent data: %s", data);
    }
}

// Function to handle discovered services
static int ble_on_disc_svc(uint16_t conn_handle, const struct ble_gatt_error *error,const struct ble_gatt_svc *svc, void *arg){
    if (error->status == 0){
        ESP_LOGI(TAG, "Service discovered, UUID: 0x%04x", ble_uuid_u16(&svc->uuid.u));
        if (ble_uuid_u16(&svc->uuid.u) == BLE_UUID_SERVICE){
            ble_gattc_disc_all_chrs(conn_handle, svc->start_handle, svc->end_handle, ble_on_disc_chr, NULL);
        }
    }
    return 0;
}

// Function to handle discovered characteristics
static int ble_on_disc_chr(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg){
    if (error->status == 0){
        uint16_t uuid = ble_uuid_u16(&chr->uuid.u);
        ESP_LOGI(TAG, "Characteristic discovered, UUID: 0x%04x", uuid);
        if (uuid == BLE_UUID_READ_CHAR){
            read_char_handle = chr->val_handle;
            ble_gattc_read(conn_handle, read_char_handle, ble_on_read, NULL);
        }
        else if (uuid == BLE_UUID_WRITE_CHAR){
            write_char_handle = chr->val_handle;
            ble_send_data("Hello from Client");
        }
    }
    return 0;
}



/* BLE Event Handling */
static int ble_gap_event(struct ble_gap_event *event, void *arg){
    struct ble_gap_conn_params conn_params;
    memset(&conn_params, 0, sizeof(conn_params));
    int ret = 0;

    switch (event->type){
    case BLE_GAP_EVENT_DISC:
        ESP_LOGI(TAG, "GAP EVENT DISCOVERY");
        ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        
        if (fields.name_len > 0){
            printf("Discovered %.*s\n", fields.name_len, fields.name);

            if (strncmp((char *)fields.name, TARGET_SERVER_NAME, fields.name_len) == 0){
                ESP_LOGI(TAG,"Target BLE Server found! Connecting...");
                ble_gap_disc_cancel();

                conn_params.scan_itvl = BLE_GAP_SCAN_FAST_INTERVAL_MAX;
                conn_params.scan_window = BLE_GAP_SCAN_FAST_WINDOW;
                conn_params.itvl_min = BLE_GAP_INITIAL_CONN_ITVL_MIN;
                conn_params.itvl_max = BLE_GAP_INITIAL_CONN_ITVL_MAX;
                conn_params.latency = BLE_GAP_INITIAL_CONN_LATENCY;
                conn_params.supervision_timeout = BLE_GAP_INITIAL_SUPERVISION_TIMEOUT;
                conn_params.min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN;
                conn_params.max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN;

                ret = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 1000, &conn_params, ble_gap_event, NULL);
                vTaskDelay(pdMS_TO_TICKS(400));

                if (ret != 0){
                    ESP_LOGI(TAG,"Failed to initiate connection error: %d",ret);
                }
            }
            else{
                printf("Skipping non-target device: %.*s\n", fields.name_len, fields.name);
            }
        }
        break;

    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "GAP EVENT CONNECTED");
        conn_handle = event->connect.conn_handle;
        printf("Connected with %.*s\n", fields.name_len, fields.name);
        if (event->connect.status == 0){
            ESP_LOGI(TAG, "Connected to BLE-Server H!");
            conn_handle = event->connect.conn_handle;
            ble_gattc_disc_all_svcs(conn_handle, ble_on_disc_svc, NULL);
        }
        else{
            ESP_LOGI(TAG, "Connection failed, restarting scan...");
            ble_app_scan();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "GAP EVENT DISCONNECTED");
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        ble_app_scan();
        break;

    default:
        break;
    }
    return 0;
}
void ble_app_scan(void){
    struct ble_gap_disc_params disc_params;

    ESP_LOGI(TAG,"Start Scanning");

    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;
    disc_params.itvl = 128;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}

static void stop_ble(void) {
    ESP_LOGI(TAG, "Stopping BLE...");

    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "Disconnecting ");
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    int ret = nimble_port_stop();  // Stop the BLE stack
    if (ret != 0) {
        ESP_LOGE(TAG, "nimble_port_stop() failed: %d", ret);
        return;
    }

    nimble_port_deinit();
    esp_nimble_hci_deinit();
    // Ensure no BLE tasks are running

    vTaskDelay(pdMS_TO_TICKS(500));
}


/**** Callbacks for BLE Client  *****/
static void blecent_on_reset(int reason){
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}


static void blecent_on_sync(void){
    ble_hs_id_infer_auto(0, &ble_addr_type);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ble_app_scan();                          
}

/** Function to Start BLE ***/
void ble_host_task(void *param){
    ESP_LOGI(TAG, "BLE Host Task Started for BLE Server");
    nimble_port_run();
    nimble_port_freertos_deinit();
    vTaskDelay(pdMS_TO_TICKS(500));
}

/** Function to Init BLE Client ***/
static esp_err_t ble_client_init(void *thing_handle) {
    tThingInfo *thingInfo= thing_handle;
    if(thingInfo == NULL) {
        ESP_LOGE(TAG, "DEVICE NULL in blecent_host_init");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Starting BLE Client...");

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
    ble_hs_cfg.reset_cb = blecent_on_reset;
    ble_hs_cfg.sync_cb = blecent_on_sync;

    ble_svc_gap_init();                             
    nimble_port_freertos_init(ble_host_task); 

    ble_mode = false;          
    return ESP_OK;
}



