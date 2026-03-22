#ifndef SPI_DRIVER
#define SPI_DRIVER
#include <stdint.h>
void spi_init(void);
void spi_write(uint16_t data);
void spi_write_keep_CS_LOW(uint16_t data);
uint16_t spi_transfer(uint16_t data);
#endif