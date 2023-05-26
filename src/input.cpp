#include "input.hpp"

#include "types.hpp"

Event event_queue[EVENT_QUEUE_SIZE];

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

void event_queue_clear() {
	for (int i = 0; i < EVENT_QUEUE_SIZE; ++i)
	{
		event_queue[i] = { UNKNOWN, false };
	}
}
