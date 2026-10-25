import time  # Módulo para manejo de delays 
import gpiod  # Módulo para manejo y configuración de las líneas de los GPIOs
from datetime import timedelta  # NUEVO: para el antirrebote

from gpiod.line import Direction, Value, Bias  # NUEVO: se agrega Bias

LED1 = 17
LED2 = 27
LED3 = 22
LED4 = 26
LED5 = 16
LED6 = 25
LED7 = 24
LED8 = 23

BOTON_RAPIDO = 5   # NUEVO: aumenta la velocidad (cambia el pin si usas otro)
BOTON_LENTO = 6    # NUEVO: disminuye la velocidad

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

retardo = 0.5
RETARDO_MIN = 0.05
RETARDO_MAX = 1.0
PASO = 0.05
rapido_anterior = False
lento_anterior = False

def presionado(pin):
    return gpio.get_value(pin) == Value.INACTIVE

def espera():
    global retardo, rapido_anterior, lento_anterior
    fin = time.monotonic() + retardo
    while time.monotonic() < fin:
        rapido = presionado(BOTON_RAPIDO)
        lento = presionado(BOTON_LENTO)

        if rapido and not rapido_anterior:
            retardo = max(RETARDO_MIN, round(retardo - PASO, 2))
            print(f"Velocidad: {retardo:.2f} s por LED")
        if lento and not lento_anterior:
            retardo = min(RETARDO_MAX, round(retardo + PASO, 2))
            print(f"Velocidad: {retardo:.2f} s por LED")

        rapido_anterior = rapido
        lento_anterior = lento
        time.sleep(0.005)

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
                output_value=Value.INACTIVE),

            # NUEVO: los dos botones como entrada con pull-up y antirrebote
            BOTON_RAPIDO: gpiod.LineSettings(
                direction=Direction.INPUT,
                bias=Bias.PULL_UP,
                debounce_period=timedelta(milliseconds=20)),
            BOTON_LENTO: gpiod.LineSettings(
                direction=Direction.INPUT,
                bias=Bias.PULL_UP,
                debounce_period=timedelta(milliseconds=20))
        },
    ) as gpio:
        print("Secuencia iniciada")
        while True:
            for n in range(8):
                gpio.set_values({leds[n]: Value.ACTIVE})
                espera()   # CAMBIO: antes time.sleep(.5)
                gpio.set_values({leds[n]: Value.INACTIVE})

            for n in range(8, 0, -1):
                gpio.set_values({leds[n-1]: Value.ACTIVE})
                espera()   # CAMBIO: antes time.sleep(.5)
                gpio.set_values({leds[n-1]: Value.INACTIVE})
            
except KeyboardInterrupt:
    print("\nSecuencia terminada")