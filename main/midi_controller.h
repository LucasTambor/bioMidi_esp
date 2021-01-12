#ifndef _MIDI_CONTROLLER_H_
#define _MIDI_CONTROLLER_H_

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

// typedef struct {
//     uint8_t type;
//     uint8_t note;
//     uint8_t velocity;
// } midi_message_t;

// typedef enum {
//     MIDI_NOTE_C = 60,
// }midi_note_e;

void vMidiController( void *pvParameters );


#endif // _MIDI_CONTROLLER_H_