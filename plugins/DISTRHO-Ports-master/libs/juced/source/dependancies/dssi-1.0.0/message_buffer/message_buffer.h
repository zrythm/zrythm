
/* message_buffer.h

   This file is in the public domain.
*/

#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H

#define MB_MESSAGE(fmt...) { \
	char _m[256]; \
	snprintf(_m, 255, fmt); \
	add_message(_m); \
}

void mb_init(const char *prefix);

void add_message(const char *msg);

#endif
