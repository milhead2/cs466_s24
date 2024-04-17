#include <stdio.h>
#include <stdint.h>

#include <FreeRTOS.h> 
#include <task.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "motor.h"
#include "pwm.h"

typedef enum {
        IRQ_LEVEL_LOW =  0x1,
        IRQ_LEVEL_HIGH = 0x2,
        IRQ_EDGE_FALL =  0x4,
        IRQ_EDGE_RISE =  0x8
} irq_event_t;

const uint8_t PHA_PIN = 5;
const uint8_t PHB_PIN = 4;

const uint8_t CT1_PIN = 6;
const uint8_t CT2_PIN = 7;



const uint8_t PULSE_PIN = 0;

#define VELO_SAMPLES 16

/*
** There are much better templates in C++ but
** these will work here just fine.
*/
#define min(x, y) (((x) > (y)) ? (y) : (x))
#define max(x, y) (((x) < (y)) ? (x) : (y))
#define abs(x)    ((x) < 0) ? ((x) * -1) : (x)

static int32_t _encoder = 0;
static uint32_t _velocitySamples[VELO_SAMPLES];
static uint32_t _velocityNext = 0;

static motor_status_t _motor_status = M_IDLE;

static int32_t _motorSetpointPosition = 0;

void phase_change_irq(unsigned int gpio, long unsigned int event);
static uint32_t _readTimer(void);

void motorDrive(int32_t drive)
{
    assert(drive >= -100);
    assert(drive <= 100);

    if (drive > 0)
    {
        gpio_put(CT1_PIN, 1);
        gpio_put(CT2_PIN, 0);
    }
    else
    {
        gpio_put(CT1_PIN, 0);
        gpio_put(CT2_PIN, 1);
    }
    
    int32_t duty = abs(drive);

#if 0
    //
    // Deadband is a real effect of using DC motors.  There is a minimum PWM percentage that will 
    // overcome the friction of a motor to start (stiction).  In a real control system you would 
    // account for static friction and dynamic friction which are generally two values with 
    // static frictioon being the higher value.
    // 
    // See https://www.wescottdesign.com/articles/Friction/friction.pdf for a 
    //
#define DEADBAND 50
    duty = (duty * (100 - DEADBAND) / 100);
#endif

    setPWMDuty(duty);

    //UARTprintf("    !! sp:%d, i1:%d, i2:%d, pwm%d\n", sp, in1, in2, abs(sp));
}


static void _pidPositionServo( void *notUsed )
{
    int previous_error = 0;
    double integral = 0;
    int dt=1;

    double Kp = 0.0; // TODO You will need to discover these three parameters
    double Ki = 0.0;
    double Kd = 0.0;

    while(1)
    {
        int error = _motorSetpointPosition - _encoder;
        integral = integral + error*dt;
        double derivative = (error - previous_error)/dt;
        double output = Kp*error + Ki*integral + Kd*derivative;
        int drive = output;
        if (drive > 99) drive = 99;
        if (drive < -99) drive = -99;
        motorDrive(drive);
        previous_error = error;

#if 0
        //
        // This printf is very noisy but gives a good picture of the drive (-99 -- 99) 
        // during tuning  It also takes a fair amount of time to process and can screw up the dt 
        // timing of the PID loop.
        //
        printf("  !! sp:%d, mp:%d, d:%d, e:%d, t:%u, v:%d\n", 
                _motorSetpointPosition, 
                _encoder, 
                drive, 
                (uint32_t)error,
                _readTimer(),
                motor_get_velocity());

#endif

        vTaskDelay(1);
        //vTaskDelayUntil( &xLastWakeTime, dt);        
    }
}


void motor_init(void)
{
    printf("%s: \n", __func__);

    // For watching the ISR timing
    gpio_init(PULSE_PIN);
    gpio_set_dir(PULSE_PIN, GPIO_OUT);

    gpio_init(PHA_PIN);
    gpio_set_dir(PHA_PIN, GPIO_IN);
    gpio_init(PHB_PIN);
    gpio_set_dir(PHB_PIN, GPIO_IN);

    gpio_init(CT1_PIN);
    gpio_set_dir(CT1_PIN, GPIO_OUT);    
    gpio_init(CT2_PIN);
    gpio_set_dir(CT2_PIN, GPIO_OUT);    

    gpio_set_irq_enabled_with_callback(PHA_PIN, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, &phase_change_irq);    
    gpio_set_irq_enabled(PHB_PIN, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true);    

    ConfigurePWM(30000);
    setPWMDuty(10);
    setPWMDuty(90);
    setPWMDuty(33);
    //_setupTimer();


#if 1
    xTaskCreate(_pidPositionServo,
                "bid",
                1024,   
                NULL,
                tskIDLE_PRIORITY+2, 
                NULL );        
#endif


}


