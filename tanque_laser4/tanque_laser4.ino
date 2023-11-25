#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL  
#define PRESCALER_VALUE 1024
#define TIMER1_COUNT (F_CPU / PRESCALER_VALUE - 1)

#define LASER_PIN PB1
#define SENSOR_LDR PC0
#define SINAL_A PC2
#define SINAL_B PC1
#define SINAL_C PC4
#define SINAL_D PC3
#define RESET_VIDAS PC5
#define LED_VIDA1 PD0
#define LED_VIDA2 PD1
#define LED_VIDA3 PD2
#define MOTOR1_FRENTE PB3 //OCR2A
#define MOTOR1_TRAS PD3 //OCR2B
#define MOTOR2_FRENTE PD5 //OCR0B
#define MOTOR2_TRAS PD6 //OCR0A

// Função de inicialização do timer 1
void timer1_init() {
    TCCR1B |= (1 << CS12) | (1 << CS10); // Prescaler para 1024
    OCR1A = TIMER1_COUNT;
    TIMSK1 |= (1 << OCIE1A); // Habilitar a interrupção de comparação A
    sei(); // Habilita as interrupções globais
}

// Função de interrupção do timer do Laser
ISR(TIMER1_COMPA_vect) {
    PORTB ^= (1 << LASER_PIN); // Alterna o estado do Laser(PB1)
}

// Função de inicialização do timer 0 para monitorar os sinais de entrada e PWM
void timer0_init() {
    // Modo PWM não invertido para OC0A e OC0B, prescaler para 64
    TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0A1) | (1 << COM0B1);
    TCCR0B |= (1 << CS01) | (1 << CS00);

    // Modo PWM não invertido para OC2A e OC2B, prescaler para 64
    TCCR2A |= (1 << WGM20) | (1 << WGM21) | (1 << COM2A1) | (1 << COM2B1);
    TCCR2B |= (1 << CS22) | (1 << CS20);
    
    TIMSK0 |= (1 << TOIE0); // Habilitar a interrupção de overflow
    sei(); // Habilita as interrupções globais
}

