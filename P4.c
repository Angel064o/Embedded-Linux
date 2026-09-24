#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lgpio.h> // Librería oficial para manejo de GPIO en Raspberry Pi OS

// Número del chip GPIO (0 es el estándar en la mayoría de Raspberry Pi)
#define GPIO_CHIP 0

// Definición de pines (Numeración BCM)
#define PIN_LED       2   // GPIO 2 (Pin físico 3)
#define PIN_BTN_MAS   11  // GPIO 11 (Pin físico 23)
#define PIN_BTN_MENOS 5   // GPIO 5 (Pin físico 29)

// Parámetros de tiempo (en microsegundos)
#define RETARDO_INICIAL 500000  // 500 ms
#define RETARDO_MIN     50000   // 50 ms (Velocidad máxima)
#define RETARDO_MAX     1000000 // 1000 ms (Velocidad mínima)
#define PASO_CAMBIO     50000   // Paso de ajuste por pulsación (50 ms)

int main() {
    int estado_led = 0;
    int retardo = RETARDO_INICIAL;
    int handle;

    // 1. Abrir el chip de GPIO
    handle = lgGpiochipOpen(GPIO_CHIP);
    if (handle < 0) {
        fprintf(stderr, "Error al abrir el chip GPIO. Ejecuta con permisos adecuados.\n");
        return 1;
    }

    // 2. Configurar direcciones de los pines
    lgGpioClaimOutput(handle, 0, PIN_LED, 0); // Pin de LED como Salida (inicia en LOW)
    lgGpioClaimInput(handle, LG_SET_PULL_UP, PIN_BTN_MAS);   // Botón (+) Entradas con Pull-Up
    lgGpioClaimInput(handle, LG_SET_PULL_UP, PIN_BTN_MENOS); // Botón (-) Entradas con Pull-Up

    printf("=== BLINK DE LED CON CONTROL DE VELOCIDAD ===\n");
    printf("LED en GPIO %d | Botón (+): GPIO %d | Botón (-): GPIO %d\n\n",
           PIN_LED, PIN_BTN_MAS, PIN_BTN_MENOS);

    while (1) {
        // 3. Cambiar el estado del LED y escribir en el pin físico
        estado_led = !estado_led;
        lgGpioWrite(handle, PIN_LED, estado_led);

        // 4. Leer lectura de botones (Lógica invertida por Pull-Up: 0 = presionado)
        if (lgGpioRead(handle, PIN_BTN_MAS) == 0) {
            retardo -= PASO_CAMBIO;
            if (retardo < RETARDO_MIN) {
                retardo = RETARDO_MIN;
            }
        }

        if (lgGpioRead(handle, PIN_BTN_MENOS) == 0) {
            retardo += PASO_CAMBIO;
            if (retardo > RETARDO_MAX) {
                retardo = RETARDO_MAX;
            }
        }

        // 5. Mostrar estado en consola
        printf("\r[ LED: %s ]  Retardo: %4d ms ", estado_led ? "ON " : "OFF", retardo / 1000);
        fflush(stdout);

        // 6. Pausa entre conmutaciones
        usleep(retardo);
    }

    // Liberar recursos al finalizar (mantenimiento de buenas prácticas)
    lgGpiochipClose(handle);
    return 0;
}
