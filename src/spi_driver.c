#include "spi_driver.h"
#include "stm32f4xx.h"

//SPI1
#define SPIx_CLK_EN_BIT 12U
#define AF_MODE 2U

void spi_init(void) {
    //0. Enable Clock to SPI APBx
    RCC->APB2ENR |= (1 << SPIx_CLK_EN_BIT);

    //1. Enable Clock to GPIOA
    RCC->AHB1ENR |= (1 << 0);

    //2. Set GPIOA to AF5
    //3. Set AF for GPIOA - AF5
    // PA4 - SPI1_NSS
    GPIOA->MODER &= ~(3U << (4U * 2U));
    GPIOA->MODER |=  (1U << (4U * 2U)); //1U or AF_MODE?
    GPIOA->AFRL  &= ~(15U << (4U * 2U));
    GPIOA->AFRL  |=  (5U << (4U * 2U));
    // PA5 - SPI1_SCK
    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (AF_MODE << (5U * 2U));
    GPIOA->AFRL  &= ~(15U << (5U * 2U));
    GPIOA->AFRL  |=  (5U << (5U * 2U));
    // PA6 - SPI1_MISO
    GPIOA->MODER &= ~(3U << (6U * 2U));
    GPIOA->MODER |=  (AF_MODE << (6U * 2U));
    GPIOA->AFRL  &= ~(15U << (6U * 2U));
    GPIOA->AFRL  |=  (5U << (6U * 2U));
    // PA7 - SPI1_MOSI
    GPIOA->MODER &= ~(3U << (7U * 2U));
    GPIOA->MODER |=  (AF_MODE << (7U * 2U));
    GPIOA->AFRL  &= ~(15U << (7U * 2U));
    GPIOA->AFRL  |=  (5U << (7U * 2U));

    //2. Configure SPI - Set Clock Polarity and Clock Phase
    SPI1->CR1 &= ~(1U << 0U); //Positive CLK EDGE
    SPI1->CR1 &= ~(1U << 1U); //IDLE when 0
    //3. Configure SPI - Set Master / Slave
    SPI1->CR1 |= (1U << 2U); //Set to master
    //4. Configure SPI - Setting Bit Rate
    SPI1->CR1 &= ~(3U << 3U) //Baudrate of 8MHz
    //5. Configure SPI - Data Frame Format to 16 for IMU
    SPI1->CR1 |= (1U << 11U);

    //6. Configure SPI - CR2 Default
    SPI1->CR2 = 0U;

    //5. ENABLE SPI
    SPI1->CR1 |= (1U << 6U);
}

void spi_write(uint16_t data){
    // Assert Busy Flag
    while(SPI1->SR & (0x80)){}
    // Set SS Pin
    GPIOA->ODR |= (1U << 4U);

    SPI1->DR = data;
}