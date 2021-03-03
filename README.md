## MAX7219

Helper library for programming a MAX6950 and MAX6951 LED drivers via SPI with
STM32 HAL.

### Example

```C
/* Project setup example */
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
SPI_HandleTypeDef hspi1;

/* Configuration */
max7219_config_t max7219_config;
max7219_config.hspi = &hspi1;
max7219_config.port_cs = SPI1_CS_GPIO_Port;
max7219_config.pin_cs = SPI1_CS_Pin;
max7219_config.intensity = 5;
max7219_config.digits = 3;
max7219_init(&max7219_config);

/* Use */
max7219_show_decimal(&max7219_config, 42);
```