// placeholder until I get a high-res timer working.
static uint32_t _readTimer(void)
{
    return time_us_32();
}

uint32_t get_postion(void)
{
    return _encoder;
}

void motor_set_position(int32_t position)
{
    printf("%s: position: %d", __func__, position);
    _motorSetpointPosition = position;
}

int32_t motor_get_velocity(void)
{
    int i = 0;
    int32_t v = 0;

    for (i = 0; i < VELO_SAMPLES; i++)
    {
        v += _velocitySamples[i];
    }

    return (v / VELO_SAMPLES);
}

void phase_change_irq(unsigned int gpio, long unsigned int event) 
{
    gpio_put(PULSE_PIN, 1);

    // Manufacture PHA and PHB state.
    static bool PHA = false;
    static bool PHB = false;

    PHA = (gpio == PHA_PIN) ? (event == IRQ_EDGE_RISE) : PHA;
    PHB = (gpio == PHB_PIN) ? (event == IRQ_EDGE_RISE) : PHB;

    // printf("gpio:%d, event:%d, PHA:%d, PHB:%d\n", gpio, event, PHA, PHB);
        
#ifdef USE_MATH_ENCODER_ALGORITHYM
 
    {
        /*
        ** This interrupt method uses the current and prior two state variables to create a 
        ** 4 bit number that is used as a modification matrix for the encoder position
        */
        static uint8_t tt=0;
        static int8_t lookup[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
        static uint32_t _priorTimer;

        tt <<= 2; // Shift tt history Left

        // set lower two bits of tt to value of A and B
        tt |= (PHB) ? 0x02 : 0x00;
        tt |= (PHA) ? 0x01 : 0x00;

        tt &= 0x0f;

        _encoder += lookup[tt];

        uint32_t tNow = _readTimer();
        if (_priorTimer < tNow)
            _velocitySamples[_velocityNext] =  tNow - _priorTimer;
        else
            _velocitySamples[_velocityNext] =  (tNow+0x7fffffff) - (_priorTimer-0x7fffffff);

        _priorTimer = tNow;
        _velocityNext = (_velocityNext+1) % VELO_SAMPLES;

    }
#else
    {
        /*
        ** This interrupt method uses a nested switch statement to determine the encoder position
        ** I've not tomed it but I suspect that this is the faster method.
        */

        static uint32_t _priorTimer;    

        static int8_t old=0;
        uint8_t new;
        new = (PHA)?0x02:0x00;
        new |=(PHB)?0x01:0x00;
        
        switch(old)
        {
            case 0:
                switch(new)
                {
                    case 0: break;
                    case 1: _encoder++; break;
                    case 2: _encoder--; break;
                    case 3: break;
                }
                break;
            case 1:
                switch(new)
                {
                    case 0: _encoder--; break;
                    case 1: break;
                    case 2: break;
                    case 3: _encoder++; break;
                }
                break;
            case 2:
                switch(new)
                {
                    case 0: _encoder++; break;
                    case 1: break;
                    case 2: break;
                    case 3: _encoder--; break;
                }
                break;
            case 3:
                switch(new)
                {
                    case 0: break;
                    case 1: _encoder--; break;
                    case 2: _encoder++; break;
                    case 3: break;
                }
                break;
        }
        old = new;


        uint32_t tNow = _readTimer();
        if (_priorTimer < tNow)
            _velocitySamples[_velocityNext] =  tNow - _priorTimer;
        else
            _velocitySamples[_velocityNext] =  (tNow+0x7fffffff) - (_priorTimer-0x7fffffff);

        _priorTimer = tNow;
        _velocityNext = (_velocityNext+1) % VELO_SAMPLES;

    }

#endif

    gpio_put(PULSE_PIN, 0);

}


void motor_speed_limit(motor_speed_t speed)
{

}

void motor_move(uint32_t pos_in_tics)
{

}

int32_t motor_get_position(void)
{
    return _encoder;
}

motor_status_t motor_status(void)
{
 
    return _motor_status;
}