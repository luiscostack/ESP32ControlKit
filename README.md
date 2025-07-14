# Controle PID Modular com FreeRTOS no ESP32

Este projeto implementa um sistema de controle PID (Proporcional-Integral-Derivativo) digital, modular e multitarefa, utilizando o sistema operacional de tempo real FreeRTOS em um microcontrolador ESP32. O sistema é projetado para controlar duas plantas de processos distintas, selecionadas dinamicamente através de um multiplexador analógico.

## Conceitos Principais

O projeto foi construído sobre três pilares fundamentais:

1.  **Controle PID:** Utiliza o clássico algoritmo de controle PID para minimizar o erro entre um setpoint (referência) desejado e a saída medida de um processo (planta).
2.  **Multitarefa com FreeRTOS:** A lógica é dividida em tarefas independentes que rodam em paralelo, garantindo que operações críticas (como o loop de controle) não sejam bloqueadas por operações mais lentas (como a comunicação serial). Para isso, são utilizadas ferramentas do FreeRTOS como:
    * **Tasks:** Para paralelismo e organização do código.
    * **Mutex:** Para proteger o acesso a dados compartilhados (estado do sistema).
    * **Timers e Semáforos:** Para garantir a execução do controle em intervalos de tempo precisos.
3.  **Arquitetura Modular:** O código é altamente modularizado, separando as responsabilidades em diferentes arquivos. Isso facilita a manutenção, o teste e a reutilização do código.

## Requisitos de Hardware

* Microcontrolador ESP32 (DOIT ESP32 DEVKIT V1).
* Multiplexador Analógico (CD4052BE).
* Componentes para montar as plantas de processo (resistores e capacitores para formar circuitos RC de 1ª e 2ª ordem).
* Protoboard e jumpers para montagem do circuito.

## Estrutura do Projeto

O código-fonte está organizado na pasta `src/` com a seguinte estrutura:

src/
    
    ├── main.cpp                # Ponto de entrada, inicialização e criação das tarefas.
    
    ├── config.h                # Configurações globais, pinos e definições do sistema.
    
    ├── controller.h / .cpp     # Implementação do algoritmo de controle PID.
    
    ├── mux.h / .cpp            # Abstração para controle do hardware do multiplexador.
    
    ├── plant.h / .cpp          # Abstração para leitura (ADC) e escrita (DAC) nas plantas.
    
    └── setpoint.h / .cpp       # Geração do perfil de referência (setpoint) ao longo do tempo.

    └── spiffs_Defs.h / .cpp    # Implementação das funções de armazenamento na memória.

    └── spiffs_Defs.h / .cpp    # Criação do e inicialização objeto websocket

* **Sincronização:** Todas as tarefas que precisam ler ou escrever dados compartilhados (como o setpoint `sp` ou a saída `y`) utilizam um **Mutex** (`xStateMutex`). Isso garante que apenas uma tarefa modifique esses dados por vez, evitando condições de corrida e inconsistências.

## Como Compilar e Usar

O projeto foi desenvolvido para ser utilizado com **Visual Studio Code** e a extensão **PlatformIO**.

1.  Clone este repositório.
2.  Abra a pasta do projeto no VS Code. O PlatformIO irá reconhecer o arquivo `platformio.ini`.
3.  Conecte seu ESP32 ao computador.
4.  Use os botões do PlatformIO para **Build** (Compilar) e **Upload** (Enviar) o código para a placa.
5.  Após o upload, abra o **Serial Monitor** do PlatformIO com a velocidade (baud rate) de **115200**. Você verá as mensagens de status, incluindo a troca de plantas e combinações do MUX.
6.  Para visualizar os gráficos, abra o **Serial Plotter**. Você verá duas linhas: uma para o Setpoint (SP) e outra para a Saída da Planta (Y), permitindo analisar a performance do controlador em tempo real.
7. Para vialização na webpage, copiar o IP gerado para o ESP e exibido no monitor serial e colar em um navegador.

## Autores

Danilo Santos e Luis Costa