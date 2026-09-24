#include<gpiod.h>
#include<stdio.h>

#include"gpio.h"

static struct gpiod_chip *chip;
static struct gpiod_line_request *request;
static struct gpiod_line_config *config;

int gpio_init(void)

{
    chip = gpiod_chip_open("/dev/gpiochip0");

    if (chip == NULL)
    {perror("Error al escribir GPIO");
    return -1;
    }
    config = gpiod_line_config();

    if (config == NULL) 
    {
    fprintf(stderr, "Error al crear line config");
    gpiod_chip_close(chip);
    chip == NULL;
    return -1;
    }
    return 0;
}

int gpio_configure(unsigned int gpio, int mode)

{

    struct gpio
}


switch (mode)
{
case  :
    /* code */
    break;

default:
    break;
}
