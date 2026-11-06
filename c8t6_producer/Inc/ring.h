/**
 * @file    ring.h
 * @brief   So hoc chi so ring buffer. BAN SAO DONG BO voi ccu6_consumer/Inc/.
 */
#ifndef RING_H
#define RING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

static inline uint32_t ring_used(uint16_t head, uint16_t tail, uint16_t size)
{
    return (head >= tail)
         ? (uint32_t)(head - tail)
         : ((uint32_t)size - (uint32_t)tail + (uint32_t)head);
}

static inline uint32_t ring_room(uint16_t head, uint16_t tail, uint16_t size)
{
    return (uint32_t)size - 1u - ring_used(head, tail, size);
}

static inline uint16_t ring_next(uint16_t i, uint16_t size)
{
    return (uint16_t)((i + 1u) % size);
}

static inline bool ring_write_frame(char *buf, volatile uint16_t *head,
                                    const volatile uint16_t *tail,
                                    uint16_t size, const char *data, int len)
{
    if (len <= 0) {
        return true;
    }
    uint16_t next = *head;
    if ((uint32_t)len > ring_room(next, *tail, size)) {
        return false;
    }
    for (int i = 0; i < len; i++) {
        buf[next] = data[i];
        next = ring_next(next, size);
    }
    atomic_signal_fence(memory_order_release);
    *head = next;
    return true;
}

static inline bool ring_read_byte(const char *buf,
                                  const volatile uint16_t *head,
                                  volatile uint16_t *tail, uint16_t size,
                                  char *out)
{
    const uint16_t current = *tail;
    if (current == *head) {
        return false;
    }
    atomic_signal_fence(memory_order_acquire);
    *out = buf[current];
    atomic_signal_fence(memory_order_release);
    *tail = ring_next(current, size);
    return true;
}

#endif
