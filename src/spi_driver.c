#include "spi_driver.h"
#include "stm32f4xx.h"

//SPI1
#define SPIx_CLK_EN_BIT 12U
#define AF_MODE 2U

// For now void parameter
void spi_init(void) {
    //0. Enable Clock to SPI APBx
    RCC->APB2ENR |= (1U << SPIx_CLK_EN_BIT);

    //1. Enable Clock to GPIOA
    RCC->AHB1ENR |= 1;

    /*GPIOA Interrupt pins*/
    //PA2
    GPIOA->MODER &= ~(3U << (2U * 2U));
    GPIOA->MODER |=  (1U << (2U * 2U));
    //PA3
    GPIOA->MODER &= ~(3U << (3U * 2U));
    GPIOA->MODER |=  (1U << (3U * 2U));

    //2. Set GPIOA to AF5
    //3. Set AF for GPIOA - AF5
    // PA4 - SPI1_NSS
    GPIOA->MODER &= ~(3U << (4U * 2U));
    GPIOA->MODER |=  (1U << (4U * 2U));
    //GPIOA->AFR[0]  &= ~(15U << (4U * 2U));
    //GPIOA->AFR[0]  |=  (5U << (4U * 2U));
    // PA5 - SPI1_SCK
    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (AF_MODE << (5U * 2U));
    GPIOA->AFR[0]  &= ~(15U << (5U * 4U));
    GPIOA->AFR[0]  |=  (5U << (5U * 4U));
    // PA6 - SPI1_MISO
    GPIOA->MODER &= ~(3U << (6U * 2U));
    GPIOA->MODER |=  (AF_MODE << (6U * 2U));
    GPIOA->AFR[0]  &= ~(15U << (6U * 4U));
    GPIOA->AFR[0]  |=  (5U << (6U * 4U));
    // PA7 - SPI1_MOSI
    GPIOA->MODER &= ~(3U << (7U * 2U));
    GPIOA->MODER |=  (AF_MODE << (7U * 2U));
    GPIOA->AFR[0]  &= ~(15U << (7U * 4U));
    GPIOA->AFR[0]  |=  (5U << (7U * 4U));

    SPI1->CR1 = 0;

    SPI1->CR1 |= (1U << 2U);   // Master
    SPI1->CR1 &=~(7U << 3U);   // Baudrate
    SPI1->CR1 |= (1U << 9U);   // SSM
    SPI1->CR1 |= (1U << 8U);   // SSI
    SPI1->CR1 |= (1U << 11U);  // 16-bit

    SPI1->CR1 |= (1 << 6);   // Enable SPI

    //6. Configure SPI - CR2 Default
    SPI1->CR2 = 0U;

    //5. ENABLE SPI
    SPI1->CR1 |= (1U << 6U);
}

void spi_write(uint16_t data){
    // Assert Busy Flag
    while(!(SPI1->SR & (1U << 1U))){}
    // Set SS Pin
    GPIOA->ODR &= ~(1U << 4U);

    SPI1->DR = data;

    while(SPI1->SR & (1U << 7U)){}

    GPIOA->ODR |= (1U << 4U);
}

void spi_write_keep_CS_LOW(uint16_t data){
    // Assert Busy Flag
    while(!(SPI1->SR & (1U << 1U))){}
    // Set SS Pin
    GPIOA->ODR &= ~(1U << 4U);

    SPI1->DR = data;

    while(SPI1->SR & (1U << 7U)){}
}

uint16_t spi_transfer(uint16_t data){
    // Assert Busy Flag (TXE = 1)
    while(!(SPI1->SR & SPI_SR_TXE)) {}

    // Pull CS low
    GPIOA->ODR &= ~(1U << 4U);

    // Send data
    SPI1->DR = data;

    // Wait until RXNE is set (data received)
    while(!(SPI1->SR & SPI_SR_RXNE)) {}

    // Read received data
    uint16_t received = SPI1->DR;

    // Keep CS LOW if desired, caller will release later
    return received;
}