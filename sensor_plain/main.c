#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <nrf24l01.h>  // https://github.com/l308g21/tiny-nrf24l01
#include <nrf24l01-mnemonics.h>
#include <string.h>
#include <stdbool.h>

#define SENSOR_TYPE 1
//#define BATTERY_POWERED
#include "sensordata.h"

uint8_t  wakeup_counter = 0;
void     extend_message     ( nRF24L01Message* message, void* data, uint8_t data_length);
void     compose_message( nRF24L01Message* message, sensor_data* Sdata);
void     get_checksum( sensor_data* Sdata );

int main(void){

    nRF24L01_begin();
    // wake up every 8 seconds | wake up occures about every 25s @ 3.3V
    wdt_enable( WDTO_8S );
    WDTCR |= (1 << WDIE);   
    sei();

    while (true){
        set_sleep_mode( SLEEP_MODE_PWR_DOWN );
        sleep_mode();
        // create timeout (wakeup_counter-match * 25)
        if ( wakeup_counter == 0){
            // gather data

            
            // compose message
            nRF24L01Message Smessage;
            Smessage.length = 0;



            // send message
            set_sleep_mode( SLEEP_MODE_IDLE );
            nRF24L01_transmit( address, &Smessage );
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



void extend_message(nRF24L01Message* message, void* data, uint8_t data_length){
    memcpy(&message->data[message->length], data, data_length);
    message->length += data_length;
    return;
}

//  match that function to SENSORTYPE
//      there might be a smarter way.
//          - iterating over all struct memebers (like done here but automagically)
//          - or directly copying and transmitting struct data
//              there padding introduces complexity
void compose_message(nRF24L01Message* message, sensor_data* Sdata){
    extend_message( message, &Sdata->type,        sizeof(Sdata->type) );
    extend_message( message, &Sdata->index,       sizeof(Sdata->index) );
    // extend_message( message, &Sdata->furtherValue1,sizeof(Sdata->furtherValue1) );
    // extend_message( message, &Sdata->furtherValue2,sizeof(Sdata->furtherValue2) );
    return;
}


void get_checksum( sensor_data* Sdata ){
    // adding all values while discarding overflows. complementing and incrementing by 1
    // sum of checksum and values == 0
    Sdata->checksum = Sdata->type + Sdata->index // + furtherValue1 + furtherValue2 ... ;
    Sdata->checksum = ~Sdata->checksum;
    Sdata->checksum++;
    return;
}