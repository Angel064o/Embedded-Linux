#include <errno.h>
#include <pigpio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "PWM.h"

// --- Configuracion de los servos (SG90) ---

#define NUM_SERVOS       3
#define SW_GPIO          23            // GPIO (BCM) del servo 3 (canal por software)
#define SERVO_PERIOD_NS  20000000u     // 50 Hz
#define SERVO_MIN_US     1000          // pulso para ANGLE_MIN (calibrar por servo)
#define SERVO_MAX_US     2000          // pulso para ANGLE_MAX (calibrar por servo)
#define ANGLE_MIN        0
#define ANGLE_MAX        180
#define ANGLE_CENTER     90

// Bandera de ejecución: Ctrl+C la pone en 0
static volatile sig_atomic_t running = 1;

// Angulo actual de cada servo (servo 1 y 2: hardware, servo 3: software)
static int angles[NUM_SERVOS] = {ANGLE_CENTER, ANGLE_CENTER, ANGLE_CENTER};

// Se ejecuta al detectar Ctrl+C
static void on_sigint(int sig) {
    (void)sig;
    running = 0;
}

// sigaction sin SA_RESTART: asi fgets() y read() regresan al pulsar Ctrl+C
static void setup_signals(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Registra el manejador de Ctrl+C
    sigaction(SIGINT, &sa, NULL);
}

// --- Servos ---

// Angulo (grados) -> ancho de pulso (microsegundos)
static unsigned int servo_pulse_us(int angle) {
    return (unsigned int)(SERVO_MIN_US +
           (angle - ANGLE_MIN) * (SERVO_MAX_US - SERVO_MIN_US) / (ANGLE_MAX - ANGLE_MIN));
}

// Mueve un servo (0, 1 o 2) al angulo indicado, limitado a [ANGLE_MIN, ANGLE_MAX]
static int servo_set_angle(unsigned int servo, int angle) {
    unsigned int us;
    int rc;

    // Servo inexistente
    if (servo >= NUM_SERVOS) {
        return -1;
    }

    // Limita el ángulo al rango permitido
    if (angle < ANGLE_MIN) {
        angle = ANGLE_MIN;
    }
    if (angle > ANGLE_MAX) {
        angle = ANGLE_MAX;
    }

    // Convierte el ángulo a ancho de pulso
    us = servo_pulse_us(angle);

    if (servo < 2) {
        rc = pwm_set_duty(us * 1000u, servo);        // hardware: nanosegundos
    } else {
        rc = (gpioServo(SW_GPIO, us) == 0) ? 0 : -1; // software: microsegundos
    }

    if (rc != 0) {
        return -1;
    }

    // Guarda el ángulo solo si el movimiento fue exitoso
    angles[servo] = angle;
    return 0;
}

// --- Inicializacion ---

// Inicia un canal por hardware: 50 Hz, pulso al centro y habilitado
static int init_hw_channel(unsigned int channel) {
    // Exporta el canal PWM
    if (pwm_init(channel) != 0) {
        return -1;
    }

    usleep(200000);  // el kernel tarda unos ms en crear pwmN

    // Periodo, pulso inicial y habilitar (si algo falla, libera el canal)
    if (pwm_set_period(SERVO_PERIOD_NS, channel) != 0 ||
        pwm_set_duty(servo_pulse_us(ANGLE_CENTER) * 1000u, channel) != 0 ||
        pwm_start(channel) != 0) {
        pwm_cleanup(channel);
        return -1;
    }

    return 0;
}

