#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#define BAUDR ((16000000UL / (16UL * 9600UL)) - 1)

void UARTinit() {//Initialises UART protocol
  UBRR0H = (uint8_t)(BAUDR >> 8);
  UBRR0L = (uint8_t)(BAUDR);
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  UCSR0A &= ~(1 << U2X0);
  UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}

void UARTtransmit(uint8_t data) {//Transmits data
  while (!(UCSR0A & (1 << UDRE0))){}
  UDR0 = data;
}

void UARTprint(const char* str) {//Prints some types of data
  while (*str) {
    UARTtransmit(*str++);
  }
}

uint8_t UARTrecieve() {
  while(!(UCSR0A & (1 << RXC0))) {}
  return UDR0;
}

uint16_t volatile position = 375;//PWM compare value for servo angle
uint8_t volatile timerseconds = 0;

ISR(INT5_vect) {//Rotary encoder ISR for reading input
  if(PING & (1 << PG5)){
    if(position < 625) {
      position += 5;
    }
  } else if (position > 125) {
    position -= 5;
  }
}

ISR(TIMER1_COMPA_vect) {
  timerseconds = 1;
}

void setup(void) {
  DDRE |= (1 << DDE4);//PWM output for servo
  TCCR3A |= (1 << COM3B1) | (1 << WGM31);//PWM mode
  TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS31) | (1 << CS30);//PWM mode and prescaler
  ICR3 = 4999;//PWM custom frequency

  DDRE &= ~((1 << DDE5) | (1 << DDE3));//D3 CLK input for rotary encoder | D5 Rotary encoder push button
  DDRG &= ~(1 << DDG5);//D4 Rotary encoder DT input
  PORTE |= (1 << PE5) | (1 << PE3);//Pull up resistor on
  PORTG |= (1 << PG5);//Pull up resistor on

  EICRB |= (1 << ISC51);//D3 ISR setup
  EIMSK |= (1 << INT5);//D3 ISR setup

  TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);//Timer CTC mode and 1024 prescaler
  TIMSK1 |= (1 << OCIE1A);//Comapre match A interrupt enable
  OCR1A = 15624;//1 second comapre value

  DDRH &= ~(1 << DDH3);//Ultrasonic Echo D6
  DDRH |= (1 << DDH4);//Ultrasonic Tring D7
  PORTH |= (1 << PH3) | (1 << PH4);//Pull up resistors on

  TCCR4A = 0;//Normal timer mode
  TCCR4B = (1 << CS41);//Distance 8 prescaler

  sei();//ISR on
}

uint8_t distanceread(void) {//Function to measure distance with ultrasonic sensor
  uint16_t time = 0;
  uint8_t distance = 0;

  PORTH &= ~((1 << PH4) | (1 << PH3));//Activation sequence for sensor
  PORTH |= (1 << PH4);
  _delay_us(10);
  PORTH &= ~(1 << PH4);

  while(!(PINH & (1 << PH3))) {}//Wait for high signal to start reading

  cli();//Disable interrupts to read distance then enable them to not conflict with encoder interupts due to timer causing bouncing
  TCNT4 = 0;
  while(PINH & (1 << PH3)) {
    if(TCNT4 > 46400) {//Stop waiting if past 4m
      break;
    }
  }
  time = TCNT4;
  sei();

  if(time >= 46400) {//Timeout if target is past 4m
    return 0;
  }

  distance = (time / 116);
  return distance;
}

int main(void) {
  UARTinit();
  setup();
  char positionbufer[64];
  uint16_t servoposition = 375;
  int16_t angle = 0;
  int16_t previousangle = 999;
  uint8_t distance = 0;
  uint8_t previousdistance = 255;

  while(1) {
    if(servoposition != position) {//Update servo position with position to only run this when position changes
    cli();//Disable interrupts
    servoposition = position;//Need at least 2 cycles to write 16 bit
    sei();//Enable interrupts
    }

    if(!(PINE & (1 << PE3))) {//Rotary encoder button resets servo to middle position
      cli();//Disable interrupts
      position = 375;//Set servo to middle position
      sei();//Enable interrupts
    }

    if(timerseconds == 1) {//Prints every 1 second and only if angle changes using timer interrupt
      timerseconds = 0;//Reset timer flag for timer interrupt to count again

      angle = 90 - (((uint32_t)(servoposition - 125) * 9) / 25);//Position in PWM CTC value to angle value
      distance = distanceread();//Get distance value in Cm

      if(angle != previousangle || distance != previousdistance) {//If angle or distance changes print the new values
        sprintf(positionbufer, "Angle:%d Degrees, Distance: %d Cm\r\n", angle, distance);
        UARTprint(positionbufer);
        previousangle = angle;
        previousdistance = distance;
      }
    }

    OCR3B = servoposition;//Change servo angle based on PWM signal
  }
}