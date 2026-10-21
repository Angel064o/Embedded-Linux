import time  # Módulo para manejo de delays 
import gpiod  # Módulo para manejo y configuración de las líneas de los GPIOs

from gpiod.line import Direction, Value  # Importamos Direction y Value correctamente

LED1 = 17
LED2 = 27
LED3 = 22
LED4 = 26
LED5 = 16
LED6 = 25
LED7 = 24
LED8 = 23

leds = {
    0: LED1,
    1: LED2,
    2: LED3,
    3: LED4,
    4: LED5,
    5: LED6,
    6: LED7,
    7: LED8
}

try:
    with gpiod.request_lines(
        "/dev/gpiochip0",
        consumer="Clase-python",
        config={
            LED1: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE),
            LED2: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE),
            LED3: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE),
            LED4: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE),
            LED5: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE),
            LED6: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE),
            LED7: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE),
            LED8: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE)
            
        },
    ) as gpio:
        print("Secuencia iniciada")
        while True:
            for n in range(8):
                gpio.set_values({leds[n]: Value.ACTIVE})
                time.sleep(.5)
                gpio.set_values({leds[n]: Value.INACTIVE})

            for n in range(8, 0, -1):
                gpio.set_values({leds[n-1]: Value.ACTIVE})
                time.sleep(.5)
                gpio.set_values({leds[n-1]: Value.INACTIVE})
            
except KeyboardInterrupt:
    print("\nSecuencia terminada")