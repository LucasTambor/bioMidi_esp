#include "ble_battery_service.h"

#include "ble_common.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

//********************************************************************************************


//********************************************************************************************

static const char * TAG = "BLE_BATTERY_SERVICE";

// static const uint8_t char_value                 = 0;
static const uint8_t blebattery_ccc[2]             = {0x00, 0x00}; // Client Characteristic Configuration (CCC) descriptor which is an additional attribute that describes if the characteristic has notifications enabled.



const uint16_t battery_service_uuid = ESP_GATT_UUID_BATTERY_SERVICE_SVC;

static const uint16_t battery_characteristics_uuid = ESP_GATT_UUID_BATTERY_LEVEL;

/* Full Database Description - Used to add attributes into the database */
const esp_gatts_attr_db_t battery_serv_gatt_db[BATTERY_IDX_MAX] =
{
    // Service Declaration
    [BATTERY_IDX_SERVICE]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(battery_service_uuid), (uint8_t *)&battery_service_uuid}},

    /* Characteristic Declaration */
    [BATTERY_IDX_CHAR]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [BATTERY_IDX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&battery_characteristics_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint8_t), 0, NULL}},

    /* Client Characteristic Configuration Descriptor (this is a BLE2902 descriptor) */
    [BATTERY_IDX_CONFIG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(blebattery_ccc), (uint8_t *)blebattery_ccc}},

};



//**************************************************************************************************************

uint16_t getAttributeIndexByBatteryHandle(uint16_t attributeHandle) {
	// Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = BATTERY_IDX_MAX;
    uint16_t index;

    for(index = 0; index < BATTERY_IDX_MAX; ++index)
    {
        if( battery_handle_table[index] == attributeHandle )
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

void handleBatteryWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len) {
    switch(attrIndex) {
        case BATTERY_IDX_VAL:
            ESP_LOGI(TAG, "GATT_WRITE_EVT, value len = %d, value :", char_val_len);
            esp_log_buffer_hex(TAG, char_val_p, char_val_len);
            // blemidi_receive_packet(0, char_val_p, char_val_len, blemidi_callback_midi_message_received);
        break;
    }

}
