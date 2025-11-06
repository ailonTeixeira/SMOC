SMOC - Sistema de Monitoramento e Otimização de Compressores Pneumáticos

Visão Geral do Projeto

O Sistema de Monitoramento e Otimização de Compressores Pneumáticos (SMOC) é uma iniciativa de IoT em andamento, visando automatizar e aprimorar o controle de compressores industriais e o monitoramento de seus parâmetros operacionais. O projeto integra hardware embarcado (ESP32), comunicação sem fio (Wi-Fi, LoRaWAN), um backend robusto (Flask, MQTT, SQLite) e uma interface web para visualização e controle.

Esta documentação descreve a arquitetura atual do projeto e as próximas etapas de desenvolvimento, com foco na integração de funcionalidades críticas e na otimização de tempo real.

Estrutura Atual do Repositório

A estrutura do projeto está organizada para facilitar o desenvolvimento e a manutenção dos diferentes módulos:

SMOC/
├── CONTROLCOMP/         # Firmware otimizado com FreeRTOS para o ESP32 receber a pressão da rede e controlar os compressores.
│   └── (CONTROLCOMP.ino) # Atualmente gerencia a lógica de histerese da pressão e o rodízio e controle de 4 compressores.
├── ESP32DPA10P-P/       # Firmware otimizado com FreeRTOS para o ESP32 de aquisição de dados do sensor de pressão (4-20mA).
│   └── (ESP32DPA10P-P.ino) # Coleta, calibração e envia os dados de pressão via Wi-Fi para o ESP32LoRaAP.
├── ESP32LoRaAP/         # Firmware para ESP32 LoRa atuando como Access Point.
│   └── (ESP32LoRaAP.ino) # Responsável por receber dados de corrente do SCT-013 e dados de pressão do ESP32DPA10P-P via Wi-Fi e retransmitir ambos para o ESP32LoRaRxTx via LoRaWAN.
├── ESP32LoRaRxTx/       # Firmware do módulo LoRa de recepção.
│   └── (ESP32LoRaRxTx.ino) # Recebe os dados consolidados dos sensores do ESP32LoRaAP via LoRaWAN e os envia para o BROKER MQTT via Wi-Fi.
├── SCT-013/             # Firmware para ESP32 do sensor de corrente SCT-013.
│   └── (SCT-013.ino)    # Envia as leituras de corrente para o ESP32LoRaAP via Wi-Fi.
├── templates/           # Arquivos HTML da interface web.
│   ├── index.html       # Interface principal para visualização da pressão atual e controle.
│   └── dados.html       # Interface para visualização de dados históricos de pressão.
├── README.md            # Este arquivo de documentação.
└── app.py               # Aplicação Flask para o backend web.
    └── (app.py)         # Gerencia o recebimento de dados via MQTT, persistência em SQLite, e serve as interfaces web.

Arquitetura Atual do Sistema

O SMOC opera com uma arquitetura distribuída, onde múltiplos ESP32s desempenham funções específicas, comunicando-se através de Wi-Fi e LoRaWAN.

    ESP32DPA10P-P (Sensor de Pressão): Coleta e pré-processa os dados de pressão.

    SCT-013 (Sensor de Corrente): Coleta dados de corrente.

    ESP32LoRaAP (Agregador LoRa): Atua como um gateway local, recebendo dados de ambos os sensores via Wi-Fi e os retransmitindo via LoRaWAN.

    ESP32LoRaRxTx (Receptor LoRa para MQTT): Recebe os dados agregados via LoRaWAN e os publica no broker MQTT via Wi-Fi.

    Backend (Flask, MQTT, SQLite): Hospedado em um Raspberry Pi (ou similar), consome os dados MQTT, armazena-os no SQLite e provê uma interface web.

    CONTROLCOMP (Controlador de Compressores): Atualmente, um ESP32 que contém a lógica de rodízio e controle dos compressores, recebendo os dados de pressão via Wi-FI e controlando os compressores de acordo com valor pré-determinado de pressão. Visa integrar sua lógica ao sensor de pressão.

Próximas Etapas e Melhorias Planejadas

O projeto SMOC está em constante evolução. As próximas etapas de desenvolvimento visam consolidar a arquitetura, melhorar a capacidade de tempo real e expandir as funcionalidades da interface do usuário.

    Integração do Controle de Compressores com o Sensor de Pressão (Prioridade Alta):

        Objetivo: Consolidar a lógica de controle dos 4 compressores (atualmente no CONTROLCOMP) diretamente no hardware do ESP32DPA10P-P (sensor de pressão).

        Justificativa: Esta integração permitirá que as decisões de ligar/desligar compressores sejam tomadas no mesmo hardware que adquire os dados de pressão, minimizando latências de rede (Wi-Fi/LoRaWAN/MQTT) e garantindo uma resposta em tempo real mais estrita e confiável para o controle de pressão. Isso atende integralmente aos requisitos de tempo real de um sistema crítico de controle.

        Ação: Refatorar o firmware do ESP32DPA10P-P para incorporar a lógica de histerese e rodízio de compressores, além da atuação direta nos relés. A comunicação dos comandos de controle para os compressores será então local ao ESP32DPA10P-P.

    Criação do Controle Manual na Interface Web Principal (index.html):

        Objetivo: Adicionar botões na interface index.html para permitir o acionamento manual (ligar/desligar) de cada um dos 4 compressores individualmente.

        Justificativa: Oferecer uma camada de controle manual para contingências ou testes, complementando a automação.

        Ação: Implementar endpoints no app.py para receber comandos manuais e desenvolver a lógica JavaScript/HTML correspondente em index.html. Estes comandos serão, então, transmitidos ao ESP32 controlador (ou ao novo ESP32DPA10P-P integrado) via MQTT.

    Criação da Página de Monitoramento Energético (Sensor de Corrente SCT-013):

        Objetivo: Desenvolver uma nova seção na interface web para visualizar os dados de consumo de corrente e potência dos compressores, utilizando as leituras do sensor SCT-013.

        Justificativa: Permitir o monitoramento energético em tempo real e histórico dos compressores, essencial para otimização de consumo e manutenção preditiva.

        Ação: Criar uma nova rota no app.py e um novo template HTML/JavaScript para exibir esses dados e adaptar o app.py para também receber e persistir os dados do SCT-013 via MQTT.

Contribuição

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues para sugestões, relatar bugs ou enviar pull requests com melhorias.
