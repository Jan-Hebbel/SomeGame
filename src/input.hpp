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
};

struct Event { // @Incomplete: key_is_down is currently not needed; refactor and keep it or discard it?
	Key_Code key_code;
	bool key_is_down; // if not down then key was released
};

struct Event_Reader { // typedef better? @Cleanup
	int index;
};

Event event_queue_next(Event_Reader *event_reader);
void event_queue_add(Event new_event, Event_Reader *event_reader);
void event_queue_clear();

#endif

// see:  https://youtu.be/AAFkdrP1CHQ?list=PLmV5I2fxaiCI9IAdFmGChKbIbenqRMi6Z&t=3549
// also: https://youtu.be/AAFkdrP1CHQ?list=PLmV5I2fxaiCI9IAdFmGChKbIbenqRMi6Z&t=5379
