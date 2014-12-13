#asm
.equ __w1_port=0x12
.equ __w1_bit=7
#endasm
#include <1wire.h>

#include <mega8.h>
#include <delay.h>

#include "ds18x20_v3.h"  
#include <bcd.h>

unsigned char digit_out[4], cur_dig=0;

int temperature;       // то, что возвращает датчик 

#define MAX_DS18x20 1
unsigned char ds18x20_devices;
unsigned char rom_code[MAX_DS18x20][9];
bit point = 0;

#define A   1
#define B   4
#define C   16 
#define D   64  
#define E  128 
#define F  2 
#define G  8

static flash unsigned char digit[] = {
    0b11111111 ^(A+B+C+D+E+F),   // 0
    0b11111111 ^(B+C),           // 1
    0b11111111 ^(A+B+D+E+G),     // 2
    0b11111111 ^(A+B+C+D+G),     // 3
    0b11111111 ^(B+C+F+G),       // 4
    0b11111111 ^(A+C+D+F+G),     // 5
    0b11111111 ^(A+C+D+E+F+G),   // 6
    0b11111111 ^(A+B+C),         // 7
    0b11111111 ^(A+B+C+D+E+F+G), // 8
    0b11111111 ^(A+B+C+D+F+G),   // 9
    0b11111111 ^(A+B+C+E+F+G),   // A - 10
    0b11111111 ^(C+D+E+F+G),     // b - 11
    0b11111111 ^(A+D+E+F),       // C - 12
    0b11111111 ^(B+C+D+E+G),     // d - 13
    0b11111111 ^(A+D+E+F+G),     // E - 14
    0b11111111 ^(A+E+F+G),       // F - 15
    0b11111111 ^(G),             // 16 - minus
    0b11111111 ^(A+B+F+G),       // 17 - grad 
    0b11111111 ^(0),             // 18 - blank
    0b11111111 ^(C+E+G),         // n
    0b11111111 ^(D+E+F+G)        // t
};

#define MINUS 16
#define SPACE 18
#define F_SYMBOL 15

    

// Timer2 overflow interrupt service routine
interrupt [TIM2_OVF] void timer2_ovf_isr(void) {
    PORTD=0b00000000;
    PORTB=digit[digit_out[cur_dig]];

    if (point && cur_dig == 2) {
        PORTB.5=0;
    }
    PORTD |= (1<<cur_dig);
	cur_dig++;
    if (cur_dig > 3) {
		cur_dig = 0;  
    }
}


void view_term(void) {
    unsigned char celie, drob, tmp, cur_t=0;
    unsigned int temp, celie_tmp, drob_tmp;
    bit zero;

    // ****************************** начало расчета **********************************
    temp = (unsigned int) temperature;
    if (temperature < 0) {
        temp = ( ~temp ) + 0x0001; // если число отрицательное перевести его в норм.вид
        zero = 1; //  темп.отрицательная (в дальнейшем атрибут отр.темп.)    
    } else {
        zero = 0; // темп.положительная
    }

    celie_tmp = temp >> 4;              // целая часть числа
    drob_tmp  = temp & 0x000F;          // дробная часть числа с точностью 1/16 градуса
    drob  = (unsigned char) ((drob_tmp * 10) / 16); // преобразование дробной части в формате "1/16 градуса"
                                                    // в десятичный формат с точностью 0,1 градуса 
    celie = (unsigned char) celie_tmp;  // я люблю явное преведение типов
    if (celie > 99) return; // индикация температуры только до 100 гр.
    tmp=bin2bcd(celie); // вычислить целую часть
    // ***************************** конец расчета ************************************

    
    if (celie >= 10) {
        digit_out[cur_t++] = zero ? MINUS :SPACE ; 
        digit_out[cur_t++] = tmp >> 4;    // десятки
        digit_out[cur_t++] = tmp & 0x0F;  // единицы
    } else {
        digit_out[cur_t++] = SPACE;
        digit_out[cur_t++] = zero ? MINUS :SPACE ;
        digit_out[cur_t++] = tmp & 0x0F;      // единицы
    }
    digit_out[cur_t++] = drob; // дробная часть
}

// Declare your global variables here

void main(void) {
    // Declare your local variables here
    unsigned int i;
    // Input/Output Ports initialization
    // Port B initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
    // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
    PORTB=0xFF;
    DDRB=0xFF;

    // Port C initialization
    // Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
    // State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
    PORTC=0x00;
    DDRC=0x00;

    // Port D initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
    // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
    PORTD=0x00;
    DDRD=0xFF;

    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: Timer 0 Stopped
    TCCR0=0x00;
    TCNT0=0x00;

    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: Timer1 Stopped
    // Mode: Normal top=0xFFFF
    // OC1A output: Discon.
    // OC1B output: Discon.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer1 Overflow Interrupt: Off
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    TCCR1A=0x00;
    TCCR1B=0x00;
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;

    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 62,500 kHz
    // Mode: Normal top=0xFF
    // OC2 output: Disconnected
    ASSR=0x00;
    TCCR2=0x05;
    TCNT2=0x00;
    OCR2=0x00;

    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    MCUCR=0x00;

    // Timer(s)/Counter(s) Interrupt(s) initialization
    TIMSK=0x40;

    // USART initialization
    // USART disabled
    UCSRB=0x00;

    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR=0x80;
    SFIOR=0x00;

    // ADC initialization
    // ADC disabled
    ADCSRA=0x00;

    // SPI initialization
    // SPI disabled
    SPCR=0x00;

    // TWI initialization
    // TWI disabled
    TWCR=0x00;

    w1_init();

        
    ds18x20_devices=w1_search(0xf0,rom_code);

    // Global enable interrupts
    #asm("sei")     

    digit_out[0] = SPACE;
    digit_out[1] = SPACE;
    digit_out[2] = F_SYMBOL;
    digit_out[3] = ds18x20_devices;

    point = 1;
    while (1) {
        if (ds18x20_devices >= 1) {
            for (i=0;i<ds18x20_devices;i++) {
                if (rom_code[i][0] == DS18B20_FAMILY_CODE){
                    temperature=ds18x20_temperature(&rom_code[i][0]);
                }
                
                if (temperature!=-9999) view_term();
                delay_ms(2000);
            }
        }
    }
}