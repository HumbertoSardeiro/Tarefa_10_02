#include <stdio.h>  // Inclui a biblioteca para funções de entrada/saída padrão
#include "pico/stdlib.h"  // Inclui a biblioteca para funcionalidades específicas do Raspberry Pi Pico
#include "hardware/adc.h"  // Inclui a biblioteca para controlar o ADC (Conversor Analógico para Digital)
#include "hardware/pwm.h"  // Inclui a biblioteca para controle de PWM (Modulação por Largura de Pulso)
#include "hardware/gpio.h"  // Inclui a biblioteca para controle de GPIO (Pinos de Entrada/Saída)
#include "inc/ssd1306.h"  // Inclui a biblioteca para controlar o display OLED SSD1306

// Definição dos pinos utilizados
#define VRy_PIN 26  // Pino de entrada analógica para o eixo Y do joystick
#define VRx_PIN 27  // Pino de entrada analógica para o eixo X do joystick
#define BOTAO_JOYSTICK 22  // Pino para o botão embutido no joystick
#define BOTAO_A 5  // Pino para um botão externo adicional
#define LED_PIN_G 11  // Pino do LED verde
#define LED_PIN_B 12  // Pino do LED azul
#define LED_PIN_R 13  // Pino do LED vermelho
#define I2C_PORT i2c1  // Porta I2C utilizada para o display OLED
#define I2C_SDA 14  // Pino SDA para o I2C
#define I2C_SCL 15  // Pino SCL para o I2C
#define endereco 0x3C  // Endereço I2C do display SSD1306
#define PWM_WRAP 4095  // Valor máximo do PWM (4095 para 12 bits)
#define valor_centro 2048  // Valor central do ADC, utilizado para comparar a posição do joystick
#define DEBOUNCE_DELAY 200  // Tempo de debounce em milissegundos para os botões

// Variáveis de estado
bool led_g_estado = false;  // Variável para armazenar o estado do LED verde
bool pwm_enabled = true;  // Variável para controlar se o PWM está habilitado ou não
uint32_t ultimo_botao_joystick = 0;  // Armazena o tempo do último pressionamento do botão do joystick
uint32_t ultimo_botao = 0;  // Armazena o tempo do último pressionamento do botão A

ssd1306_t ssd;  // Estrutura que representa o display SSD1306
int estilo_borda = 0;  // Variável para controlar o estilo da borda no display

// Função para inicializar o PWM no pino especificado
uint pwm_init_gpio(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);  // Define o pino como função PWM
    uint slice_num = pwm_gpio_to_slice_num(gpio);  // Obtém o número do slice de PWM associado ao pino
    pwm_set_wrap(slice_num, PWM_WRAP);  // Define o valor máximo do PWM
    pwm_set_enabled(slice_num, true);  // Habilita o PWM
    return slice_num;  // Retorna o número do slice para referência futura
}

// Função para calcular a intensidade do LED com base no valor do ADC
uint16_t calcular_intensidade_led(uint16_t value) {
    if (value > valor_centro) {
        return (value - valor_centro) * 2;  // Se o valor for maior que o centro, aumenta a intensidade
    } else if (value < valor_centro) {
        return (valor_centro - value) * 2;  // Se o valor for menor que o centro, diminui a intensidade
    } else {
        return 0;  // Se o valor for exatamente o centro, intensidade zero
    }
}

// Função para desenhar a borda no display OLED
void desenhar_borda(int style) {
    switch (style) {
        case 0: 
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);  // Desenha uma borda simples
            break;
        case 1:
            // Desenha bordas múltiplas para um efeito mais sofisticado
            ssd1306_rect(&ssd, 4, 4, 120, 56, true, false);
            ssd1306_rect(&ssd, 1, 1, 126, 62, true, false);
            ssd1306_rect(&ssd, 2, 2, 124, 60, true, false);
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
            break;
        case 2:
            ssd1306_fill(&ssd, false);  // Limpa o display (sem preenchimento)
            break;
        default:
            break;
    }
    ssd1306_send_data(&ssd);  // Atualiza o display com as alterações
}

// Callback de interrupção para os botões
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());  // Obtém o tempo atual em milissegundos

    // Verifica debounce para o botão do joystick
    if (gpio == BOTAO_JOYSTICK && (current_time - ultimo_botao_joystick >= DEBOUNCE_DELAY)) {
        ultimo_botao_joystick = current_time;  // Atualiza o tempo do último botão pressionado
        led_g_estado = !led_g_estado;  // Alterna o estado do LED verde
        gpio_put(LED_PIN_G, led_g_estado);  // Atualiza o LED verde

        // Alterna o estilo da borda no display
        estilo_borda = (estilo_borda + 1) % 3;  // Faz um ciclo entre 3 estilos de borda
        desenhar_borda(estilo_borda);  // Atualiza a borda do display
    }
    
    // Verifica debounce para o botão A
    if (gpio == BOTAO_A && (current_time - ultimo_botao >= DEBOUNCE_DELAY)) {
        ultimo_botao = current_time;  // Atualiza o tempo do último botão pressionado
        pwm_enabled = !pwm_enabled;  // Alterna o estado do PWM (ligar/desligar)
        if (!pwm_enabled) {
            pwm_set_gpio_level(LED_PIN_R, 0);  // Desliga o LED vermelho
            pwm_set_gpio_level(LED_PIN_B, 0);  // Desliga o LED azul
        }
    }
}

