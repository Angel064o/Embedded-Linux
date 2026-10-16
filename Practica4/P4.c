#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <lgpio.h>

#define GPIOCHIP       0                         /* Pi 4: gpiochip0          */
#define BTN_UP_PIN     23                        /* Sube el duty cycle       */
#define BTN_DOWN_PIN   24                        /* Baja el duty cycle       */

#define PWM_CHIP       "/sys/class/pwm/pwmchip0"
#define PWM_CH         "/sys/class/pwm/pwmchip0/pwm0"
#define PWM_PERIOD_NS  1000000                   /* 1 kHz                    */

#define DUTY_STEP      5                         /* Cambio por pulsacion (%) */
#define DUTY_MIN       0
#define DUTY_MAX       100

#define POLL_S         0.010                     /* Lectura cada 10 ms       */
#define DEBOUNCE_S     0.050                     /* Antirrebote 50 ms        */

static volatile int running = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

/* Escribe un valor (texto) en un archivo de sysfs. Devuelve 0 si salio bien */
static int sysfs_write(const char *path, const char *value)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    int r = (fputs(value, f) < 0) ? -1 : 0;
    if (fclose(f) != 0) r = -1;
    return r;
}

static int sysfs_write_long(const char *path, long value)
{
    char buf[32];
    snprintf(buf, sizeof buf, "%ld", value);
    return sysfs_write(path, buf);
}

static int pwm_init(void)
{
    char path[128];

    /* Exportar canal 0 (si ya estaba exportado, el error se ignora) */
    sysfs_write(PWM_CHIP "/export", "0");
    usleep(200000);                              /* Esperar a que aparezcan los archivos */

    /* Duty en 0 primero para que nunca sea mayor que el periodo */
    snprintf(path, sizeof path, "%s/duty_cycle", PWM_CH);
    if (sysfs_write(path, "0") < 0) return -1;

    snprintf(path, sizeof path, "%s/period", PWM_CH);
    if (sysfs_write_long(path, PWM_PERIOD_NS) < 0) return -1;

    snprintf(path, sizeof path, "%s/enable", PWM_CH);
    if (sysfs_write(path, "1") < 0) return -1;

    return 0;
}

static void pwm_set_duty(int duty_percent)
{
    char path[128];
    long duty_ns = (long)PWM_PERIOD_NS * duty_percent / 100;
    snprintf(path, sizeof path, "%s/duty_cycle", PWM_CH);
    sysfs_write_long(path, duty_ns);
}

static void pwm_close(void)
{
    char path[128];
    pwm_set_duty(0);
    snprintf(path, sizeof path, "%s/enable", PWM_CH);
    sysfs_write(path, "0");
    sysfs_write(PWM_CHIP "/unexport", "0");
}

int main(void)
{
    int duty = 50;
    int last_up = 1, last_down = 1;              /* Botones en reposo = HIGH (pull-up) */
    int up, down;

    if (pwm_init() < 0) {
        fprintf(stderr, "Error: no se pudo configurar el PWM en %s (ejecuta con sudo).\n", PWM_CH);
        return EXIT_FAILURE;
    }

    int h = lgGpiochipOpen(GPIOCHIP);
    if (h < 0) {
        fprintf(stderr, "Error: no se pudo abrir gpiochip%d (%s)\n", GPIOCHIP, lguErrorText(h));
        pwm_close();
        return EXIT_FAILURE;
    }

    if (lgGpioClaimInput(h, LG_SET_PULL_UP, BTN_UP_PIN) < 0 ||
        lgGpioClaimInput(h, LG_SET_PULL_UP, BTN_DOWN_PIN) < 0) {
        fprintf(stderr, "Error: no se pudieron configurar los botones.\n");
        lgGpiochipClose(h);
        pwm_close();
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    pwm_set_duty(duty);

    printf("=== Control PWM - Raspberry Pi 4B ===\n");
    printf("PWM: GPIO18 | Freq: %d Hz | Paso: %d%%\n", 1000000000 / PWM_PERIOD_NS, DUTY_STEP);
    printf("Boton UP: GPIO%d | Boton DOWN: GPIO%d\n", BTN_UP_PIN, BTN_DOWN_PIN);
    printf("Ctrl+C para salir.\n\n");
    printf("Duty cycle: %3d%%\n", duty);

    while (running) {
        up   = lgGpioRead(h, BTN_UP_PIN);
        down = lgGpioRead(h, BTN_DOWN_PIN);

        /* Flanco de bajada en UP = presionado */
        if (last_up == 1 && up == 0) {
            if (duty < DUTY_MAX) {
                duty += DUTY_STEP;
                if (duty > DUTY_MAX) duty = DUTY_MAX;
                pwm_set_duty(duty);
                printf("[UP]   -> Duty cycle: %3d%%\n", duty);
            } else {
                printf("[UP]   -> Ya esta en el maximo (%d%%)\n", DUTY_MAX);
            }
            lguSleep(DEBOUNCE_S);
        }

        /* Flanco de bajada en DOWN = presionado */
        if (last_down == 1 && down == 0) {
            if (duty > DUTY_MIN) {
                duty -= DUTY_STEP;
                if (duty < DUTY_MIN) duty = DUTY_MIN;
                pwm_set_duty(duty);
                printf("[DOWN] -> Duty cycle: %3d%%\n", duty);
            } else {
                printf("[DOWN] -> Ya esta en el minimo (%d%%)\n", DUTY_MIN);
            }
            lguSleep(DEBOUNCE_S);
        }

        /* Releer despues del antirrebote para que un boton sostenido no cuente como nueva pulsacion */
        last_up   = lgGpioRead(h, BTN_UP_PIN);
        last_down = lgGpioRead(h, BTN_DOWN_PIN);

        lguSleep(POLL_S);
    }

    printf("\nSaliendo... PWM apagado.\n");
    lgGpiochipClose(h);
    pwm_close();
    return EXIT_SUCCESS;
}
