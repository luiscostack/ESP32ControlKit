// src/plant.cpp

#include "plant.h"
#include "config.h"

/**
 * @brief Inicializa os pinos de ADC e DAC.
 */
void plant_init() {
    // Configura os pinos de ADC
    pinMode(ADC_PIN_PLANT_1, INPUT);
    pinMode(ADC_PIN_PLANT_2, INPUT);

    // Configura os pinos de DAC (embora a função dac_output_enable já faça isso)
    dac_output_enable(DAC_CHANNEL_1); // Para DAC_PIN_PLANT_1 (GPIO 25)
    dac_output_enable(DAC_CHANNEL_2); // Para DAC_PIN_PLANT_2 (GPIO 26)

    // Zera as saídas DAC iniciais
    dac_output_voltage(DAC_CHANNEL_1, 0);
    dac_output_voltage(DAC_CHANNEL_2, 0);
}

/**
 * @brief Lê o valor do ADC da planta selecionada e converte para tensão.
 * @param plant_id O ID da planta (1 ou 2).
 * @return A tensão lida em Volts.
 */
double plant_read_voltage(int plant_id) {
    int raw_adc = 0;
    if (plant_id == 1) {
        raw_adc = analogRead(ADC_PIN_PLANT_1);
    } else if (plant_id == 2) {
        raw_adc = analogRead(ADC_PIN_PLANT_2);
    }
    return (double)raw_adc * (VCC / (double)ADC_RESOLUTION);
}

/**
 * @brief Escreve o sinal de controle no DAC da planta selecionada.
 * @param plant_id O ID da planta (1 ou 2).
 * @param control_signal_u O sinal de controle (esperado na faixa 0-255).
 */
void plant_write_control(int plant_id, double control_signal_u) {
    // Garante que o sinal esteja dentro dos limites do DAC de 8 bits
    if (control_signal_u > DAC_RESOLUTION) control_signal_u = DAC_RESOLUTION;
    if (control_signal_u < 0) control_signal_u = 0;
    
    uint8_t dac_value = (uint8_t)control_signal_u;

    if (plant_id == 1) {
        dac_output_voltage(DAC_CHANNEL_1, dac_value);
    } else if (plant_id == 2) {
        dac_output_voltage(DAC_CHANNEL_2, dac_value);
    }
}