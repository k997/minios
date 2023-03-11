#ifndef __LIB_DEVICE_TIMER_H
#define __LIB_DEVICE_TIMER_H

void timer_init(void);
void sleep(uint32_t seconds);
void sleep_mtime(uint32_t m_seconds);

#endif