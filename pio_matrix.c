#include <stdio.h>
#include "pico/stdlib.h"

#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"

#include "hardware/timer.h"

//arquivo .pio
#include "pio_matrix.pio.h"

//chama o arquivo que contem os desenhos
#include "desenho.h"

#define NUM_PIXELS 25
#define btA 5
#define btB 6
#define Matrix_LED 7
#define LED_RED 13

static volatile uint32_t a = 0;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

//rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){

    uint32_t current_time = to_us_since_boot(get_absolute_time());
    printf("contador = %d \n", a);

    if(current_time - last_time > 200000){
        last_time = current_time;
        if(gpio == btA){
            if(a<9){
                printf("incrementa -> A = %d\n", a);       
                a++;
            }else{
                a = 9;
            }
            
        }else if(gpio == btB){
            if(a>0){
                printf("decrementa -> A = %d\n", a);     
                a--;
            }else{
                a = 0;
            }
        }
    }
}

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){
    for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(desenho[24-i], desenho[24-i], g=0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
    } 
}

int main()
{
    PIO pio = pio0; 
    bool ok;
    uint16_t i;
    uint32_t valor_led;
    double r = 0.0, b = 0.0 , g = 0.0;

    //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    ok = set_sys_clock_khz(128000, false);

    // Inicializa todos os códigos stdio padrão que estão ligados ao binário.
    stdio_init_all();

    if (ok) printf("clock set to %ld\n", clock_get_hz(clk_sys));

    //configurações da PIO
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, Matrix_LED);

     //inicializar o botão de interrupção - GPIO5
    gpio_init(btA);
    gpio_set_dir(btA, GPIO_IN);
    gpio_pull_up(btA);

    //inicializar o botão de interrupção - GPIO5
    gpio_init(btB);
    gpio_set_dir(btB, GPIO_IN);
    gpio_pull_up(btB);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    //interrupção da gpio habilitada
    gpio_set_irq_enabled_with_callback(btA, GPIO_IRQ_EDGE_FALL, 1, & gpio_irq_handler);
    gpio_set_irq_enabled(btB, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        
        for(int i = 0; i < 10; i++){
            if(a == i){
                //rotina para escrever na matriz de leds com o emprego de PIO - desenho 2
                desenho_pio(nums[a], valor_led, pio, sm, r, g, b);
            }
        }
        
        gpio_put(LED_RED, true);
        sleep_ms(100);
        gpio_put(LED_RED, false);
        sleep_ms(100);
    }
}
