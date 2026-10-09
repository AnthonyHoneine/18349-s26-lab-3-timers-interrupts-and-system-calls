#include <unistd.h>
#include <gpio.h>
#include <keypad_driver.h>
#include <systick.h>

#define KEYPAD_ROW1_PORT GPIO_B
#define KEYPAD_ROW1_PIN 3
#define KEYPAD_ROW2_PORT GPIO_B
#define KEYPAD_ROW2_PIN 5
#define KEYPAD_ROW3_PORT GPIO_B
#define KEYPAD_ROW3_PIN 10
#define KEYPAD_ROW4_PORT GPIO_A
#define KEYPAD_ROW4_PIN 9

#define KEYPAD_COL1_PORT GPIO_A
#define KEYPAD_COL1_PIN 5
#define KEYPAD_COL2_PORT GPIO_A
#define KEYPAD_COL2_PIN 10
#define KEYPAD_COL3_PORT GPIO_A
#define KEYPAD_COL3_PIN 8

#define DEBOUNCE_MS 20u

static char candidate_key = '\0';
static char stable_key = '\0';
static uint32_t candidate_since = 0;

void keypad_init() {
    gpio_init(GPIO_B, 3, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_PULL_UP, 0);
    gpio_init(GPIO_B, 5, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_PULL_UP, 0);
    gpio_init(GPIO_B, 10, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_PULL_UP, 0);
    gpio_init(GPIO_A, 9, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_PULL_UP, 0);

    gpio_init(GPIO_A, 5, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, 0);
    gpio_init(GPIO_A, 10, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, 0);
    gpio_init(GPIO_A, 8, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, 0);

    gpio_set(KEYPAD_COL1_PORT, KEYPAD_COL1_PIN);
    gpio_set(KEYPAD_COL2_PORT, KEYPAD_COL2_PIN);
    gpio_set(KEYPAD_COL3_PORT, KEYPAD_COL3_PIN);

    candidate_key = '\0';
    stable_key = '\0';
    candidate_since = systick_get_ticks();
}

/** Drive one column low and all other columns high. */
void set_col(int col) {
    gpio_set(KEYPAD_COL1_PORT, KEYPAD_COL1_PIN);
    gpio_set(KEYPAD_COL2_PORT, KEYPAD_COL2_PIN);
    gpio_set(KEYPAD_COL3_PORT, KEYPAD_COL3_PIN);

    switch (col) {
        case 1:
            gpio_clr(KEYPAD_COL1_PORT, KEYPAD_COL1_PIN);
            break;
        case 2:
            gpio_clr(KEYPAD_COL2_PORT, KEYPAD_COL2_PIN);
            break;
        case 3:
            gpio_clr(KEYPAD_COL3_PORT, KEYPAD_COL3_PIN);
            break;
    }
}

int read_row(int row) {
    switch (row) {
        case 1: return gpio_read(KEYPAD_ROW1_PORT, KEYPAD_ROW1_PIN);
        case 2: return gpio_read(KEYPAD_ROW2_PORT, KEYPAD_ROW2_PIN);
        case 3: return gpio_read(KEYPAD_ROW3_PORT, KEYPAD_ROW3_PIN);
        case 4: return gpio_read(KEYPAD_ROW4_PORT, KEYPAD_ROW4_PIN);
        default: return -1;
    }
}

/** Perform one scan without waiting for a debounce interval. */
static char scan_keypad(void) {
    set_col(1);
    if (!read_row(1)) return '1';
    if (!read_row(2)) return '4';
    if (!read_row(3)) return '7';
    if (!read_row(4)) return '*';

    set_col(2);
    if (!read_row(1)) return '2';
    if (!read_row(2)) return '5';
    if (!read_row(3)) return '8';
    if (!read_row(4)) return '0';

    set_col(3);
    if (!read_row(1)) return '3';
    if (!read_row(2)) return '6';
    if (!read_row(3)) return '9';
    if (!read_row(4)) return '#';

    return '\0';
}

/** Return a new key only after its state has been stable for 20 ms. */
char keypad_read() {
    char raw_key = scan_keypad();
    uint32_t now = systick_get_ticks();

    if (raw_key != candidate_key) {
        candidate_key = raw_key;
        candidate_since = now;
        return '\0';
    }

    if (candidate_key != stable_key &&
        (uint32_t)(now - candidate_since) >= DEBOUNCE_MS) {
        stable_key = candidate_key;
        if (stable_key != '\0') {
            return stable_key;
        }
    }

    return '\0';
}
