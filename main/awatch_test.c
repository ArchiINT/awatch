/*
 * awatch_test.c — Apple Watch-inspired dark watch face
 *
 * Screen layout (240 × 280)
 * ─────────────────────────
 *  y= 12   ● AWATCH          small title + stress-mode dot
 *  y= 36   ────────          60 px teal accent line
 *  y= 64   HH:MM:SS          large white time
 *  y=100   Wednesday         silver weekday
 *  y=122   16 Apr 2026       dim date
 *  y=148   ──────────────    200 px dim separator
 *  y=172   RAM      72 %     teal label + right-aligned %
 *  y=192   ██████░░░░░░░░    4 px teal progress bar
 *  y=210   CPU      18 %     coral label + right-aligned %
 *  y=230   ████░░░░░░░░░░    4 px coral progress bar
 *  y=262   ○ ● ○             pagination dots
 *
 * Task priority map
 * ─────────────────
 *  3  main_task    — animation + watch-face loop (never starved)
 *  2  stress_task  — CPU / RAM stress cycles
 *  1  counter_task — idle-rate probe; starved ⇒ CPU% rises
 */

#include "esp_heap_caps.h"
#include "fonts.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "st7789_draw.h"
#include "st7789_init.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ── screen geometry ─────────────────────────────────────────────────── */
#define SCREEN_W  240
#define SCREEN_H  320
#define CX        (SCREEN_W / 2)
#define CY        (SCREEN_H / 2)

/* ── colour palette (native RGB565, before any byte-swap) ────────────── */
#define COL_BLACK    0x0000   /* background                               */
#define COL_WHITE    0xFFFF   /* primary text                             */
#define COL_SILVER   0x8C71   /* weekday label                            */
#define COL_DIMGRAY  0x4208   /* date + title text                        */
#define COL_TEAL     0x07FF   /* RAM bar, accent line, boot rings         */
#define COL_CORAL    0xFB0C   /* CPU bar  (R=31, G=24, B=12 ≈ coral red) */
#define COL_DARKBAR  0x18C3   /* inactive part of progress bar            */
#define COL_SEP      0x2945   /* horizontal separator                     */
#define COL_DOTACT   0xFFFF   /* active pagination dot                    */
#define COL_DOTINACT 0x2945   /* inactive pagination dot                  */
#define COL_STCPU    0xF800   /* CPU stress indicator                     */
#define COL_STRAM    0xFD20   /* RAM stress indicator                     */

/* ── layout constants ────────────────────────────────────────────────── */
#define Y_TITLE    12
#define Y_ACCENTLN 36
#define Y_TIME     64
#define Y_WEEKDAY  100
#define Y_DATE     122
#define Y_SEP      148
#define Y_RAMLBL   172
#define Y_RAMBAR   192
#define Y_CPULBL   210
#define Y_CPUBAR   230
#define Y_DOTS     262

#define BAR_X      16                     /* bar left margin              */
#define BAR_W     (SCREEN_W - BAR_X * 2) /* 208 px                       */
#define BAR_H      4

/* right-align column for 4-char "100%" text */
#define PCT_X     (SCREEN_W - BAR_X - 4 * Font_11x18.FontWidth)

/* ── shared state ────────────────────────────────────────────────────── */
static volatile uint32_t s_idle_count    = 0;
static          uint32_t s_idle_baseline = 1;
static volatile uint8_t  s_stress_mode  = 0; /* 0 off · 1 CPU · 2 RAM   */

/* ── tasks ───────────────────────────────────────────────────────────── */

static void counter_task(void *arg)
{
    (void)arg;
    for (;;) { s_idle_count++; taskYIELD(); }
}

