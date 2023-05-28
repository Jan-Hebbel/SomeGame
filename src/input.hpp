/*
* Here is how input works for this game:
* 
* Once per frame platform_process_events() is called. This function
* polls Windows for events. If the event was a Keydown or 
* Syskeydown event we set the local keyboard state for the key to 
* true. Once the PeekMessage-Loop is done, we call the 
* input_add_events() function to add events to event_queue for all 
* keys that got set to true in keyboard_state in that PeekMessage-
* Loop. 
* 
* Later in the game_update() function we empty out the queue that
* holds all events that got added for this frame by calling 
* event_queue_next(). This way we keep track of the state of all
* keys that are down for one frame and handle it later in the 
* game_update() function.
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

struct Event { // @Incomplete: key_is_down is currently not needed; refactor and keep it or discard it?
	Key_Code key_code;
	bool key_is_down; // if not down then key was released
};

struct Event_Reader { // typedef better? @Cleanup
	int index;
};

void process_key_event(Key_Code key_code, bool is_down);
void input_add_events(Event_Reader *event_reader);
Event event_queue_next(Event_Reader *event_reader);
void event_queue_add(Event new_event, Event_Reader *event_reader);
void event_queue_clear();

#endif

// see:  https://youtu.be/AAFkdrP1CHQ?list=PLmV5I2fxaiCI9IAdFmGChKbIbenqRMi6Z&t=3549
// also: https://youtu.be/AAFkdrP1CHQ?list=PLmV5I2fxaiCI9IAdFmGChKbIbenqRMi6Z&t=5379
