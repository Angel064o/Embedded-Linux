#include <stdio.h>
#include <unistd.h>
#include "PWM.h"

#define PWM_CHIP_PATH "/sys/class/pwm/pwmchip0"

int pwm_init(unsigned int channel) {
    FILE *puntero;

    // Revisa si existe pwmchip0
    if (access(PWM_CHIP_PATH, F_OK) != 0) {
        perror("Error: pwmchip0 no existe!");
        return -1;
    }

    // Valida canal
    if ((channel != 0) && (channel != 1)) {
        fprintf(stderr, "Error: canal inválido\n");
        return -1;
    }

    // Exporta el canal
    puntero = fopen(PWM_CHIP_PATH "/export", "w");
    if (puntero == NULL) {
        perror("Error: no se pudo abrir el archivo export!");
        return -1;
    }
    if (fprintf(puntero, "%u", channel) < 0) {
        perror("Error: no se pudo escribir en el archivo export!");
        fclose(puntero);
        return -1;
    }
    fclose(puntero);

    return 0;
}

int pwm_set_period(unsigned int period, unsigned int channel) {
    FILE *puntero;
    char path[100];

    // Validar el canal
    if (channel > 1) {
        fprintf(stderr, "Error: canal invalido!\n");
        return -1;
    }

    // Validar el periodo
    if (period == 0) {
        fprintf(stderr, "Error: el periodo debe ser mayor que cero!\n");
        return -1;
    }

    // Construir la ruta del archivo
    snprintf(path, sizeof(path),
             PWM_CHIP_PATH "/pwm%u/period", channel);

    // Abrir el archivo period
    puntero = fopen(path, "w");
    if (puntero == NULL) {
        perror("Error: no se pudo abrir period!");
        return -1;
    }

    // Escribir el periodo
    if (fprintf(puntero, "%u", period) < 0) {
        perror("Error: no se pudo escribir en el archivo period!");
        fclose(puntero);
        return -1;
    }

    fclose(puntero);
    return 0;
}

int pwm_set_duty(unsigned int duty, unsigned int channel) {
    FILE *puntero;
    char path[100];

    // Validar el canal
    if (channel > 1) {
        fprintf(stderr, "Error: canal invalido!\n");
        return -1;
    }

    unsigned int period;
    char period_path[100];

    snprintf(period_path, sizeof(period_path),
            PWM_CHIP_PATH "/pwm%u/period", channel);

    puntero = fopen(period_path, "r");
    if (puntero == NULL) {
        perror("Error: no se pudo abrir period!");
        return -1;
    }

    if (fscanf(puntero, "%u", &period) != 1) {
        fprintf(stderr, "Error: no se pudo leer el periodo!\n");
        fclose(puntero);
        return -1;
    }

    fclose(puntero);

    if (duty > period) {
        fprintf(stderr, "Error: el duty cycle no puede ser mayor al periodo!\n");
        return -1;
    }

    // Construir la ruta del archivo
    snprintf(path, sizeof(path),
             PWM_CHIP_PATH "/pwm%u/duty_cycle", channel);

    // Abrir el archivo duty_cycle
    puntero = fopen(path, "w");
    if (puntero == NULL) {
        perror("Error: no se pudo abrir duty_cycle!");
        return -1;
    }

    // Escribir el ciclo de trabajo
    if (fprintf(puntero, "%u", duty) < 0) {
        perror("Error: no se pudo escribir en el archivo duty_cycle!");
        fclose(puntero);
        return -1;
    }

    fclose(puntero);
    return 0;
}

int pwm_start(unsigned int channel) {
    FILE *puntero;
    char path[100];

    if (channel > 1) {
        fprintf(stderr, "Error: canal invalido!\n");
        return -1;
    }

    snprintf(path, sizeof(path),
             PWM_CHIP_PATH "/pwm%u/enable", channel);

    puntero = fopen(path, "w");
    if (puntero == NULL) {
        perror("Error: no se pudo abrir enable!");
        return -1;
    }

    if (fprintf(puntero, "1") < 0) {
        perror("Error: no se pudo habilitar el PWM!");
        fclose(puntero);
        return -1;
    }

    fclose(puntero);
    return 0;
}

int pwm_stop(unsigned int channel) {
    FILE *puntero;
    char path[100];

    if (channel > 1) {
        fprintf(stderr, "Error: canal invalido!\n");
        return -1;
    }

    snprintf(path, sizeof(path),
             PWM_CHIP_PATH "/pwm%u/enable", channel);

    puntero = fopen(path, "w");
    if (puntero == NULL) {
        perror("Error: no se pudo abrir enable!");
        return -1;
    }

    if (fprintf(puntero, "0") < 0) {
        perror("Error: no se pudo deshabilitar el PWM!");
        fclose(puntero);
        return -1;
    }

    fclose(puntero);
    return 0;
}

int pwm_cleanup(unsigned int channel) {
    FILE *puntero;

    // Detener el PWM
    if (pwm_stop(channel) != 0) {
        return -1;
    }

    // Abrir el archivo unexport
    puntero = fopen(PWM_CHIP_PATH "/unexport", "w");
    if (puntero == NULL) {
        perror("Error: no se pudo abrir el archivo unexport!");
        return -1;
    }

    // Liberar el canal
    if (fprintf(puntero, "%u", channel) < 0) {
        perror("Error: no se pudo liberar el canal PWM!");
        fclose(puntero);
        return -1;
    }

    fclose(puntero);
    return 0;
}