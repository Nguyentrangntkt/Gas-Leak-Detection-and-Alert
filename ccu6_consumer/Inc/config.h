/**
 * @file    config.h
 * @brief   Toan bo hang so cau hinh cua Consumer (STM32F401CCU6).
 *
 * Nguon chan ly duy nhat cho chan, thoi gian va mau canh bao.
 * KHONG include header thanh ghi -> cac module thuan tinh toan dung chung
 * file nay van bien dich duoc tren PC de kiem thu.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---- Clock ------------------------------------------------------- */
/* [UNVERIFIED] 25 MHz la thach anh cua board WeAct Black Pill, chua do
 * xac nhan. Neu board dung 8 MHz, chi can sua dong duoi: he so PLL tu
 * dieu chinh de VCO input luon bang 1 MHz. */
#define CFG_HSE_HZ                  25000000UL
#define CFG_TARGET_SYSCLK_HZ        84000000UL
#define CFG_HSI_HZ                  16000000UL  /* duong lui khi HSE hong */

/* VCOin = HSE/M = 1 MHz; VCO = 336 MHz; SYSCLK = VCO/P = 84 MHz */
#define CFG_PLL_M                   (CFG_HSE_HZ / 1000000UL)
#define CFG_PLL_N                   336UL
#define CFG_PLL_P                   4UL
#define CFG_PLL_Q                   7UL

#define CFG_CLK_SPIN_LIMIT          200000u     /* so vong cho co RCC   */

/* ---- Chan ket noi ------------------------------------------------ */
#define CFG_LED_PORT_BLUE_PIN       0u          /* PB0 */
#define CFG_LED_PORT_YELLOW_PIN     1u          /* PB1 */
#define CFG_LED_PORT_RED_PIN        2u          /* PB2, cung la BOOT1   */
#define CFG_LED_PORT_GREEN_PIN      3u          /* PB3, cung la JTDO    */
#define CFG_SWITCH_PIN              13u         /* PC13, muc 0 = ON     */

/* [DESIGN DECISION] Bao cao goc chi ghi buzzer o GPIOA va relay o GPIOB,
 * khong ghi so chan. Hai chan duoi la lua chon cua ban khoi phuc, KHONG
 * phai chan da xac nhan tren phan cung goc. */
#define CFG_BUZZER_PIN              1u          /* PA1 */
#define CFG_RELAY_PIN               5u          /* PB5 */

#define CFG_LED_ACTIVE_HIGH         1
#define CFG_BUZZER_ACTIVE_HIGH      1           /* [UNVERIFIED] cuc tinh coi */
#define CFG_RELAY_ACTIVE_LOW        1           /* module relay kich LOW */

/* ---- UART -------------------------------------------------------- */
#define CFG_UART_BAUD               9600UL
#define CFG_UART_TX_RING            64u         /* chua duoc 63 byte    */
#define CFG_UART_RX_RING            32u         /* chua duoc 31 byte    */

/* Khong nhan duoc trang thai hop le trong khoang nay -> mat lien ket. */
#define CFG_LINK_TIMEOUT_MS         3000u

/* Chu ky phat lai lenh I/O lam heartbeat chieu nguoc. */
#define CFG_CMD_HEARTBEAT_MS        5000u

/* Han cho Producer thoi bao "dang dung" khi cong tac dang ON. Qua han
 * thi coi la mat dong bo, khong phai dang khoi dong. */
#define CFG_SYNC_TIMEOUT_MS         10000u

/* ---- Watchdog doc lap -------------------------------------------- */
/* PR = 3 -> bo chia 32. Voi LSI danh nghia 32 kHz thi RLR dem theo ms,
 * nen chu ky danh nghia la 500 ms.
 * [UNVERIFIED] LSI cua F401 chi duoc bao dam trong khoang 17-47 kHz, nen
 * chu ky THUC TE nam trong khoang 340-940 ms. Chua do tren board. */
#define CFG_WDG_PRESCALER_CODE      3u
#define CFG_WDG_RELOAD              499u

/* ---- Thoi gian --------------------------------------------------- */
#define CFG_TIM_TICK_HZ             1000u       /* nhip TIM3            */
#define CFG_DEBOUNCE_MS             50u         /* cua so on dinh cong tac */

/* ---- Mau nhay LED va mau bip ------------------------------------- */
#define CFG_BLINK_L2_HALF_MS        500u        /* muc 2: 1 Hz          */
#define CFG_BLINK_L3_HZ_MIN         2u          /* muc 3: tang dan      */
#define CFG_BLINK_L3_HZ_MAX         10u
#define CFG_BLINK_L3_BASE_MS        500u        /* nua chu ky = BASE/hz */
#define CFG_BLINK_FAULT_HALF_MS     250u        /* loi cam bien         */
#define CFG_BLINK_LINK_HALF_MS      500u        /* mat lien ket / warm-up */

#define CFG_FAULT_BEEP_ON_MS        100u        /* bip ngan, de phan biet */
#define CFG_FAULT_BEEP_PERIOD_MS    2000u       /* voi coi lien tuc cua gas */

/* ---- Nguong dau ra ----------------------------------------------- */
/* Kich relay va keu coi tu muc nay tro len. Loi cam bien va mat lien ket
 * KHONG di qua hai nguong nay: chung luon kich relay va dung mau bip. */
#define CFG_RELAY_ASSERT_FROM_LEVEL 2u

#ifndef CFG_BUZZER_FROM_LEVEL
#define CFG_BUZZER_FROM_LEVEL       2u
#endif

#endif