static void stress_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(12000));        /* wait past boot + baseline */

    for (;;) {
        /* 3 s CPU — tight loop starves counter_task → CPU% → ~90 %      */
        s_stress_mode = 1;
        TickType_t until = xTaskGetTickCount() + pdMS_TO_TICKS(3000);
        volatile uint32_t x = 0xDEADBEEF;
        while ((TickType_t)(xTaskGetTickCount() - until) & 0x80000000U)
            for (int i = 0; i < 10000; i++) x = x * 1664525U + 1013904223U;

        s_stress_mode = 0;
        vTaskDelay(pdMS_TO_TICKS(3000));

        /* 3 s RAM — hold 80 KB → RAM% rises ~25 pp                      */
        s_stress_mode = 2;
        uint8_t *p = malloc(80000);
        if (p) memset(p, 0xAB, 80000);
        vTaskDelay(pdMS_TO_TICKS(3000));
        free(p);

        s_stress_mode = 0;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ── drawing primitives ──────────────────────────────────────────────── */

static uint16_t s_row_buf[SCREEN_W];

/*
 * Bounds-clamped rectangle fill.
 * safe_fill accepts signed int so callers can write (CX - r) freely.
 * Byte-swap: st7789_send_buffer sends little-endian host memory;
 * ST7789 expects big-endian RGB565 over SPI.
 */
static void safe_fill(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0)            { w += x; x = 0; }
    if (y < 0)            { h += y; y = 0; }
    if (x + w > SCREEN_W) { w = SCREEN_W - x; }
    if (y + h > SCREEN_H) { h = SCREEN_H - y; }
    if (w <= 0 || h <= 0) return;

    uint16_t sc = (color >> 8) | (color << 8);
    for (int i = 0; i < w; i++) s_row_buf[i] = sc;
    for (int r = 0; r < h; r++)
        st7789_send_buffer((uint16_t)x,       (uint16_t)(y + r),
                           (uint16_t)(x+w-1), (uint16_t)(y + r),
                           s_row_buf, w);
}

static inline void fill_rect(uint16_t x, uint16_t y,
                              uint16_t w, uint16_t h, uint16_t c)
{
    safe_fill(x, y, w, h, c);
}

/* Axis-aligned rectangular ring centred on (CX, CY) */
static void draw_ring(int r, uint16_t color, int t)
{
    if (r <= 0) return;
    if (t > r)  t = r;
    safe_fill(CX - r,       CY - r,         2*r + 1, t,           color);
    safe_fill(CX - r,       CY + r - t + 1, 2*r + 1, t,           color);
    if (r > t) {
        safe_fill(CX - r,       CY - r + t, t, 2*(r - t) + 1,     color);
        safe_fill(CX + r - t+1, CY - r + t, t, 2*(r - t) + 1,     color);
    }
}

