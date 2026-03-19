#ifndef SPI_DRIVER
#define SPI_DRIVER
#include <stdint.h>
void spi_init(void);
void spi_write(uint16_t data);
#endif