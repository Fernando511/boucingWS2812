#include <stdio.h>
#include "pico/stdlib.h"
#include <math.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/timer.h"

// Arquivo que contém o programa PIO
#include "pio_matrix.pio.h"

// Arquivo que contém os desenhos dos números
#include "desenho.h"

// Definição de constantes
#define NUM_PIXELS 25 // Número de LEDs na matriz
#define btA 5        // Pino do botão A
#define btB 6        // Pino do botão B
#define Matrix_LED 7 // Pino de controle da matriz de LEDs
#define LED_RED 13   // Pino do LED vermelho


// Variáveis globais
static volatile uint32_t a = 0;          // Contador para exibição de números
static volatile uint32_t last_time = 0;  // Armazena o tempo do último evento (em microssegundos)
static volatile uint intensidade = 50; // Armazena a intensidade da matriz de LEDs

// Rotina da interrupção dos botões
static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time()); // Obtém o tempo atual em microssegundos
    printf("contador = %d \n", a);

    // Implementação de debounce: verifica se passaram pelo menos 200ms desde a última interrupção
    if (current_time - last_time > 200000) {
        last_time = current_time;
        
        // Se o botão pressionado foi o btA, incrementa 'a' até 9
        if (gpio == btA) {
            if (a < 9) {
                printf("incrementa -> A = %d\n", a);
                a++;
            } else {
                a = 9;
            }
        }
        // Se o botão pressionado foi o btB, decrementa 'a' até 0
        else if (gpio == btB) {
            if (a > 0) {
                printf("decrementa -> A = %d\n", a);
                a--;
            } else {
                a = 0;
            }
        }
    }
}

// Função para definir a intensidade das cores do LED
uint32_t matrix_rgb(double b, double r, double g) {
    unsigned char R, G, B;
    R = r * intensidade;
    G = g * intensidade;
    B = b * intensidade;
    return (G << 24) | (R << 16) | (B << 8); // Retorna a cor em formato de 32 bits
}

// Função que escreve na matriz de LEDs usando PIO
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b) {
    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        valor_led = matrix_rgb(b, desenho[24 - i], g=0.0); // Define a cor do LED
        pio_sm_put_blocking(pio, sm, valor_led); // Envia o valor para a PIO
    }
}

int main() {
    PIO pio = pio0;  // Seleciona o primeiro bloco de PIO
    bool ok;
    uint16_t i;
    uint32_t valor_led;
    double r = 0.0, b = 0.0, g = 0.0;

    // Configura a frequência de clock para 128 MHz
    ok = set_sys_clock_khz(128000, false);

    // Inicializa a comunicação serial
    stdio_init_all();

    if (ok) printf("clock set to %ld\n", clock_get_hz(clk_sys));

    // Configuração da PIO para controle da matriz de LEDs
    uint offset = pio_add_program(pio, &pio_matrix_program); // Carrega o programa PIO
    uint sm = pio_claim_unused_sm(pio, true);               // Adquire uma máquina de estado
    pio_matrix_program_init(pio, sm, offset, Matrix_LED);   // Inicializa a PIO

    // Inicialização do botão btA
    gpio_init(btA);
    gpio_set_dir(btA, GPIO_IN);
    gpio_pull_up(btA);

    // Inicialização do botão btB
    gpio_init(btB);
    gpio_set_dir(btB, GPIO_IN);
    gpio_pull_up(btB);

    // Inicialização do LED vermelho
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    // Configuração das interrupções das GPIOs
    gpio_set_irq_enabled_with_callback(btA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(btB, GPIO_IRQ_EDGE_FALL, true);

    // Loop principal
    while (true) {
        // Atualiza o desenho na matriz de LEDs baseado no valor de 'a'
        for (int i = 0; i < 10; i++) {
            if (a == i) {
                desenho_pio(nums[a], valor_led, pio, sm, r, g, b);
            }
        }

        // Pisca o LED vermelho para indicar que o programa está rodando
        gpio_put(LED_RED, true);
        sleep_ms(100);
        gpio_put(LED_RED, false);
        sleep_ms(100);
    }
}
