#ifndef PWM_HH
#define PWM_HH 

#define PWM_PERIOD_NS 1000000u

int pwm_init(unsigned int);
int pwm_set_period(unsigned int, unsigned int);
int pwm_set_duty(unsigned int, unsigned int);
int pwm_start(unsigned int);
int pwm_stop(unsigned int);
int pwm_cleanup(unsigned int);

#endif