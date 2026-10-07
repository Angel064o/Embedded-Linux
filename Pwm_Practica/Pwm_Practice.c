/*
 * PWM control with two buttons - Raspberry Pi 4 Model B
 * Library: pigpio
 *
 * Wiring (BCM numbering):
 *   PWM output  -> GPIO18 (physical pin 12)  [hardware PWM0]
 *   Button UP   -> GPIO23 (physical pin 16)  other leg to GND
 *   Button DOWN -> GPIO24 (physical pin 18)  other leg to GND
 *   (Internal pull-ups are used, no external resistors needed)
 *
 * Install:
 *   sudo apt update
 *   sudo apt install pigpio libpigpio-dev
 *
 * Compile:
 *   gcc -o pwm_buttons pwm_buttons.c -lpigpio -lrt -lpthread
 *
 * Run (needs root):
 *   sudo ./pwm_buttons
 *
 * Note: if the pigpio daemon is running, stop it first:
 *   sudo systemctl stop pigpiod
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pigpio.h>

#define PWM_PIN        18          /* Hardware PWM pin             */
#define BTN_UP_PIN     23          /* Increase duty cycle          */
#define BTN_DOWN_PIN   24          /* Decrease duty cycle          */

#define PWM_FREQ_HZ    1000        /* PWM frequency: 1 kHz         */
#define PWM_RANGE      1000000     /* pigpio hardware PWM range    */
#define DUTY_STEP      5           /* Change per press (%)         */
#define DUTY_MIN       0
#define DUTY_MAX       100

#define POLL_US        10000       /* Poll every 10 ms             */
#define DEBOUNCE_US    50000       /* 50 ms debounce               */

static volatile int running = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

static void set_duty(int duty_percent)
{
    unsigned int dc = (unsigned int)duty_percent * (PWM_RANGE / 100);
    gpioHardwarePWM(PWM_PIN, PWM_FREQ_HZ, dc);
}

int main(void)
{
    int duty = 0;
    int last_up = 1, last_down = 1;       /* Buttons idle HIGH (pull-up) */
    int up, down;

    if (gpioInitialise() < 0) {
        fprintf(stderr, "Error: could not initialize pigpio (run with sudo).\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    /* Buttons as inputs with pull-up */
    gpioSetMode(BTN_UP_PIN, PI_INPUT);
    gpioSetPullUpDown(BTN_UP_PIN, PI_PUD_UP);
    gpioSetMode(BTN_DOWN_PIN, PI_INPUT);
    gpioSetPullUpDown(BTN_DOWN_PIN, PI_PUD_UP);

    /* Start PWM at 0% */
    set_duty(duty);

    printf("=== PWM control - Raspberry Pi 4B ===\n");
    printf("PWM pin: GPIO%d | Freq: %d Hz | Step: %d%%\n",
           PWM_PIN, PWM_FREQ_HZ, DUTY_STEP);
    printf("UP button: GPIO%d | DOWN button: GPIO%d\n", BTN_UP_PIN, BTN_DOWN_PIN);
    printf("Press Ctrl+C to exit.\n\n");
    printf("Duty cycle: %3d%%\n", duty);

    while (running) {
        up   = gpioRead(BTN_UP_PIN);
        down = gpioRead(BTN_DOWN_PIN);

        /* Falling edge on UP button = pressed */
        if (last_up == 1 && up == 0) {
            if (duty < DUTY_MAX) {
                duty += DUTY_STEP;
                if (duty > DUTY_MAX) duty = DUTY_MAX;
                set_duty(duty);
                printf("[UP button pressed]   -> Duty cycle: %3d%%\n", duty);
            } else {
                printf("[UP button pressed]   -> Already at maximum (%d%%)\n", DUTY_MAX);
            }
            gpioDelay(DEBOUNCE_US);
        }

        /* Falling edge on DOWN button = pressed */
        if (last_down == 1 && down == 0) {
            if (duty > DUTY_MIN) {
                duty -= DUTY_STEP;
                if (duty < DUTY_MIN) duty = DUTY_MIN;
                set_duty(duty);
                printf("[DOWN button pressed] -> Duty cycle: %3d%%\n", duty);
            } else {
                printf("[DOWN button pressed] -> Already at minimum (%d%%)\n", DUTY_MIN);
            }
            gpioDelay(DEBOUNCE_US);
        }

        /* Re-read after debounce so a held button isn't seen as a new press */
        last_up   = gpioRead(BTN_UP_PIN);
        last_down = gpioRead(BTN_DOWN_PIN);

        gpioDelay(POLL_US);
    }

    printf("\nExiting... PWM off.\n");
    set_duty(0);
    gpioTerminate();
    return EXIT_SUCCESS;
}
