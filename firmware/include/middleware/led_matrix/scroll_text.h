#ifndef __SCROLL_TEXT_H__
#define __SCROLL_TEXT_H__
#include <stddef.h>
#include <stdint.h>

void generate_frame(const char* text, size_t len, int scroll_position,
                    uint8_t frame[7][7]);

#endif /*__SCROLL_TEXT_H__*/