// Inicia los tres servos y los deja en el centro.
// Si algo falla, libera lo que ya se habia iniciado.
// OJO: al arrancar, cada servo salta al centro; el brazo debe estar en una pose segura.
static int PWM_INIT(void) {
    // a) Canales por hardware (GPIO18 y GPIO19)
    if (init_hw_channel(0) != 0) {
        fprintf(stderr, "Error: no se pudo iniciar el canal 0 (hardware)\n");
        return -1;
    }

    if (init_hw_channel(1) != 0) {
        fprintf(stderr, "Error: no se pudo iniciar el canal 1 (hardware)\n");
        pwm_cleanup(0);     // libera el canal 0 que sí se había iniciado
        return -1;
    }

    // b) Canal por software (pigpio)
    // pigpio no debe quedarse con el manejador de Ctrl+C ni usar el reloj del PWM por hardware
    // (ambas configuraciones deben hacerse antes de gpioInitialise)
    gpioCfgSetInternals(gpioCfgGetInternals() | PI_CFG_NOSIGHANDLER);
    gpioCfgClock(5, PI_CLOCK_PCM, 0);

    // Inicia pigpio, configura el pin y manda el pulso central
    if (gpioInitialise() < 0 ||
        gpioSetMode(SW_GPIO, PI_OUTPUT) != 0 ||
        gpioServo(SW_GPIO, servo_pulse_us(ANGLE_CENTER)) != 0) {
        fprintf(stderr, "Error: no se pudo iniciar el canal 2 (software). "
                        "Falta sudo o esta corriendo pigpiod?\n");
        gpioTerminate();
        pwm_cleanup(1);
        pwm_cleanup(0);
        return -1;
    }

    return 0;
}

// Deja de enviar pulsos y libera los tres canales
// OJO: sin pulsos los servos pierden la fuerza y el brazo puede caer
static void PWM_CLEANUP(void) {
    // Apaga el servo por software y cierra pigpio
    gpioServo(SW_GPIO, 0);
    gpioTerminate();

    // Libera los canales por hardware
    pwm_cleanup(1);
    pwm_cleanup(0);
}

// --- Control por teclado ---

// Dibuja la línea de estado (el servo elegido va entre corchetes)
static void print_status(unsigned int selected, int step) {
    unsigned int i;

    printf("\r");
    for (i = 0; i < NUM_SERVOS; i++) {
        printf("%cM%u:%3d%c ", i == selected ? '[' : ' ', i + 1, angles[i],
               i == selected ? ']' : ' ');
    }
    printf(" paso:%2d  ", step);
    fflush(stdout);
}

static void control_teclado(void) {
    static const int steps[] = {1, 2, 5, 10};   // tamaños de paso en grados
    const int num_steps = (int)(sizeof(steps) / sizeof(steps[0]));
    int step_idx = 2;
    unsigned int selected = 0;
    unsigned int i;
    int active = 1;
    struct termios orig, raw;
    ssize_t n;
    char key;

    // Sin terminal no se puede leer tecla por tecla
    if (tcgetattr(STDIN_FILENO, &orig) != 0) {
        fprintf(stderr, "Error: la entrada no es una terminal.\n");
        return;
    }

    // Modo crudo: las teclas llegan una por una, sin Enter y sin eco.
    // VMIN=0 y VTIME=1: read() regresa tras 0.1 s aunque no haya tecla.
    // ISIG sigue activo, asi que Ctrl+C funciona.
    raw = orig;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    printf("\n--- Control por teclado ---\n");
    printf("1/2/3: elegir servo    a/d: mover -/+\n");
    printf("w/s: cambiar paso      c: centrar todos\n");
    printf("q: volver al menu\n\n");
    print_status(selected, steps[step_idx]);

    while (running && active) {
        // Espera una tecla
        n = read(STDIN_FILENO, &key, 1);
        if (n < 0) {
            if (errno == EINTR) {
                continue;       // senal (Ctrl+C): el while revisa running
            }
            break;
        }
        if (n == 0) {
            continue;           // sin tecla en este intervalo
        }

        switch (key) {
        // Elige el servo
        case '1':
        case '2':
        case '3':
            selected = (unsigned int)(key - '1');
            break;
        // Sube el ángulo
        case 'd':
            servo_set_angle(selected, angles[selected] + steps[step_idx]);
            break;
        // Baja el ángulo
        case 'a':
            servo_set_angle(selected, angles[selected] - steps[step_idx]);
            break;
        // Cambia el tamaño del paso
        case 'w':
            if (step_idx < num_steps - 1) {
                step_idx++;
            }
            break;
        case 's':
            if (step_idx > 0) {
                step_idx--;
            }
            break;
        // Centra los tres servos
        case 'c':
            for (i = 0; i < NUM_SERVOS; i++) {
                servo_set_angle(i, ANGLE_CENTER);
            }
            break;
        // Vuelve al menú
        case 'q':
            active = 0;
            break;
        default:
            break;
        }

        print_status(selected, steps[step_idx]);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &orig);   // restaura la terminal
    printf("\n");
}