float posicao_x_anterior = -1;  // Variável para armazenar a última posição do eixo X
float posicao_y_anterior = -1;  // Variável para armazenar a última posição do eixo Y

int main() {
    stdio_init_all();  // Inicializa a comunicação serial para depuração
    adc_init();  // Inicializa o ADC para leituras analógicas

    // Configura os pinos do joystick (eixos X e Y) como entradas analógicas
    adc_gpio_init(VRy_PIN);
    adc_gpio_init(VRx_PIN);

    // Inicializa o PWM para os LEDs
    uint pwm_slice_R = pwm_init_gpio(LED_PIN_R);  // Inicializa o PWM para o LED vermelho
    uint pwm_slice_B = pwm_init_gpio(LED_PIN_B);  // Inicializa o PWM para o LED azul

    // Configuração dos botões (com pull-up interno)
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);  // Configura o botão do joystick como entrada
    gpio_pull_up(BOTAO_JOYSTICK);  // Habilita o pull-up interno
    gpio_set_irq_enabled_with_callback(BOTAO_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);  // Habilita interrupção para queda de nível no botão do joystick
    
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);  // Configura o botão A como entrada
    gpio_pull_up(BOTAO_A);  // Habilita o pull-up interno
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);  // Habilita interrupção para queda de nível no botão A

    // Configuração do LED verde
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);  // Configura o LED verde como saída

    // Inicializa o I2C e o display OLED SSD1306
    i2c_init(I2C_PORT, 400 * 1000);  // Inicializa o I2C na porta especificada com 400kHz de velocidade
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Configura o pino SDA como I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);  // Configura o pino SCL como I2C
    gpio_pull_up(I2C_SDA);  // Habilita o pull-up para o pino SDA
    gpio_pull_up(I2C_SCL);  // Habilita o pull-up para o pino SCL
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);  // Inicializa o display OLED SSD1306
    ssd1306_config(&ssd);  // Configura o display SSD1306
    ssd1306_fill(&ssd, false);  // Limpa o display (sem preenchimento)
    ssd1306_send_data(&ssd);  // Envia os dados para o display

    uint32_t ultimo_tempo = 0;  // Variável para armazenar o último tempo de execução

    while (true) {
        adc_select_input(0);  // Seleciona a entrada do eixo Y do joystick
        uint16_t vry_value = adc_read();  // Lê o valor analógico do eixo Y

        adc_select_input(1);  // Seleciona a entrada do eixo X do joystick
        uint16_t vrx_value = adc_read();  // Lê o valor analógico do eixo X

        if (pwm_enabled) {
            // Ajustar brilho dos LEDs proporcionalmente ao joystick
            uint16_t led_intensidade_B = calcular_intensidade_led(vry_value);
            uint16_t led_intensidade_R = calcular_intensidade_led(vrx_value);

            pwm_set_gpio_level(LED_PIN_B, led_intensidade_B);
            pwm_set_gpio_level(LED_PIN_R, led_intensidade_R);
        }

        float posicao_y = (4095 - (float)vry_value) / 4095 * 56;
        float posicao_x = (float)vrx_value / 4095 * 120;

        // Limpar a posição anterior, se houver
        if (posicao_y_anterior != -1 && posicao_x_anterior != -1) {
            ssd1306_rect(&ssd, posicao_y_anterior, posicao_x_anterior, 8, 8, false, true);  // Apaga o ponto anterior
        }
        desenhar_borda(estilo_borda);

        // Atualiza as variáveis da posição anterior
        posicao_x_anterior = posicao_x;
        posicao_y_anterior = posicao_y;

        ssd1306_rect(&ssd, posicao_y, posicao_x, 8, 8, true, true);
        ssd1306_send_data(&ssd);

        float duty_cycle_R = (calcular_intensidade_led(vry_value) / 4095.0) * 100;
        float duty_cycle_B = (calcular_intensidade_led(vrx_value) / 4095.0) * 100;

        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - ultimo_tempo >= 1000) {
            printf("VRx: %u | Duty Cycle R: %.0f%%\n", vry_value, duty_cycle_R);
            printf("VRy: %u | Duty Cycle B: %.0f%%\n", vrx_value, duty_cycle_B);
            ultimo_tempo = current_time;
        }

        sleep_ms(100);
    }

    return 0;
}