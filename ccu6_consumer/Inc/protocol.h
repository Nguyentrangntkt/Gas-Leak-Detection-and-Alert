/**
 * @file    protocol.h
 * @brief   Khung UART giua ba thiet bi. BAN SAO DONG BO voi c8t6_producer/Inc/.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define PROTO_CODE_L0        '0'
#define PROTO_CODE_L1        '1'
#define PROTO_CODE_L2        '2'
#define PROTO_CODE_L3        '3'
#define PROTO_CODE_WARMUP    'W'
#define PROTO_CODE_FAULT     'F'
#define PROTO_CODE_STOPPED   'O'

/* "Chua co trang thai nao" - khong bao gio xuat hien tren day. Dung o
 * phia nhan de vo hieu hoa ma cu; FSM coi no la mat lien ket. */
#define PROTO_CODE_NONE      '\0'

#define PROTO_CMD_START      'I'
#define PROTO_CMD_STOP       'O'

#define PROTO_MAX_LINE       12u

typedef struct {
    char    buf[PROTO_MAX_LINE];
    uint8_t len;
    bool    overflow;
} proto_rx_t;

void proto_rx_init(proto_rx_t *rx);

void proto_rx_discard(proto_rx_t *rx);

bool proto_rx_push(proto_rx_t *rx, char c);

int proto_build_status(char *dst, int cap, char code);
int proto_build_cmd(char *dst, int cap, char cmd);
int proto_build_telemetry(char *dst, int cap, uint32_t ppm, char code);

bool proto_is_status_code(char code);

bool proto_parse_status(const char *line, uint8_t len, char *code_out);

bool proto_parse_cmd(const char *line, uint8_t len, char *cmd_out);

bool proto_code_to_level(char code, uint8_t *level_out);

#endif