// --- Demostracion del brazo ---

static void demo_brazo(void) {
    printf("\n--- Iniciando Demostracion del Brazo ---\n");
    printf("Presiona Ctrl+C en cualquier momento para cancelar.\n\n");

    // Secuencia de posiciones: {Servo 0, Servo 1, Servo 2}
    const int posiciones[][NUM_SERVOS] = {
        //15 120 120
        {0,  0,  0},   // 1. Posición Inicial (Centro)
        {0,  40, 40},   // 2. Pose A
        {0, 80,  80},  // 3. Pose B
        {15,   120, 120},    // 4. Extremos Min/Max
        {0, 0, 0},  // 5. Extremos Invertidos
        {15, 120, 120}    // 6. Regreso al Centro
    };

    size_t num_pasos = sizeof(posiciones) / sizeof(posiciones[0]);

    // Recorre las poses mientras no se pulse Ctrl+C
    for (size_t paso = 0; paso < num_pasos && running; paso++) {
        printf("Paso %zu/%zu -> S1: %3d° | S2: %3d° | S3: %3d°\n",
               paso + 1, num_pasos,
               posiciones[paso][0],
               posiciones[paso][1],
               posiciones[paso][2]);

        // Actualizar todos los servos para esta pose
        for (unsigned int i = 0; i < NUM_SERVOS; i++) {
            servo_set_angle(i, posiciones[paso][i]);
        }

        // Esperar 1.5 segundos entre movimientos (en bloques de 100ms para responder rápido a Ctrl+C)
        for (int t = 0; t < 15 && running; t++) {
            usleep(100000);
        }
    }

    // Si running quedó en 0, se canceló con Ctrl+C
    if (!running) {
        printf("\nDemostracion interrumpida por el usuario.\n");
    } else {
        printf("\nDemostracion completada con exito.\n");
    }
}

// --- Menu ---

static void print_menu(void) {
    printf("\n=== Brazo robotico ===\n");
    printf("1) Demostracion del brazo\n");
    printf("2) Control por teclado\n");
    printf("3) Salir\n");
    printf("Opcion: ");
    fflush(stdout);
}

static void run_menu(void) {
    char line[32];
    char *end;
    long option;

    while (running) {
        print_menu();

        // Lee la opción como texto
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (feof(stdin)) {
                break;          // Ctrl+D
            }
            clearerr(stdin);    // interrumpido por una senal (Ctrl+C)
            printf("\n");
            continue;
        }

        // Convierte el texto a número
        option = strtol(line, &end, 10);
        if (end == line) {
            printf("Opcion invalida.\n");
            continue;
        }

        switch (option) {
        case 1:
            demo_brazo();
            break;
        case 2:
            control_teclado();
            break;
        case 3:
            running = 0;    // sale del menú
            break;
        default:
            printf("Opcion invalida.\n");
            break;
        }
    }
}

int main(void) {
    // Detecta Ctrl+C
    setup_signals();

    // Inicializa PWM por hardware (canales 0 y 1) y por software (canal 2)
    if (PWM_INIT() != 0) {
        fprintf(stderr, "Error: no se pudo inicializar el PWM.\n");
        return EXIT_FAILURE;
    }

    // Muestra el menú hasta que el usuario salga
    run_menu();

    // Apaga y libera todos los canales
    PWM_CLEANUP();
    printf("Adios.\n");
    return EXIT_SUCCESS;
}