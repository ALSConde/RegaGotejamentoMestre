# RegaGotejamentoMestre
Projeto do Mestre para o sistema de rega. É necessario configurar o dispositivo a ser usado, e atribuir um Identificador(ID) ao dispositivo.

Capacidade de alteração de parametros via protocolo MQTT, formato atual de envio dos dados: ID,ParametroUmidade. Ex 1,75.60

# TODO LIST:
Atribuição automatica de ID.
Retrabalho da funcao de comunicação MQTT
Adequação do padrão de envio para algo Parecido com JSON. Ex: "Id:1,Umidade:75.35"