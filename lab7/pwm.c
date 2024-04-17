// 
#include <stdio.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "hardware/pwm.h"
#include "hardware/clocks.h"

const uint8_t PWM_PIN = 8;  // PWM 4A

static uint32_t _sysHz;
static uint32_t _wrap;
uint32_t _slice_num;
uint32_t _channel;

void ConfigurePWM(uint32_t baseFrequency)
{
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    _slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    _channel = pwm_gpio_to_channel(PWM_PIN);

    pwm_config conf = pwm_get_default_config();	

    _sysHz = clock_get_hz(clk_sys);
    _wrap = _sysHz / baseFrequency;
    printf("%s: sysHz:%d, wrap: %d\n", __func__, _sysHz, _wrap);
    pwm_set_wrap (_slice_num, (uint16_t) _wrap);

    // Initalize the pwm to 50%
    pwm_set_chan_level(_slice_num, _channel, _wrap/2); // Push a 50% duty cycle

    pwm_set_enabled(_slice_num, true);
}

void setPWMDuty(uint32_t percent)
{
    // 100% is _wrap, 0 is 0..
    uint32_t increment = _wrap/100;
    uint16_t duty = increment*percent;


    printf("%s: percent: %d, increment: %d, duty: %d\n", __func__, percent, increment, duty);

    pwm_set_chan_level(_slice_num, _channel, duty); // Push a 50% duty cycle
}