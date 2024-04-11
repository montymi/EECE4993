/*
Example program to setup pwm on a Longan Nano board with a Risc V processor GD32VF103
to produce a rainbow color cycle with the on board rgb led.
It uses purely hardware driven alternate function pins and an interrupt driven approach.
Hardware driven approach is faster and uses no CPU cycles but only works on selected pins.
Interrupt driven approach can be used on any pin but is limited to at least 10 times lower frequencies.
*/

#ifdef WITH_SERIAL
#include <usart.h>
#include <stdio.h>
#endif

#include "systick/systick.h"

#include <gd32vf103_timer.h>
#include <gd32vf103_gpio.h>
#include <gd32vf103_rcu.h>


/*
Parameter driven pwm setup to make the code easier to reuse
Look at _cfg_*[] and resize _irq_*[] as needed
*/

struct timers {
    uint32_t port;
    uint32_t rcu;
    uint32_t eclic_interrupt;
    struct irq_channels *channels; // no need to init this
} _cfg_timers[] = {
    { TIMER1, RCU_TIMER1, TIMER1_IRQn }
};

enum Timers { Timer1 };  // Index into _cfg_timers array above


struct timer_channels {
    uint16_t channel;           // channel of the timer to use
    uint32_t interrupt_channel; // only needed if gpio_mode is not alternate function
    uint32_t interrupt_flag;
} _cfg_channels[] = {
    { TIMER_CH_1, TIMER_INT_CH1, TIMER_INT_FLAG_CH1 },
    { TIMER_CH_2, TIMER_INT_CH2, TIMER_INT_FLAG_CH2 },
    { TIMER_CH_3, TIMER_INT_CH3, TIMER_INT_FLAG_CH3 }
};

enum Timer_Channels { Channel1, Channel2, Channel3 };  // Index into _cfg_channels array above


struct gpio_banks {
    uint32_t port;
    uint32_t rcu;
} _cfg_gpio_banks[] = {
    { GPIOA, RCU_GPIOA },
    { GPIOC, RCU_GPIOC },
};

enum Gpio_Banks { BankA, BankC };  // Index into _cfg_gpio_banks array above


enum Pwm_Modes { 
    Timer     = GPIO_MODE_AF_PP, 
    Interrupt = GPIO_MODE_OUT_PP
};

struct pins {
    enum Timers         timer;     // timer to use for that pin
    enum Timer_Channels channel;   // channel of the timer to use
    enum Gpio_Banks     bank;      // gpio bank this pin is part of
    enum Pwm_Modes      mode;
    uint32_t            pin;
} _cfg_pins[] = {
    { Timer1, Channel1, BankA, Timer,     GPIO_PIN_1  }, // green led has advanced timer function
    { Timer1, Channel2, BankA, Timer,     GPIO_PIN_2  }, // blue led has advanced timer function
    { Timer1, Channel3, BankC, Interrupt, GPIO_PIN_13 }  // red led has no advanced timer function
};

enum Pins { PinA1, PinA2, PinC13 };  // Index into _cfg_pins array above


/*
Datastructures to make interrupt handling with many pins faster.
Lists built at startup from _cfg_* configuration.
If you don't provide enough room in _irq_channels[] or _irq_pins[] then interrupt driven pins wont work!
*/

struct irq_pins {
    uint32_t pin;
    uint32_t port;
    uint32_t equalizer;
    struct irq_pins *next;
};

struct irq_channels {
    uint32_t channel;
    uint32_t interrupt_flag;
    struct irq_pins *pins;
    struct irq_channels *next;
};

struct irq_channels _irq_channels[1]; // provide space for all channels of all timers used for interrupt pins
struct irq_pins _irq_pins[1];         // provide space for all interrupt pins


// PWM timimg stuff
const uint16_t PRESCALE = 200;  // Min 200 for interrupt pins -> ~500kHz ticks.
const uint16_t MAX_DUTY = 1000; // 100kHz ticks/MAX_DUTY: 100Hz pwm interval


// Application timing stuff
const uint32_t DUTY_US = 5000; // 5ms same duty: duty*MAX_DUTY = 5s per fade

// Pins the application uses for the led colors
enum Color {
    Red   = PinC13,
    Green = PinA1,
    Blue  = PinA2
};


/*
Regular code starts here. 
No timer, led or other pwm configuration should be necessary below
*/


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))
#endif


#ifdef WITH_SERIAL

