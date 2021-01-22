#ifndef _BLE_BATTERY_SERVICE_
#define _BLE_BATTERY_SERVICE_

#include "stdint.h"
#include "esp_gatt_defs.h"
#include <string.h>

/* Enumeration of the services and characteristics */
enum
{
    BATTERY_IDX_SERVICE,
    BATTERY_IDX_CHAR,
    BATTERY_IDX_VAL,
    BATTERY_IDX_CONFIG,

    BATTERY_IDX_MAX,
};
uint16_t battery_handle_table[BATTERY_IDX_MAX];

extern const uint16_t battery_service_uuid;
extern const esp_gatts_attr_db_t battery_serv_gatt_db[BATTERY_IDX_MAX];

uint16_t getAttributeIndexByBatteryHandle(uint16_t attributeHandle);

// void handleMIDIReadEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp);
void handleBatteryWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len);
void handleBatteryMtuEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len);


#endif //_BLE_BATTERY_SERVICE_