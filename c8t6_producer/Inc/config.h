/**
 * @file    config.h
 * @brief   Toan bo hang so cau hinh cua Producer (STM32F103C8T6).
 *
 * Nguon chan ly duy nhat cho chan, thoi gian, nguong va he so.
 * KHONG include header thanh ghi -> cac module thuan tinh toan dung chung
 * file nay van bien dich duoc tren PC de kiem thu.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---- Clock ------------------------------------------------------- */
#define CFG_HSE_HZ                  8000000UL   /* thach anh ngoai      */
#define CFG_TARGET_SYSCLK_HZ        72000000UL  /* HSE x PLL9           */
#define CFG_HSI_HZ                  8000000UL   /* dao dong noi, duong lui */
#define CFG_CLK_SPIN_LIMIT          100000u     /* so vong cho co RCC   */

/* ---- ADC --------------------------------------------------------- */
#define CFG_ADC_CHANNEL             0u          /* PA0 = ADC1_IN0       */
#define CFG_ADC_SPIN_LIMIT          100000u     /* so vong cho co EOC   */

/* ---- UART -------------------------------------------------------- */
#define CFG_UART_BAUD               9600UL
#define CFG_UART_TX_RING            64u         /* chua duoc 63 byte    */
#define CFG_UART_RX_RING            32u         /* chua duoc 31 byte    */

/* Khong nhan duoc lenh nao trong khoang nay -> co link_down.
 * Producer VAN tiep tuc do va gui, khong tu tat. */
#define CFG_LINK_TIMEOUT_MS         15000u

/* ---- Chu ky do --------------------------------------------------- */
#define CFG_SAMPLE_PERIOD_MS        1000u       /* mot chu ky do        */
#define CFG_SAMPLE_COUNT            21          /* so mau ADC moi chu ky */
#define CFG_SAMPLE_GAP_MS           10u         /* tre giua hai mau     */
#define CFG_WARMUP_MS               30000u      /* lam nong truoc khi bao */

/* ---- Chuoi quy doi MQ-2 ------------------------------------------ */
/* V = raw/4095 x Vref;  Rs = RL x (Vc - V)/V;  ppm = A x (Rs/R0)^B    */
#define CFG_ADC_MAX                 4095.0f     /* ADC 12-bit           */
#define CFG_ADC_VREF_MV             3300.0f     /* dien ap tham chieu ADC */
#define CFG_MQ2_VC_MV               3300.0f     /* [UNVERIFIED] Vc mach do */
#define CFG_MQ2_RL_OHM              1000.0f     /* [UNVERIFIED] dien tro tai */
#define CFG_MQ2_R0_OHM              95.6f       /* lay tu ma trong bao cao goc */
#define CFG_MQ2_CLEAN_AIR_RATIO     9.5f        /* Rs/R0 trong khong khi sach */

/* He so duong dac tuyen LPG, dang luy thua va dang logarit tuong duong.
 * [UNVERIFIED] Hai bo nay luu hanh trong cong dong, khong phai so lieu
 * Winsen cong bo chinh thuc. */
#define CFG_MQ2_CURVE_A             574.25f
#define CFG_MQ2_CURVE_B             (-2.222f)
#define CFG_MQ2_LOG_M               (-0.450045f)
#define CFG_MQ2_LOG_B               (1.241721f)

/* ---- Dai hop le: ngoai dai la nghi hong, khong phai so do ---------- */
#define CFG_ADC_RAW_MIN_VALID       20u         /* duoi: nghi dut day   */
#define CFG_ADC_RAW_MAX_VALID       4075u       /* tren: nghi ngan mach */
#define CFG_MQ2_RATIO_MIN_VALID     0.01f
#define CFG_MQ2_RATIO_MAX_VALID     50.0f

/* So chu ky do TOT lien tiep de ROI trang thai loi cam bien. */
#define CFG_SENSOR_FAULT_CYCLES     5

/* Tuoi toi da cua phep do con duoc cong bo thanh mot muc gas. Qua han
 * ma chua xac nhan loi thi bao "chua san sang", khong bao gio bao muc 0. */
#define CFG_MEAS_MAX_AGE_MS         2500u

/* ---- Nguong canh bao (ppm) --------------------------------------- */
#define CFG_GAS_TH1_PPM             1000.0f     /* muc 1 ~ 5 %LEL       */
#define CFG_GAS_TH2_PPM             1500.0f     /* muc 2 ~ 7,5 %LEL     */
#define CFG_GAS_TH3_PPM             2000.0f     /* muc 3 ~ 10 %LEL      */

/* Hysteresis 10 %: leo thang tuc thi, ha cap can ppm duoi nguong tru
 * hysteresis trong CFG_GAS_DOWNGRADE_CYCLES chu ky LIEN TIEP. */
#define CFG_GAS_HYST1_PPM           100.0f
#define CFG_GAS_HYST2_PPM           150.0f
#define CFG_GAS_HYST3_PPM           200.0f
#define CFG_GAS_DOWNGRADE_CYCLES    3

/* ---- LCD 16x2 qua PCF8574 ---------------------------------------- */
#define CFG_LCD_I2C_ADDR7           0x27u       /* [UNVERIFIED] co module la 0x3F */
#define CFG_LCD_COLS                16u
#define CFG_LCD_ROWS                2u
#define CFG_LCD_RETRY_MS            10000u      /* chu ky thu khoi tao lai LCD */

/* ---- I2C --------------------------------------------------------- */
#define CFG_I2C_SPIN_LIMIT          20000u      /* so vong cho mot co    */
#define CFG_I2C_RECOVERY_MS         30u         /* han thoi gian bus-clear */
#define CFG_I2C_RECOVERY_POLLS      1000000u    /* lui chan khi dong ho dung */

/* ---- Che do mo phong (bo comment dong duoi de bat) ---------------- */
/* #define CFG_SIMULATE_PPM */
#define CFG_SIM_PPM_MIN             0.0f
#define CFG_SIM_PPM_MAX             2600.0f
#define CFG_SIM_PPM_STEP            120.0f

#endif
