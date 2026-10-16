#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "PWM.h"

static volatile sig_atomic_t running = 1;

static void on_sigint(int sig)
{
    (void)sig;
    running = 0;
}

int main(void) {
    signal(SIGINT, on_sigint);

    // Canal 0 (GPIO18)
    pwm_init(0);
    usleep(200000);  /* el kernel tarda unos ms en crear pwm0 */
    pwm_set_period(PWM_PERIOD_NS, 0);
    pwm_set_duty(PWM_PERIOD_NS / 4, 0);
    pwm_start(0);

    // Canal 1 (GPIO19)
    pwm_init(1);
    usleep(200000);  /* el kernel tarda unos ms en crear pwm1 */
    pwm_set_period(PWM_PERIOD_NS, 1);
    pwm_set_duty(PWM_PERIOD_NS * 3 / 4, 1);
    pwm_start(1);

    // While infinito 
    printf("PWM activo. Ctrl+C para salir.\n");
    while (running) {
        pause();
    }

    // Apaga y libera los canales
    pwm_cleanup(0);
    pwm_cleanup(1);

    printf("Adiós.\n");
    return 0;
}