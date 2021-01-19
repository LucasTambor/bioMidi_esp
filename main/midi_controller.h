#ifndef _MIDI_CONTROLLER_H_
#define _MIDI_CONTROLLER_H_

#include <stdint.h>

#define BIOMIDI_MIDI_CHANNEL    0xA
typedef enum {
    STATE_IDLE = 0,
    STATE_WAITING_CONNECTION,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_RUN,
    STATE_CALIBRATION,
    STATE_SLEEP,
    STATE_MAX,
} midi_controller_state_e;


typedef enum {
    MIDI_STATUS_NOTE_OFF            = 0x8,
    MIDI_STATUS_NOTE_ON             = 0x9,
    MIDI_STATUS_POLY_KEY_PRESSURE   = 0xA,
    MIDI_STATUS_CONTROL_CHANGE      = 0xB,
    MIDI_STATUS_PROGRAM_CHANGE      = 0xC,
    MIDI_STATUS_CHANNEL_PRESSURE    = 0xD,
    MIDI_STATUS_PITCH_BEND          = 0xE,
}midi_voice_status_e;
typedef union {
    struct{
        uint8_t channel : 4;
        uint8_t type : 4;
    }status;
    uint8_t midi_status;
}midi_status_t;

// MIDI General Purpose Controllers
typedef enum {
    MIDI_CONTROLER_GPC_1 = 0x10,
    MIDI_CONTROLER_GPC_2 = 0x11,
    MIDI_CONTROLER_GPC_3 = 0x12,
    MIDI_CONTROLER_GPC_4 = 0x13,
    MIDI_CONTROLER_GPC_5 = 0x50,
    MIDI_CONTROLER_GPC_6 = 0x51,
    MIDI_CONTROLER_GPC_7 = 0x52,
    MIDI_CONTROLER_GPC_8 = 0x53,
}midi_controller_number_e;

typedef struct {
    midi_status_t status;                           // Bn - B = Control Change | n = channel
    midi_controller_number_e control_number;        // General Purpose Controllers number
    uint8_t data;                                    // 0 - 127
} midi_message_t;

void vMidiController( void *pvParameters );


#endif // _MIDI_CONTROLLER_H_