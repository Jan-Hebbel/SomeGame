#include "input.hpp"

#include "types.hpp"
#include "platform.hpp"

Event event_queue[EVENT_QUEUE_SIZE];
Key_State keyboard_state[KEY_CODE_AMOUNT];

void process_key_event(Key_Code key_code, Key_State key_state, Event_Reader *event_reader) {
	keyboard_state[key_code] = key_state; // set keyboard_state (this is for moving and the similar, since this works every frame)
	event_queue[event_reader->index++] = { key_code, key_state }; // adding events to the event queue; only called every few frames
}

Event event_queue_next(Event_Reader *event_reader) {
	Key_State key_state_all_false = {};
	Event dummy = { UNKNOWN, key_state_all_false };

	if (event_reader->index < 0 || event_reader->index >= EVENT_QUEUE_SIZE) {
		return dummy;
	}

	int old_index = event_reader->index++;

	Event old_event = event_queue[old_index];
	event_queue[old_index] = { UNKNOWN, key_state_all_false };
	return old_event;
}

Key_State get_key_state(Key_Code key_code) {
	return keyboard_state[key_code];
}
