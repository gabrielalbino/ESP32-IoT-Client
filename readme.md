# Trabalho final

Professor, não deu pra fazer o servidor central, devido a isso deixo abaixo as informações para simular o servidor central via MQTTBOX:

Outro problema: Não consegui identificar o que estou fazendo de errado, porém a interrupção de pulldown não está acontecendo mesmo com o código abaixo setado:
```
gpio_pulldown_en(BOTAO_1);
gpio_pullup_en(BOTAO_1);
```
Porém tenho certeza que a lógica para envio da mudança de estado está correta, pois testei o código utilizando um loop para a detecção de mudança e funcionou. (basta descomentar a linha 232 e comentar a 231 do main.c)

* Servidor:  mqtt://test.mosquitto.org:1883

* Tópico para assistir: fse2020/160028361/#

* Tópico para comunicação: 
* Json para configurar placa: 
```json
{
  "tipo": "cadastro-realizado", 
  "name": "NomeComodo",
  "entrada": "NomeEntrada",
  "saida": "NomeSaida"
}
```
* Json para acionar o led:
```json
{"tipo": "set-output", "status": 0} OU
{"tipo": "set-output", "status": 1}
```