# 🚁 Embedded Drone Site Survey (C/C++)

Um sistema embarcado de tempo real desenvolvido em **C/C++** para controle de voo, telemetria e mapeamento tridimensional de redes Wi-Fi (Site Survey) embarcado em um drone.

Este projeto foi arquitetado com foco em **Sistemas de Tempo Real (RTOS)**, baixa latência e design altamente modular, separando as responsabilidades de hardware, controle e comunicação de rede.

## 🏗️ Arquitetura de Software e Hardware

O código foi estruturado utilizando o ecossistema **PlatformIO**, com forte componentização em bibliotecas (Managers) para garantir a escalabilidade e manutenção do firmware:

* **Controle de Voo e Estabilidade:** Implementação de malhas de controle via `PIDManager` e leitura de giroscópio/acelerômetro via `IMUManager`.
* **Navegação e Posicionamento:** Integração com módulos de geolocalização utilizando o `GPSManager`.
* **Comunicação em Tempo Real:** Transmissão bidirecional de dados utilizando WebSockets e processamento de pacotes JSON (`TelemetryManager` e `WiFiManager`).
* **Interface Web Embarcada:** Servidor web nativo servindo uma interface front-end (HTML/JS/CSS) alocada diretamente na memória flash do hardware (`data/`) para monitoramento visual via `WebManager`.
* **Armazenamento de Dados:** Persistência de dados de navegação e redes mapeadas através do `DBManager`.

## 📂 Estrutura do Projeto

A arquitetura respeita o padrão de componentização do PlatformIO:

    embedded-drone-survey/
    ├── data/               # Arquivos estáticos do Web Server nativo (HTML, JS, CSS)
    ├── include/            # Cabeçalhos globais, Definições de Pinos e Structs de Estado
    ├── lib/                # Módulos do Sistema (Core do Drone)
    │   ├── DBManager/      # Persistência de Dados
    │   ├── FlightManager/  # Orquestração de Voo
    │   ├── IMUManager/     # Leitura de Sensores Inerciais
    │   ├── PIDManager/     # Algoritmo de Controle Proporcional-Integral-Derivativo
    │   ├── TelemetryManager/# WebSockets e parsing JSON
    │   └── ...
    ├── src/
    │   └── main.cpp        # Entrypoint do sistema embarcado
    └── platformio.ini      # Configurações de build e gerenciamento de dependências

## 🚀 Tecnologias Utilizadas
* **Linguagem:** C/C++
* **Ambiente de Build:** PlatformIO
* **Hardware:** Microcontrolador NodeMCU ESP8266 com módulos IMU e GPS integrados.
* **Protocolos:** Wi-Fi, WebSockets, I2C/SPI (Sensores), Serial/UART.

## ⚙️ Configuração Prévia

Antes de compilar e enviar o firmware para a placa, é necessário configurar as credenciais da sua rede Wi-Fi local para que o Drone consiga estabelecer a conexão e levantar o servidor web.

Navegue até o arquivo de configuração (onde as variáveis de rede estão definidas) e substitua os valores padrão pelas suas credenciais reais:

```cpp
const char* ssid = "SSID";
const char* password = "PASSWORD";
```

## 💻 Como Compilar e Fazer o Flash

Para compilar e enviar este firmware para a placa, é necessário ter o **PlatformIO** instalado (via VSCode ou CLI).

    # Clonar o repositório
    git clone https://github.com/DiogoDomi/embedded-drone-survey.git
    cd embedded-drone-survey

    # 1. Fazer o upload do File System (Interface Web HTML/JS para a Flash da NodeMCU)
    pio run --target uploadfs

    # 2. Compilar e fazer o Flash do código C++ para a placa
    pio run --target upload

## 📄 Licença
Distribuído sob a licença MIT. Veja `LICENSE` para mais informações.