// Função de interrupção do timer 0
ISR(TIMER0_OVF_vect) {
    // Ler os sinais de entrada nas portas PC1, PC2, PC3, PC4
    uint8_t frente = PINC & (1 << SINAL_A);
    uint8_t direita = PINC & (1 << SINAL_B);
    uint8_t esquerda = PINC & (1 << SINAL_C);
    uint8_t atras = PINC & (1 << SINAL_D);

    if (frente == (1 << SINAL_A)) {
        // Ligar MOTOR1_FRENTE e MOTOR2_FRENTE 65%
        // Desliga MOTOR1_TRAS e MOTOR2_TRAS
        OCR2B = 0;  // Ajuste para 80% do ciclo de trabalho
        OCR0B = 205;  // Desliga motor
        OCR0A = 0;  // Desliga motor
        OCR2A = 205;  // Ajuste para 80% do ciclo de trabalho
        //PORTD &= ~((1 << MOTOR1_TRAS) | (1 << MOTOR2_TRAS));
    } 
    else if (esquerda == (1 << SINAL_C)) {
        // Ligar MOTOR1_frente 80% e MOTOR2_frente 60%
        // Desliga MOTOR1_TRAS e MOTOR2_TRAS
        OCR2B = 0;  // Ajuste para 80% do ciclo de trabalho
        OCR0B = 0;   // Ajuste para 60% do ciclo de trabalho
        OCR0A = 205;  // Desliga motor
        OCR2A = 144;  // Desliga motor
        //PORTD &= ~((1 << MOTOR1_TRAS) | (1 << MOTOR2_TRAS));
    } 
    else if (direita == (1 << SINAL_B)) {
        // Ligar MOTOR1_frente 60% e MOTOR2_frente 80%
        // Desliga MOTOR1_TRAS e MOTOR2_TRAS
        OCR2B = 144;   // Ajuste para 60% do ciclo de trabalho
        OCR0B = 205;  // Ajuste para 80% do ciclo de trabalho
        OCR0A = 0;  // Desliga motor
        OCR2A = 0;  // Desliga motor
        //PORTD &= ~((1 << MOTOR1_TRAS) | (1 << MOTOR2_TRAS));
    } 
    else if (atras == (1 << SINAL_D)) {
        // Ligar MOTOR1_TRAS e MOTOR2_TRAS 80%
        // Desliga MOTOR1_FRENTE e MOTOR2_FRENTE
        OCR0A = 205;  // Ajuste para 80% do ciclo de trabalho
        OCR2A = 0;  // Desliga motor
        OCR0B = 0;  // Desliga motor
        OCR2B = 205;  // Ajuste para 80% do ciclo de trabalho
        //PORTD &= ~((1 << MOTOR1_FRENTE) | (1 << MOTOR2_FRENTE));
    } 
    else {
        OCR0A = 0;  // Desliga motor
        OCR2A = 0;  // Desliga motor
        OCR0B = 0;  // Desliga motor
        OCR2B = 0;  // Desliga motor
        // Desliga MOTOR1_FRENTE, MOTOR2_FRENTE, MOTOR1_TRAS e MOTOR2_TRAS
        //PORTD &= ~((1 << MOTOR1_FRENTE) | (1 << MOTOR2_FRENTE) | (1 << MOTOR1_TRAS) | (1 << MOTOR2_TRAS));
    }
}
// Função de inicialização do timer 2 para monitorar o Sensor LDR
void timer2_init() {
    
    TCCR2B |= (1 << CS22);// Configurar o prescaler para 64 
    TIMSK2 |= (1 << TOIE2);// Habilitar a interrupção de overflow
    TCNT2 = 0;// Inicializar o Timer2
    
}
ISR(TIMER2_OVF_vect) {
    
    // Selecionar o canal ADC (por exemplo, ADC0 para o pino PC0)
    ADMUX &= 0xF0;          // Limpar os bits de seleção de canal
    ADMUX |= SENSOR_LDR; // Configurar o canal ADC (por exemplo, para o pino PC0)

    // Iniciar a conversão
    ADCSRA |= (1 << ADSC);

    // Aguardar o término da conversão
    while (ADCSRA & (1 << ADSC));

    uint16_t ldr_value = ADC;

    if ((PIND & (1 << LED_VIDA1)) && (PIND & (1 << LED_VIDA2)) && (PIND & (1 << LED_VIDA3)) && (ldr_value < 450)) {
            
            PORTD &= ~(1 << LED_VIDA3);// Desliga o LED_VIDA3
            _delay_ms(5000);
    }
    else if ((PIND & (1 << LED_VIDA1)) && (PIND & (1 << LED_VIDA2)) && (ldr_value < 450)) {
            
            PORTD &= ~(1 << LED_VIDA2);// Desliga o LED_VIDA2
            _delay_ms(5000);
    }
    else if ((PIND & (1 << LED_VIDA1)) && (ldr_value < 450)) {
            
            PORTD &= ~(1 << LED_VIDA1);// Desliga o LED_VIDA1
            _delay_ms(5000);
    }
}
int main() {
  
    ADCSRA |= (1 << ADEN); // Habilita o ADC
    
    timer1_init(); // Inicializa o timer1
    timer0_init(); // Inicializa o timer0
    timer2_init(); // Inicializa o timer2
    
    DDRB |= (1 << LASER_PIN) | (1 << MOTOR1_FRENTE); // Configura como pinos de saída

    DDRD |= (1 << LED_VIDA1) | (1 << LED_VIDA2) | (1 << LED_VIDA3) |
            (1 << MOTOR1_TRAS) | (1 << MOTOR2_FRENTE) | (1 << MOTOR2_TRAS); // Configura como pinos de saída

    DDRC &= ~((1 << SENSOR_LDR) | (1 << SINAL_A) | (1 << SINAL_B) |
              (1 << SINAL_C) | (1 << SINAL_D) | (1 << RESET_VIDAS)); // Configura como pinos de entrada
              
    PORTD |= (1 << LED_VIDA1) | (1 << LED_VIDA2) | (1 << LED_VIDA3); // Acende os LEDs quando o sistema e' ligado
    
    // Main Loop
    while (1) {
        // Verifica se o botão na porta PC5 está pressionado
        if (!(PINC & (1 << RESET_VIDAS))) {
            PORTD |= (1 << LED_VIDA1) | (1 << LED_VIDA2) | (1 << LED_VIDA3); // Acende os LEDs assim resetando as vidas
        }
    }
    return 0;
}
