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

## Fluxo de Funcionamento

O sistema pode ser entendido através da analogia de uma **orquestra**:

* **O Maestro (`setup()` em `main.cpp`):** Prepara o ambiente uma única vez, inicializa os módulos, cria as ferramentas do FreeRTOS (Mutex, Timer, Semáforo) e cria as tarefas (os "músicos"). Ao final, ele inicia o Timer e entrega o controle ao escalonador do FreeRTOS.

* **A Performance (Tarefas em Paralelo):**
    1.  **`pid_controller_task` (O Solista):** A tarefa mais crítica. Ela fica adormecida até ser acordada a cada 200ms por um sinal do Timer. Ao acordar, ela lê o estado do sistema, seleciona a planta no MUX, lê o sensor, calcula o novo sinal de controle PID e o aplica na planta via DAC. Seu ritmo é preciso e ininterrupto.
    2.  **`combination_selector_task` (O Percussionista):** Em um ritmo mais lento (a cada 5 segundos), esta tarefa muda o estado do sistema. Ela alterna entre as combinações de MUX e, eventualmente, entre as Plantas 1 e 2, garantindo que todo o sistema seja testado. Ela reporta cada mudança no monitor serial.
    3.  **`setpoint_generator_task` (O Pianista):** Define a "melodia" (o setpoint). Em um loop próprio, ela ajusta o valor do setpoint ao longo do tempo, criando um perfil de referência dinâmico para o controlador seguir.
    4.  **`serial_plotter_task` (O Locutor):** Em intervalos curtos, esta tarefa lê os dados de setpoint e saída do sistema e os imprime no terminal serial em um formato compatível com o Serial Plotter, permitindo a visualização dos gráficos em tempo real.

* **Sincronização:** Todas as tarefas que precisam ler ou escrever dados compartilhados (como o setpoint `sp` ou a saída `y`) utilizam um **Mutex** (`xStateMutex`). Isso garante que apenas uma tarefa modifique esses dados por vez, evitando condições de corrida e inconsistências.

## Como Compilar e Usar

O projeto foi desenvolvido para ser utilizado com **Visual Studio Code** e a extensão **PlatformIO**.

1.  Clone este repositório.
2.  Abra a pasta do projeto no VS Code. O PlatformIO irá reconhecer o arquivo `platformio.ini`.
3.  Conecte seu ESP32 ao computador.
4.  Use os botões do PlatformIO para **Build** (Compilar) e **Upload** (Enviar) o código para a placa.
5.  Após o upload, abra o **Serial Monitor** do PlatformIO com a velocidade (baud rate) de **115200**. Você verá as mensagens de status, incluindo a troca de plantas e combinações do MUX.
6.  Para visualizar os gráficos, abra o **Serial Plotter**. Você verá duas linhas: uma para o Setpoint (SP) e outra para a Saída da Planta (Y), permitindo analisar a performance do controlador em tempo real.

## Autor

Danilo Santos e Luis Costa