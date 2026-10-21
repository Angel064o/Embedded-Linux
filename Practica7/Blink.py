import time  # Módulo para manejo de delays 
import gpiod  # Módulo para manejo y configuración de las líneas de los GPIOs

from gpiod.line import Direction, Value  # Importamos Direction y Value correctamente

LED = 17  # Pin GPIO17 (BCM) que se usará para el LED

try:
    with gpiod.request_lines(
        "/dev/gpiochip0",
        consumer="Clase-python",
        config={
            LED: gpiod.LineSettings(
                direction=Direction.OUTPUT,
                output_value=Value.INACTIVE  # Usamos el objeto Value en lugar de un entero
            )
        },
    ) as gpio:
        print("Parpadeo iniciado")
        while True:
            gpio.set_values({LED: Value.ACTIVE})
            time.sleep(1)
            gpio.set_values({LED: Value.INACTIVE})
            time.sleep(1)
except KeyboardInterrupt:
    print("Parpadeo detenido")