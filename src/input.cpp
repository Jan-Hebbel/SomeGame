#include "input.hpp"

#include "types.hpp"

Event event_queue[EVENT_QUEUE_SIZE];
bool keyboard_state[KEY_CODE_AMOUNT];

void process_key_event(Key_Code key_code, bool is_down) {
	keyboard_state[key_code] = is_down;
}

void input_add_events(Event_Reader *event_reader) {
	for (int i = 0; i < KEY_CODE_AMOUNT; ++i) {
		if (keyboard_state[i]) {
			event_queue_add({ (Key_Code)i, true }, event_reader);
		}
	}
}

Event event_queue_next(Event_Reader *event_reader) {
	Event dummy = { UNKNOWN, false };

	if (event_reader->index < 0 || event_reader->index >= EVENT_QUEUE_SIZE) {
		return dummy;
	}

	int old_index = event_reader->index++;

	return event_queue[old_index];
}

void event_queue_add(Event new_event, Event_Reader *event_reader) {
	event_queue[event_reader->index++] = new_event;
}

void event_queue_clear() { // @Cleanup: keep this or set event_queue[event_reader->index].key_code to UNKNOWN in event_queue_next()?
	for (int i = 0; i < EVENT_QUEUE_SIZE; ++i)
	{
		event_queue[i] = { UNKNOWN, false };
	}
}
