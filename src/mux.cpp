// src/mux.cpp

#include "mux.h"
#include "config.h"

/**
 * @brief Inicializa os pinos de controle do multiplexador.
 */
void mux_init() {
    pinMode(MUX_IN_A_PIN, OUTPUT);
    pinMode(MUX_IN_B_PIN, OUTPUT);
}

/**
 * @brief Seleciona a planta E a combinação de pinos do MUX.
 * @param plant_id O ID da planta (1 ou 2).
 * @param combination O índice da combinação de pinos.
 */
void mux_select_plant(int plant_id, int combination) {
    if (plant_id == 1) {
        // Lógica para as 4 combinações da Planta 1 usando operadores bitwise
        // combination 0 (0b00) -> A=LOW, B=LOW
        // combination 1 (0b01) -> A=LOW, B=HIGH
        // combination 2 (0b10) -> A=HIGH, B=LOW
        // combination 3 (0b11) -> A=HIGH, B=HIGH
        digitalWrite(MUX_IN_A_PIN, (combination & 0b10) ? HIGH : LOW); // Checa o segundo bit
        digitalWrite(MUX_IN_B_PIN, (combination & 0b01) ? HIGH : LOW); // Checa o primeiro bit
    } else if (plant_id == 2) {
        // Lógica para as 2 combinações da Planta 2
        // combination 0 -> A=LOW, B=LOW
        // combination 1 -> A=LOW, B=HIGH
        digitalWrite(MUX_IN_A_PIN, LOW); // Fixo em LOW para Planta 2
        digitalWrite(MUX_IN_B_PIN, (combination == 1) ? HIGH : LOW);
    }
}

/**
 * @brief Reporta no terminal qual planta e combinação estão selecionadas.
 * @param plant_id O ID da planta (1 ou 2).
 * @param combination O índice da combinação de pinos.
 */
void mux_report_selection(int plant_id, int combination) {
    char buffer[100]; // Buffer para formatar a string de saída

    if (plant_id == 1) {
        // Valida a combinação para a planta 1
        if (combination < 0 || combination > 3) return;
        const char* pin_a_state = (combination & 0b10) ? "HIGH" : "LOW";
        const char* pin_b_state = (combination & 0b01) ? "HIGH" : "LOW";
        sprintf(buffer, ">> Planta 1 | Combinação %d (MUX: IN_A=%s, IN_B=%s)", combination, pin_a_state, pin_b_state);
    } else if (plant_id == 2) {
        // Valida a combinação para a planta 2
        if (combination < 0 || combination > 1) return;
        const char* pin_b_state = (combination == 1) ? "HIGH" : "LOW";
        sprintf(buffer, ">> Planta 2 | Combinação %d (MUX: IN_A=LOW, IN_B=%s)", combination, pin_b_state);
    } else {
        sprintf(buffer, ">> Planta desconhecida!");
    }

    Serial.println(); // Adiciona uma linha em branco para melhor visualização
    Serial.println(buffer);
    Serial.flush(); // Garante que a mensagem seja enviada imediatamente
}