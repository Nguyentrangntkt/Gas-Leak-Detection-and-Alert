/**
 * @file    protocol.c
 * @brief   Dong va mo khung UART. BAN SAO DONG BO voi ccu6_consumer/Src/.
 */
#include "protocol.h"

void proto_rx_init(proto_rx_t *rx)
{
    rx->len = 0u;
    rx->overflow = false;
}

void proto_rx_discard(proto_rx_t *rx)
{
    rx->len = 0u;
    rx->overflow = true;
}

bool proto_rx_push(proto_rx_t *rx, char c)
{
    if (c == '\r') {
        return false;
    }

    if (c == '\n') {
        const bool ok = (!rx->overflow) && (rx->len > 0u);
        if (!ok) {
            rx->len = 0u;
            rx->overflow = false;
        }
        return ok;
    }

    if (rx->len >= PROTO_MAX_LINE) {
        rx->overflow = true;
        return false;
    }

    rx->buf[rx->len++] = c;
    return false;
}

int proto_build_status(char *dst, int cap, char code)
{
    if (cap < 3) {
        return 0;
    }
    dst[0] = 'S';
    dst[1] = code;
    dst[2] = '\n';
    return 3;
}

int proto_build_cmd(char *dst, int cap, char cmd)
{
    if (cap < 2) {
        return 0;
    }
    dst[0] = cmd;
    dst[1] = '\n';
    return 2;
}

int proto_build_telemetry(char *dst, int cap, uint32_t ppm, char code)
{
    if (cap < 9) {
        return 0;
    }
    if (ppm > 99999u) {
        ppm = 99999u;
    }

    dst[0] = 'P';
    for (int i = 5; i >= 1; i--) {
        dst[i] = (char)('0' + (ppm % 10u));
        ppm /= 10u;
    }
    dst[6] = ',';
    dst[7] = code;
    dst[8] = '\n';
    return 9;
}

bool proto_is_status_code(char code)
{
    switch (code) {
    case PROTO_CODE_L0:
    case PROTO_CODE_L1:
    case PROTO_CODE_L2:
    case PROTO_CODE_L3:
    case PROTO_CODE_WARMUP:
    case PROTO_CODE_FAULT:
    case PROTO_CODE_STOPPED:
        return true;
    default:
        return false;
    }
}

bool proto_parse_status(const char *line, uint8_t len, char *code_out)
{
    if (len != 2u || line[0] != 'S' || !proto_is_status_code(line[1])) {
        return false;
    }
    *code_out = line[1];
    return true;
}

bool proto_parse_cmd(const char *line, uint8_t len, char *cmd_out)
{
    if (len != 1u) {
        return false;
    }
    if (line[0] != PROTO_CMD_START && line[0] != PROTO_CMD_STOP) {
        return false;
    }
    *cmd_out = line[0];
    return true;
}

bool proto_code_to_level(char code, uint8_t *level_out)
{
    if (code < PROTO_CODE_L0 || code > PROTO_CODE_L3) {
        return false;
    }
    *level_out = (uint8_t)(code - PROTO_CODE_L0);
    return true;
}