/* Horizontal line growing from screen centre (used in reveal) */
static void expand_line(uint16_t y_pos, uint16_t half_w,
                        uint16_t color, uint16_t delay_ms)
{
    for (int d = 4; d <= (int)half_w; d += 6) {
        safe_fill(CX - d, y_pos, d * 2, 1, color);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    safe_fill(CX - half_w, y_pos, half_w * 2, 1, color);
}

/* ── boot animation ──────────────────────────────────────────────────── */

static void boot_animation(void)
{
    st7789_fill_color(COL_BLACK);

    /* ── Phase 1: two teal ring pulses ──────────────────────────────── */
    const int RMAX = 134, STEP = 4, TRAIL = 24;
    for (int pulse = 0; pulse < 2; pulse++) {
        for (int r = STEP; r <= RMAX + TRAIL; r += STEP) {
            draw_ring(r,         COL_TEAL,  3);
            draw_ring(r - TRAIL, COL_BLACK, 3);
            vTaskDelay(pdMS_TO_TICKS(7));
        }
        for (int r = RMAX; r <= RMAX + TRAIL + STEP; r += STEP)
            draw_ring(r, COL_BLACK, 3);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    /* ── Phase 2: progressive reveal ────────────────────────────────── */

    /* teal accent line */
    expand_line(Y_ACCENTLN, 30, COL_TEAL, 12);
    vTaskDelay(pdMS_TO_TICKS(40));

    /* time digits, one by one */
    time_t now; struct tm tm;
    char tbuf[10], dbuf[13], wbuf[12];
    time(&now); localtime_r(&now, &tm);
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S",  &tm);
    strftime(wbuf, sizeof(wbuf), "%A",         &tm);
    strftime(dbuf, sizeof(dbuf), "%d %b %Y",   &tm);

    uint16_t tx = (SCREEN_W - 8  * Font_16x26.FontWidth) / 2;
    for (int i = 0; tbuf[i]; i++) {
        draw_char(tx + i * Font_16x26.FontWidth, Y_TIME,
                  tbuf[i], &Font_16x26, COL_WHITE, COL_BLACK);
        vTaskDelay(pdMS_TO_TICKS(60));
    }

    /* weekday */
    uint16_t wx = (SCREEN_W - (uint16_t)strlen(wbuf) * Font_11x18.FontWidth) / 2;
    for (int i = 0; wbuf[i]; i++) {
        draw_char(wx + i * Font_11x18.FontWidth, Y_WEEKDAY,
                  wbuf[i], &Font_11x18, COL_SILVER, COL_BLACK);
        vTaskDelay(pdMS_TO_TICKS(35));
    }

    /* date */
    uint16_t dx = (SCREEN_W - (uint16_t)strlen(dbuf) * Font_11x18.FontWidth) / 2;
    for (int i = 0; dbuf[i]; i++) {
        draw_char(dx + i * Font_11x18.FontWidth, Y_DATE,
                  dbuf[i], &Font_11x18, COL_DIMGRAY, COL_BLACK);
        vTaskDelay(pdMS_TO_TICKS(30));
    }

    /* separator */
    expand_line(Y_SEP, 100, COL_SEP, 5);
    vTaskDelay(pdMS_TO_TICKS(30));

    /* bar labels */
    draw_string(BAR_X, Y_RAMLBL, "RAM", &Font_11x18, COL_TEAL,  COL_BLACK);
    vTaskDelay(pdMS_TO_TICKS(60));
    draw_string(BAR_X, Y_CPULBL, "CPU", &Font_11x18, COL_CORAL, COL_BLACK);
    vTaskDelay(pdMS_TO_TICKS(60));

    /* pagination dots */
    uint16_t dot_x = CX - 13;
    fill_rect(dot_x,      Y_DOTS, 6, 6, COL_DOTINACT);
    fill_rect(dot_x + 10, Y_DOTS, 6, 6, COL_DOTACT);
    fill_rect(dot_x + 20, Y_DOTS, 6, 6, COL_DOTINACT);

    vTaskDelay(pdMS_TO_TICKS(300));
}

/* ── static chrome (drawn once after animation) ──────────────────────── */

static void draw_chrome(void)
{
    /* teal accent line */
    fill_rect((SCREEN_W - 60) / 2, Y_ACCENTLN, 60, 2, COL_TEAL);

    /* separator */
    fill_rect((SCREEN_W - 200) / 2, Y_SEP, 200, 1, COL_SEP);

    /* bar labels */
    draw_string(BAR_X, Y_RAMLBL, "RAM", &Font_11x18, COL_TEAL,  COL_BLACK);
    draw_string(BAR_X, Y_CPULBL, "CPU", &Font_11x18, COL_CORAL, COL_BLACK);

    /* pagination dots */
    uint16_t dot_x = CX - 13;
    fill_rect(dot_x,      Y_DOTS, 6, 6, COL_DOTINACT);
    fill_rect(dot_x + 10, Y_DOTS, 6, 6, COL_DOTACT);
    fill_rect(dot_x + 20, Y_DOTS, 6, 6, COL_DOTINACT);
}

/* ── progress bar (RAM or CPU) ───────────────────────────────────────── */

static void update_bar(uint16_t y_lbl, uint16_t y_bar,
                       uint8_t pct, uint16_t color)
{
    /* percentage text, right-aligned */
    char buf[6];
    snprintf(buf, sizeof(buf), "%3u%%", pct);
    fill_rect(PCT_X, y_lbl, 4 * Font_11x18.FontWidth, Font_11x18.FontHeight,
              COL_BLACK);
    draw_string(PCT_X, y_lbl, buf, &Font_11x18, color, COL_BLACK);

    /* bar */
    uint16_t f = (uint16_t)(BAR_W * pct / 100);
    fill_rect(BAR_X,     y_bar, f,          BAR_H, color);
    fill_rect(BAR_X + f, y_bar, BAR_W - f,  BAR_H, COL_DARKBAR);
}

/* ── watch face (updated every second) ──────────────────────────────── */

static void update_watch_face(void)
{
    /* ── title + stress-mode dot ── */
    uint8_t m  = (s_stress_mode > 2) ? 0 : s_stress_mode;
    uint16_t dc = (m == 1) ? COL_STCPU
                : (m == 2) ? COL_STRAM
                :             COL_DIMGRAY;

    /* dot: 8 × 8 px, vertically centred with the title text           */
    fill_rect(10, Y_TITLE + 5, 8, 8, dc);

    /* "AWATCH" text (static but re-drawn to keep it clean)            */
    draw_string(24, Y_TITLE, "AWATCH", &Font_11x18, COL_DIMGRAY, COL_BLACK);

    /* ── time ── */
    time_t now; struct tm tm;
    char tbuf[10], wbuf[12], dbuf[13];
    time(&now); localtime_r(&now, &tm);
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &tm);
    strftime(wbuf, sizeof(wbuf), "%A",        &tm);
    strftime(dbuf, sizeof(dbuf), "%d %b %Y",  &tm);

    uint16_t tx = (SCREEN_W - 8  * Font_16x26.FontWidth) / 2;
    uint16_t wx = (SCREEN_W - (uint16_t)strlen(wbuf) * Font_11x18.FontWidth) / 2;
    uint16_t dx = (SCREEN_W - (uint16_t)strlen(dbuf) * Font_11x18.FontWidth) / 2;

    fill_rect(0, Y_TIME,    SCREEN_W, Font_16x26.FontHeight, COL_BLACK);
    fill_rect(0, Y_WEEKDAY, SCREEN_W, Font_11x18.FontHeight, COL_BLACK);
    fill_rect(0, Y_DATE,    SCREEN_W, Font_11x18.FontHeight, COL_BLACK);

    draw_string(tx, Y_TIME,    tbuf, &Font_16x26, COL_WHITE,  COL_BLACK);
    draw_string(wx, Y_WEEKDAY, wbuf, &Font_11x18, COL_SILVER, COL_BLACK);
    draw_string(dx, Y_DATE,    dbuf, &Font_11x18, COL_DIMGRAY,COL_BLACK);

    /* ── RAM bar ── */
    size_t fh = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t th = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    uint8_t ram_pct = (uint8_t)(100 - (fh * 100 / th));
    update_bar(Y_RAMLBL, Y_RAMBAR, ram_pct, COL_TEAL);

    /* ── CPU bar ── */
    static uint32_t last_idle = 0;
    uint32_t cur   = s_idle_count;
    uint32_t delta = cur - last_idle;
    last_idle      = cur;
    uint8_t cpu_pct = (delta < s_idle_baseline)
                      ? (uint8_t)(100 - delta * 100 / s_idle_baseline)
                      : 0;
    update_bar(Y_CPULBL, Y_CPUBAR, cpu_pct, COL_CORAL);
}

/* ── main task ───────────────────────────────────────────────────────── */

static void main_task(void *arg)
{
    (void)arg;

    boot_animation();

    /* 1 s baseline: stress_task still sleeping its 12-s delay */
    s_idle_count = 0;
    vTaskDelay(pdMS_TO_TICKS(1000));
    s_idle_baseline = (s_idle_count > 0) ? s_idle_count : 1;

    st7789_fill_color(COL_BLACK);
    draw_chrome();

    for (;;) {
        update_watch_face();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── entry point ─────────────────────────────────────────────────────── */

void app_main(void)
{
    st7789_init_spi();
    st7789_backlight_on();
    st7789_init();

    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    xTaskCreate(counter_task, "cpu_cnt", 2048, NULL, 1, NULL);
    xTaskCreate(stress_task,  "stress",  4096, NULL, 2, NULL);
    xTaskCreate(main_task,    "main",    8192, NULL, 3, NULL);
    /* app_main returns and its task is auto-deleted */
}
