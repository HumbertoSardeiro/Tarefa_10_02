# Controle de LEDs e Display OLED com Joystick no Raspberry Pi Pico

## Descrição

Este projeto utiliza um Raspberry Pi Pico para controlar um conjunto de LEDs RGB e um display OLED SSD1306 através de um joystick analógico e botões. O joystick é usado para ajustar a intensidade dos LEDs e mover um cursor no display OLED. O sistema também suporta botões para alternar o estado dos LEDs e modificar a interface no display.

## Funcionalidades

Leitura de joystick analógico via ADC para controle de LEDs e interface OLED.

Controle de LEDs RGB utilizando PWM para ajustar a intensidade.

Exibição de informações no display OLED SSD1306 via protocolo I2C.

Interrupções nos botões para alternar LEDs e modificar o display.

## Componentes Utilizados

Raspberry Pi Pico

Joystick analógico (com botão)

Display OLED SSD1306 (I2C)

LED RGB (3 canais PWM)

Botão

Resistores e jumpers

Ligações dos Componentes

Componente

Pino Raspberry Pi Pico

Joystick eixo X
GP27 (ADC1)

Joystick eixo Y
GP26 (ADC0)

Botão do joystick
GP22

Botão A
GP5

LED Vermelho (R)
GP13 (PWM)

LED Verde (G)
GP11 (Digital)

LED Azul (B)
GP12 (PWM)

OLED SDA
GP14 (I2C1 SDA)

OLED SCL
GP15 (I2C1 SCL)

## Uso do Projeto

Movimente o joystick para alterar a intensidade dos LEDs RGB.

Pressione o botão do joystick para alternar estilos da borda no display OLED.

Pressione o botão A para ativar/desativar os LEDs RGB.

## Vídeo Demonstrativo

https://youtu.be/12pJ4eZjSww