/* 
Init serial and reroute c standard output there.
Convenience stuff, not related to pwm at all.
*/

// Init serial output and announce ourselves there
void init_usart0() {
    usart_init(USART0, 115200);
    printf("Rainbow V4 10/2020\n\r");
}

// Reroute c standard output to serial
int _put_char( int ch ) {
    return usart_put_char(USART0, ch);
}

#define DEBUG_OUT(...) printf(__VA_ARGS__)

#else
#define DEBUG_OUT(...)
#endif


/* 
Setup hardware supported pwm using a timer.
Since not all pins can be made to follow the timer generated pattern directly, 
interrupts are used to switch gpio state following the pwm pattern.
Interrupt based pwm maxes out at least 10 times earlier and uses cpu cycles. 
So try to avoid, e.g. connect the red led pin PC13 to adjacent pin PA0 would be an option.
*/

// Make a timer channel use pwm pattern defined by given structure
void init_pwm_channel( uint32_t timer, uint16_t channel, timer_oc_parameter_struct *ocp ) {
    timer_channel_output_config(timer, channel, ocp);
    timer_channel_output_pulse_value_config(timer, channel, 0); // duty off
    timer_channel_output_mode_config(timer, channel, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(timer, channel, TIMER_OC_SHADOW_DISABLE);
}


struct irq_channels *find_or_add_channel( struct irq_channels **head, struct irq_channels **pool, enum Timer_Channels c ) {
    struct irq_channels *curr = *head;
    while( curr ) {
        if( curr->channel == c ) return curr; // ok, channel already available (driving multiple gpios)
        curr = curr->next;
    }

    // Allocate new channel element from pool 
    if( *pool == 0 ) return 0; // no more elements. Increase pool.
    curr = (*pool)++;

    // Init new element with data
    curr->channel = _cfg_channels[c].channel;
    curr->interrupt_flag = _cfg_channels[c].interrupt_flag;
    curr->pins = 0;

    // link new element into list at head
    curr->next = *head;
    *head = curr;

    return curr;
}


struct irq_pins *find_or_add_pin( struct irq_pins **head, struct irq_pins **pool, enum Pins p ) {
    struct irq_pins *curr = *head;
    while( curr ) {
        if( curr->pin == p ) return curr;
        curr = curr->next;
    }

    if( *pool == 0 ) return 0;
    curr = (*pool)++;

    curr->pin = _cfg_pins[p].pin;
    curr->port = _cfg_gpio_banks[_cfg_pins[p].bank].port;
    curr->equalizer = 0;

    curr->next = *head;
    *head = curr;

    return curr;
}


// Reset eclic config and provide clock/reset used gpio banks
// Do this before other components want to use gpio or interrupts
void preinit_pwm() {
    for( int p = 0; p < ARRAY_SIZE(_cfg_pins); p++ ) {
        if( _cfg_pins[p].mode == Interrupt ) {
            ECLIC_Init();
	}
    }

    for( int c = 0; c < ARRAY_SIZE(_cfg_gpio_banks); c++ ) {
        rcu_periph_clock_enable(_cfg_gpio_banks[c].rcu);
        gpio_deinit(_cfg_gpio_banks[c].port);
    }
}


// Setup timer to generate pwm ticks and interval,
// use channels to define the pwm pattern, 
// and make gpio pin state follow that pattern
void init_pwm( uint16_t prescale, uint16_t ticks ) {
    // prepare structure for fast interrupt handling
    struct irq_channels *c_pool = _irq_channels;
    struct irq_pins *p_pool = _irq_pins;
    // clear channel lists
    for( int t = 0; t < ARRAY_SIZE(_cfg_timers); t++ ) {
        _cfg_timers[t].channels = 0;
    }
    // create channel lists with pins using interrupts instead of alternate timer function
    for( int p = 0; p < ARRAY_SIZE(_cfg_pins); p++ ) {
        if( _cfg_pins[p].mode == Interrupt ) {
            struct irq_channels *pc = find_or_add_channel(&_cfg_timers[_cfg_pins[p].timer].channels, &c_pool, _cfg_pins[p].channel);
            if( !pc ) {
                DEBUG_OUT("ERROR: increase channel pool or some irq pins wont work!\n\r");
                break; // Error! Provide more space in _irq_channels
            }
            if( c_pool && c_pool >= &_irq_channels[ARRAY_SIZE(_irq_channels)] ) c_pool = 0; // last slot in _irq_channels used up
            if( !find_or_add_pin(&pc->pins, &p_pool, p) ) {
                DEBUG_OUT("ERROR: increase pin pool or some irq pins wont work!\n\r");
                break; // Error! Provide more space in _irq_pins
            }
            if( p_pool && p_pool >= &_irq_pins[ARRAY_SIZE(_irq_pins)] ) p_pool = 0; // last slot in _irq_pins used up
        }
    }
    DEBUG_OUT("irq lists done\n\r");

    // Init used gpio pins
    for( int p = 0; p < ARRAY_SIZE(_cfg_pins); p++ ) {
        gpio_init(_cfg_gpio_banks[_cfg_pins[p].bank].port, _cfg_pins[p].mode, GPIO_OSPEED_10MHZ, _cfg_pins[p].pin);
        gpio_bit_set(_cfg_gpio_banks[_cfg_pins[p].bank].port, _cfg_pins[p].pin); // switch off inverted led
    }

    DEBUG_OUT("gpio done\n\r");

    for( int t = 0; t < ARRAY_SIZE(_cfg_timers); t++ ) {
        rcu_periph_clock_enable(_cfg_timers[t].rcu); // clock for used timers
        timer_deinit(_cfg_timers[t].port);           // unset old stuff paranoia

        timer_parameter_struct tp = {
            .prescaler = prescale - 1,       // default CK_TIMERx is 54MHz, prescale = 540 -> 100kHz ticks (shortest pulse)
            .alignedmode = TIMER_COUNTER_EDGE,
            .counterdirection = TIMER_COUNTER_UP,
            .period = ticks - 1,             // 1000 ticks for full cycle -> 100Hz pwm interval
            .clockdivision = TIMER_CKDIV_DIV1,
            .repetitioncounter = 0};
        timer_init(_cfg_timers[t].port, &tp);
    }

    DEBUG_OUT("timer init done. %lu ns = %lu kHz ticks and %lu us = %lu Hz intervals if CK_TIMER = CK_ABP1 = %lu MHz\n\r",
        500000000UL / (rcu_clock_freq_get(CK_APB1) / prescale), 2UL * rcu_clock_freq_get(CK_APB1) / (prescale * 1000UL),
        500000UL / (rcu_clock_freq_get(CK_APB1) / (prescale * ticks)), 2UL * rcu_clock_freq_get(CK_APB1) / (prescale * ticks),
        rcu_clock_freq_get(CK_APB1)/1000000UL);

    timer_oc_parameter_struct cp = {
        .outputstate  = TIMER_CCX_ENABLE,
        .outputnstate = TIMER_CCXN_DISABLE,
        .ocpolarity   = TIMER_OC_POLARITY_LOW, // RGB led output is inverted
        .ocnpolarity  = TIMER_OCN_POLARITY_LOW,
        .ocidlestate  = TIMER_OC_IDLE_STATE_HIGH,
        .ocnidlestate = TIMER_OCN_IDLE_STATE_HIGH};
    int use_mode_interrupt = 0;
    for( int p = 0; p < ARRAY_SIZE(_cfg_pins); p++ ) {
        init_pwm_channel(_cfg_timers[_cfg_pins[p].timer].port, _cfg_channels[_cfg_pins[p].channel].channel, &cp);
        if( _cfg_pins[p].mode == Interrupt ) {
            timer_interrupt_enable(_cfg_timers[_cfg_pins[p].timer].port, _cfg_channels[_cfg_pins[p].channel].interrupt_channel);
            timer_interrupt_enable(_cfg_timers[_cfg_pins[p].timer].port, TIMER_INT_UP);
            eclic_irq_enable(_cfg_timers[_cfg_pins[p].timer].eclic_interrupt, 1, 1); // level and prio up to you...
            use_mode_interrupt = 1;
        }
    }

    DEBUG_OUT("channel init done\n\r");

    if( use_mode_interrupt ) {
        eclic_global_interrupt_enable(); // make the interrupt controller do its work
    }

    DEBUG_OUT("eclic enabled\n\r");

    for( int t = 0; t < ARRAY_SIZE(_cfg_timers); t++ ) {
        timer_primary_output_config(_cfg_timers[t].port, ENABLE);
        timer_auto_reload_shadow_enable(_cfg_timers[t].port);
        timer_enable(_cfg_timers[t].port);
    }

    DEBUG_OUT("timers enabled\n\r");
}


volatile uint32_t _h = 0;
volatile uint32_t _u = 0;
volatile uint32_t _c = 0;
volatile uint32_t _g = 0;

// Timer event routine called on rising and falling edges of the pwm signal
// Tested to work up until around 200kHz tick speed (makes 200Hz intervals with 1000 steps)
void handle_pwm_interrupt( enum Timers timer ) {
    _h++;
    if (_cfg_timers[timer].channels)
    {
        // Interrupts not only used for signal going up or down,
        // so filter depending on current output state and timer value
        uint32_t count = timer_counter_read(_cfg_timers[timer].port);

        struct irq_channels *ch = _cfg_timers[timer].channels;
        while( ch ) {
            if( SET == timer_interrupt_flag_get(_cfg_timers[timer].port, ch->interrupt_flag) ) {
                _c++;
                timer_interrupt_flag_clear(_cfg_timers[timer].port, ch->interrupt_flag);
                struct irq_pins *pn = ch->pins;
                while( pn ) {
                    if( pn->equalizer && count >= timer_channel_capture_value_register_read(_cfg_timers[timer].port, ch->channel) ) {
                        gpio_bit_set(pn->port, pn->pin); // inverted led off
                        pn->equalizer = 0;
                    }
                    pn = pn->next;
                }
            }
            ch = ch->next;
        }

        if( SET == timer_interrupt_flag_get(_cfg_timers[timer].port, TIMER_INT_FLAG_UP) ) {
            _u++;
            timer_interrupt_flag_clear(_cfg_timers[timer].port, TIMER_INT_FLAG_UP);
            struct irq_channels *ch = _cfg_timers[timer].channels;
            while( ch ) {
                struct irq_pins *pn = ch->pins;
                while( pn ) {
                    if( (!pn->equalizer) && count < timer_channel_capture_value_register_read(_cfg_timers[timer].port, ch->channel) ) {
                        gpio_bit_reset(pn->port, pn->pin); // inverted led on
                        pn->equalizer = 1;
                    }
                    pn = pn->next;
                }
                ch = ch->next;
            }
        }
    }
}

// Timer interrupt handler needs to have this name to be used by the system
// Define one for each timer that uses interrups according to _cfg_pins[]
void TIMER1_IRQHandler() {
    // This resets flags for UP and CHx
    handle_pwm_interrupt(Timer1);
}


// From the constant pwm interval duration of MAX_DUTY ticks,
// how many of them is the signal of a pin up?
void set_pwm_duty( enum Pins pin, uint16_t duty ) {
    // duty >= MAX_DUTY: always on
    // duty == 0: always off
    timer_channel_output_pulse_value_config(_cfg_timers[_cfg_pins[pin].timer].port, _cfg_channels[_cfg_pins[pin].channel].channel, duty);
}


/* 
Application side using the pwm to fade LEDs
No pin, timer or pwm configuration below
*/

void delay_1us(uint32_t microseconds) {
    // Assuming each iteration of the loop takes approximately 1us
    for (uint32_t i = 0; i < microseconds; i++) {
        // Do nothing (busy waiting)
    }
}


// Gradualy adjust pwm duty to make LED darker or brighter 
void fade( enum Color from, enum Color to, uint32_t duty_us ) {
    for( uint32_t i = 0; i <= MAX_DUTY; i++ ) { // 5s per cycle
        set_pwm_duty(from, MAX_DUTY - i);
        set_pwm_duty(to, i);
        delay_1us(duty_us);
    }
}


// Putting it all together: 
// * Start program saying hello on serial
// * Setup the pwm signal
// * Start fading between colors to cycle through the rainbow
int main() {
    preinit_pwm(); // reset interrupt and gpio state paranoia
    #ifdef WITH_SERIAL
    init_usart0();
    #endif
    init_pwm(PRESCALE, MAX_DUTY);
    DEBUG_OUT("init done\n\r");

    while( 1 ) {
        DEBUG_OUT("fade r->b\n\r");
        fade(Red, Blue, DUTY_US);
        DEBUG_OUT("fade b->g\n\r");
        fade(Blue, Green, DUTY_US);
        DEBUG_OUT("fade g->r\n\r");
        fade(Green, Red, DUTY_US);
        DEBUG_OUT("handler/update/channel/overflow: %lu/%lu/%lu/%lu\n\r", _h, _u, _c, _g);
    }
}
