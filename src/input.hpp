/*
* Here is how input works for this game:
* 
* Once per frame platform_process_events() is called. This function
* polls Windows for events. If the event was a Keydown or 
* Syskeydown event we set the local keyboard state for the key to 
* true. It stays that way until the key is released and Windows 
* sends a new event, that's when the keyboard state for that key is
* set to false. 
* 
* Poll the internal keyboard state to get the position of key for 
* every frame.
* 
* The other system in place is the one that just sends events as 
* Windows does and sends stores them in an Event queue. These can 
* be polled with event_queue_next().
* 
* Use this when it's not important or unwanted to check for a key
* state every frame. Like for example when pressing Esc to enter 
* the menu.
*/

#ifndef INPUT_H
#define INPUT_H

#include "types.hpp"

const int EVENT_QUEUE_SIZE = 10;

enum Key_Code {
	UNKNOWN = 0,
	ESCAPE  = 1,
	W = 2,
	A = 3,
	S = 4,
	D = 5,
	SPACE = 6,
	KEY_CODE_AMOUNT = 7
};

struct Key_State {
	bool is_down;
	bool released;
	bool repeated;
	bool alt_down;
};

struct Event { 
	Key_Code key_code;
	Key_State key_state;
};

struct Event_Reader { // typedef better? @Cleanup
	int index;
};

void process_key_event(Key_Code key_code, Key_State key_state, Event_Reader *event_reader);
Event event_queue_next(Event_Reader *event_reader);
Key_State get_key_state(Key_Code key_code);
void reset_keyboard_state();

#endif

// see:  https://youtu.be/AAFkdrP1CHQ?list=PLmV5I2fxaiCI9IAdFmGChKbIbenqRMi6Z&t=3549
// also: https://youtu.be/AAFkdrP1CHQ?list=PLmV5I2fxaiCI9IAdFmGChKbIbenqRMi6Z&t=5379
