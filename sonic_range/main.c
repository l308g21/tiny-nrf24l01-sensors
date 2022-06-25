#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <nrf24l01.h>  // https://github.com/l308g21/tiny-nrf24l01
#include <nrf24l01-mnemonics.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>


#define SENSOR_TYPE 1
//#define BATTERY_POWERED
#include <sensordata.h>
#define SENSOR_INDEX 1

#define TRIG PB3
#define ECHO PB4 

inline void     start_timer();
inline void     stop_timer ();
uint16_t get_sonic_range    ( uint8_t trig, uint8_t echo);
void     extend_message     ( nRF24L01Message* message, void* data, uint8_t data_length);
void     compose_message( nRF24L01Message* message, sensor_data* Sdata);
void     get_checksum( sensor_data* Sdata );


uint8_t wakeup_counter = 0;
uint64_t address = 0x6262626262;
sensor_data Sdata;


int main(void){


    Sdata.type = SENSOR_TYPE;
    Sdata.index = SENSOR_INDEX;
    
    nRF24L01_begin();
    // wake up via wdt interrupt every 8 seconds ( about every 25s @3.3V)
    wdt_enable( WDTO_8S );
    WDTCR |= (1 << WDIE);   
    sei();

    while (true){
        set_sleep_mode( SLEEP_MODE_PWR_DOWN );
        sleep_mode();
        
        // take measurement about every 10min (@3.3V 25s * 24 = 10min)
        if ( wakeup_counter == 23){
            // gather data
            Sdata.sonic_range = get_sonic_range(TRIG, ECHO);
            get_checksum( &Sdata );

            // compose message
            nRF24L01Message Smessage;
            Smessage.length = 0;
            compose_message( &Smessage, &Sdata );
            
            // send message
            set_sleep_mode( SLEEP_MODE_IDLE );
            nRF24L01_transmit( &address, &Smessage );
            // catch nrf24l01 transmit interrupt 
            sleep_mode();
            wakeup_counter = 0;
        }
        else 
            wakeup_counter++;
    }
    return 0;
}



ISR( INT0_vect ){
    // nothing in here
}

ISR( WDT_vect ){
    // enable wdt interrupt
    WDTCR |= (1 << WDIE);
}

ISR( TIMER1_OVF_vect ){
    TCNT0++;
}



void start_timer(){
    //start counting ticks
    // power up timer
    PRR  &= ~(1 << PRTIM1);
    //reset timer;
    TCNT1 = 0;
    //set prescaler to clock/1
    TCCR1 =  (1 << CS10);
    //enable compare match interrupt
    TIMSK = (1 << TOIE1);
    return;
}

void stop_timer(){
    TCCR1 = 0;
    PRR  |= (1 << PRTIM1);
    return;
}


uint16_t get_sonic_range( uint8_t trig, uint8_t echo ){
    DDRB  &= ~(1 << echo);
    DDRB  |=  (1 << trig);
    PORTB &= ~( (1 << trig) | (1 << echo) );
    TCNT0  = 0;

    // pulse trigger >= 10us;
    PORTB |=  (1 << trig);
    _delay_us(10);
    start_timer();
    PORTB &= ~(1 << trig);

    while ( !(PINB & (1 << echo)) ){}
    TCNT1 = 0;
    while ( PINB & (1 << echo) && (TCNT0 < 46) ){}
    stop_timer();

    uint16_t range = (TCNT0 << 8) | TCNT1;
    if (TCNT0 >= 46) range = ~range | (1 << 15);    
    return range;
}



void extend_message(nRF24L01Message* message, void* data, uint8_t data_length){
    memcpy(&message->data[message->length], data, data_length);
    message->length += data_length;
    return;
}



void compose_message(nRF24L01Message* message, sensor_data* Sdata){
    extend_message( message, &Sdata->type,        sizeof(Sdata->type)       );
    extend_message( message, &Sdata->index,       sizeof(Sdata->index)      );
    extend_message( message, &Sdata->checksum,    sizeof(Sdata->checksum)   );
    extend_message( message, &Sdata->sonic_range, sizeof(Sdata->sonic_range));
    return;
}



void get_checksum( sensor_data* Sdata ){
    // adding all values while discarding overflows. complementing and incrementing by 1
    // sum of checksum and values == 0
    Sdata->checksum = Sdata->type + Sdata->index + Sdata->sonic_range;
    Sdata->checksum = ~Sdata->checksum;
    Sdata->checksum++;
    return;
}
