SMOC - Sistema de Monitoramento e Otimização de Compressores Pneumáticos

Visão Geral do Projeto

O Sistema de Monitoramento e Otimização de Compressores Pneumáticos (SMOC) é uma iniciativa de IoT em andamento, visando automatizar e aprimorar o controle de compressores industriais e o monitoramento de seus parâmetros operacionais. O projeto integra hardware embarcado (ESP32), comunicação sem fio (Wi-Fi, LoRaWAN), um backend robusto (Flask, MQTT, SQLite) e uma interface web para visualização e controle.

Esta documentação descreve a arquitetura atual do projeto e as próximas etapas de desenvolvimento, com foco na integração de funcionalidades críticas e na otimização de tempo real.

Estrutura Atual do Repositório

A estrutura do projeto está organizada para facilitar o desenvolvimento e a manutenção dos diferentes módulos:
<img width="1313" height="356" alt="image" src="https://github.com/user-attachments/assets/551d935a-2c41-43a1-8f3c-767f6fd23129" />


Arquitetura Atual do Sistema

O SMOC opera com uma arquitetura distribuída, onde múltiplos ESP32s desempenham funções específicas, comunicando-se através de Wi-Fi e LoRaWAN.
1. ESP32DPA10P-P (Sensor de Pressão): Coleta e pré-processa os dados de pressão.
2. SCT-013 (Sensor de Corrente): Coleta dados de corrente.
3. ESP32LoRaAP (Agregador LoRa): Atua como um gateway local, recebendo dados de ambos os sensores via Wi-Fi e os retransmitindo via LoRaWAN.
4. ESP32LoRaRxTx (Receptor LoRa para MQTT): Recebe os dados agregados via LoRaWAN e os publica no broker MQTT via Wi-Fi.
5. Backend (Flask, MQTT, SQLite): Hospedado em um Raspberry Pi (ou similar), consome os dados MQTT, armazena-os no SQLite e provê uma interface web.
6. CONTROLCOMP (Controlador de Compressores): Atualmente, um ESP32 que contém a lógica de rodízio e controle dos compressores, recebendo os dados de pressão via Wi-FI e controlando os compressores de acordo com valor pré-determinado de pressão. Visa integrar sua lógica ao sensor de pressão.
   <img width="1422" height="858" alt="image" src="https://github.com/user-attachments/assets/ab625fac-3196-4bae-bb71-240392bcacb6" />


Próximas Etapas e Melhorias Planejadas

O projeto SMOC está em constante evolução. As próximas etapas de desenvolvimento visam consolidar a arquitetura, melhorar a capacidade de tempo real e expandir as funcionalidades da interface do usuário.

1. Integração do Controle de Compressores com o Sensor de Pressão (Prioridade Alta):

Objetivo: Consolidar a lógica de controle dos 4 compressores (atualmente no CONTROLCOMP) diretamente no hardware do ESP32DPA10P-P (sensor de pressão).

Justificativa: Esta integração permitirá que as decisões de ligar/desligar compressores sejam tomadas no mesmo hardware que adquire os dados de pressão, minimizando latências de rede (Wi-Fi/LoRaWAN/MQTT) e garantindo uma resposta em tempo real mais estrita e confiável para o controle de pressão. Isso atende integralmente aos requisitos de tempo real de um sistema crítico de controle.

Ação: Refatorar o firmware do ESP32DPA10P-P para incorporar a lógica de histerese e rodízio de compressores, além da atuação direta nos relés. A comunicação dos comandos de controle para os compressores será então local ao ESP32DPA10P-P.

2. Criação do Controle Manual na Interface Web Principal (index.html):

Objetivo: Adicionar botões na interface index.html para permitir o acionamento manual (ligar/desligar) de cada um dos 4 compressores individualmente.

Justificativa: Oferecer uma camada de controle manual para contingências ou testes, complementando a automação.

Ação: Implementar endpoints no app.py para receber comandos manuais e desenvolver a lógica JavaScript/HTML correspondente em index.html. Estes comandos serão, então, transmitidos ao ESP32 controlador (ou ao novo ESP32DPA10P-P integrado) via MQTT.

3. Criação da Página de Monitoramento Energético (Sensor de Corrente SCT-013):

Objetivo: Desenvolver uma nova seção na interface web para visualizar os dados de consumo de corrente e potência dos compressores, utilizando as leituras do sensor SCT-013.

Justificativa: Permitir o monitoramento energético em tempo real e histórico dos compressores, essencial para otimização de consumo e manutenção preditiva.

Ação: Criar uma nova rota no app.py e um novo template HTML/JavaScript para exibir esses dados e adaptar o app.py para também receber e persistir os dados do SCT-013 via MQTT.

Contribuição

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues para sugestões, relatar bugs ou enviar pull requests com melhorias.
