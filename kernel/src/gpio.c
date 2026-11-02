#include <stdint.h>
#include <gpio.h>
#include <rcc.h>

#define BITS_PER_ALT 4
#define BITS_PER_MODE 2
#define BITS_PER_SPEED 2
#define BITS_PER_PUPD 2
#define BITS_PER_TYPE 1
#define GPIOS_PER_ALT_REG 8


/** @brief GPIO Registers - A through G */
typedef struct{
    volatile unsigned long mode;    /**< 0 - Mode */
    volatile unsigned long o_type;  /**< 4 - Output Type */
    volatile unsigned long o_speed; /**< 8 - Output Speed */
    volatile unsigned long pu_pd;   /**< C - Pull-Up/Pull-Down */
    volatile unsigned long idr;     /**< 10 - Input Data */
    volatile unsigned long odr;     /**< 14 - Output Data*/
    volatile unsigned long bsrr;    /**< 18 - Set/Reset */
    volatile unsigned long lckr;    /**< 1C - Configuration Lock */
    volatile unsigned long afr[2];  /**< 20 - Alternate Function Control*/
} gpio_reg;

/* Bitmask to enable IO Port (A - E) */
const int gpio_en[] = {0x01, 0x02, 0x04, 0x08, 0x10};

/* Base addresses of GPIO regs A, B, and C */
gpio_reg* const gpio_regs[] = {(void*)0x40020000, (void*)0x40020400, (void*)0x40020800};



/*
 * gpio_init: GPIO initialization function
 *
 * port  - GPIO_A, GPIO_B, or GPIO_C
 * num - 0 to 15
 * mode - GPIO Port Mode
 * cnf  - GPIO Port Configuration
 */
void gpio_init(gpio_port port, unsigned int num, unsigned int mode, unsigned int otype, unsigned int speed, unsigned int pupd, unsigned int alt){
    struct rcc_reg_map *rcc = (struct rcc_reg_map *)RCC_BASE;     /* Base address of RCC */
    rcc->ahb1_enr |= gpio_en[port];  /* Enable clock for GPIO[port] (A - G) */

    gpio_reg *gp = gpio_regs[port];  /* Base address of GPIO[port] (A - G) */

    /* Reconfiguration must clear the previous bitfield, not only OR in bits. */
    uint32_t mshift = num * BITS_PER_MODE;
    uint32_t sshift = num * BITS_PER_SPEED;
    uint32_t pshift = num * BITS_PER_PUPD;
    uint32_t ashift = (num % GPIOS_PER_ALT_REG) * BITS_PER_ALT;
    unsigned int high = num >= GPIOS_PER_ALT_REG;
    gp->mode = (gp->mode & ~(3u << mshift)) | ((mode & 3u) << mshift);
    gp->o_type = (gp->o_type & ~(1u << num)) | ((otype & 1u) << num);
    gp->o_speed = (gp->o_speed & ~(3u << sshift)) | ((speed & 3u) << sshift);
    gp->pu_pd = (gp->pu_pd & ~(3u << pshift)) | ((pupd & 3u) << pshift);
    gp->afr[high] = (gp->afr[high] & ~(15u << ashift)) | ((alt & 15u) << ashift);
}

/*
 * gpio_set: Set specified GPIO pin to high.
 */
void gpio_set(gpio_port port, unsigned int num){
    gpio_regs[port]->bsrr = 1 << num;           /* Writing to bits 0-15 of BSRR sets the GPIO pin */
}

/*
 * gpio_set: Clear specified GPIO pin to low.
 */
void gpio_clr(gpio_port port, unsigned int num){
    gpio_regs[port]->bsrr = 1 << (num + 16);    /* Writing to bits 16-31 of BSRR clears the GPIO pin */
}

/*
 * gpio_read: Read from selected GPIO pin.
 */
int gpio_read(gpio_port port, unsigned int num) {
    return !!(gpio_regs[port]->idr & (1 << num));
}
