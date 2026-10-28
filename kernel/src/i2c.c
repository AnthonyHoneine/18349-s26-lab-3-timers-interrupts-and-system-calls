#include <unistd.h>

#include <gpio.h>
#include <i2c.h>
#include <rcc.h>

struct i2c_reg_map {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
    volatile uint32_t FLTR;
};

// Base address of the I2C Register
#define I2C_BASE (struct i2c_reg_map *) 0x40005400

// Sets peripheral clock frequency to 16 MHz
#define I2C_FREQ (16)

// Hard coded 100kHz clock
#define I2C_CCR (80) 

// Enables I2C1
#define I2C1_EN (1 << 21)

// Sends a START condition
#define I2C_START (1 << 8)

// Enables peripherals
#define I2C_PE (1)

// Start bit, set when start condition generated
#define I2C_SB (1)

// Data register is empty
#define I2C_TXE (1 << 7)

// Byte Transfer Finished
#define I2C_BTF (1 << 2)

// sends a STOP condition
#define I2C_STOP (1 << 9)

// Enables ACKs
#define I2C_ACK (1 << 10)

// ACK from peripheral address
#define I2C_ADDR (1 << 1)

// Rise time for clock
#define I2C_TRISE 0x11

// Shift I2C address over, setting read/write bit
#define I2C_ADD_SHIFT (1)


/**
 * @brief Initializes the I2C protocol. Clk is not used, frequency
 * is hard-coded to 100kHz
 */
void i2c_master_init(uint16_t clk){
    (void) clk;

    struct rcc_reg_map *rcc = RCC_BASE;
    rcc->apb1_enr |= I2C1_EN;
    
    struct i2c_reg_map *i2c = I2C_BASE;
    i2c->CR2 |= I2C_FREQ;
    i2c->CR1 |= I2C_ACK;
    i2c->CCR |= I2C_CCR;
    i2c->TRISE = I2C_TRISE;
    i2c->CR1 |= I2C_PE; // peripheral en
    gpio_init(GPIO_B, 8, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW, PUPD_NONE, ALT4); // SCL/D15
    gpio_init(GPIO_B, 9, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW, PUPD_NONE, ALT4); // SDA/D14
    return;
}

/**
 * @brief Start of an I2C transmission. Sends a start condition,
 * then waits for acknowledgement.
 */
void i2c_master_start() {
    struct i2c_reg_map *i2c = I2C_BASE;
    i2c->CR1 |= I2C_START; // generate start condition
    while (!(i2c->SR1 & I2C_SB));
    return;
}

/**
 * @brief End of I2C transmission. Sends a Stop condition.
 */
void i2c_master_stop() {
    struct i2c_reg_map *i2c = I2C_BASE;
    while (!((i2c->SR1 & I2C_TXE) && (i2c->SR1 & I2C_BTF)));
    i2c->CR1 |= I2C_STOP;
    return;
}

/**
 * @brief Sends a value to the specified slave.
 */
int i2c_master_write(uint8_t *buf, uint16_t len, uint8_t slave_addr){
    i2c_master_start();
    struct i2c_reg_map *i2c = I2C_BASE;
    i2c->DR = (slave_addr << I2C_ADD_SHIFT);
    while(!(i2c->SR1 & I2C_ADDR)); // ev6
    (void) i2c->SR2; // read to clear DR
    for (uint16_t i = 0; i < len; i++) {
        while(!(i2c->SR1 & I2C_TXE));
        i2c->DR = buf[i];
    }
    i2c_master_stop();
    return 0;
}

int i2c_master_read(uint8_t *buf, uint16_t len, uint8_t slave_addr){
    (void) buf;
    (void) len;
    (void) slave_addr;

    return 0;
}
