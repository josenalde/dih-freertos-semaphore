#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <Arduino.h>

#define GREEN_LED 25
#define YELLOW_LED 26
#define BTN_PIN 18

SemaphoreHandle_t SMF;
TaskHandle_t handleIsrTask;

void IRAM_ATTR isr() { // não pode ser mutex dentro de isr
    xSemaphoreGiveFromISR(SMF, NULL); //Libera o semaforo (sem_wait semaphore.h) colocando em estado 1
}

void blinkAndReport(uint8_t pin, uint8_t times, uint16_t durationMs) {
    for (uint8_t i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(durationMs));
        digitalWrite(pin, LOW);
        vTaskDelay(pdMS_TO_TICKS(durationMs));
    }
    Serial.print("Estou no trecho após liberação do semáforo pela isr com SMF= ");
    /* Se é de contagem retorna o valor corrente. 
    Se é binário retorna 1 se disponível e 0 em caso contrário (bloqueado) */
    Serial.println(uxSemaphoreGetCount(SMF));
    Serial.printf("Pisquei o LED no pino %d", pin);        
} 

void handleIsrTask(void* z) {
    while (1) {
         //Tenta obter o semaforo durante 200ms (Timeout). Caso o semaforo nao fique
        //disponivel em 200ms, retornara FALSE
        Serial.printf("Rodando a task handleIsr no core: %d\n",xPortGetCoreID());
        if (xSemaphoreTake(SMF, pdMS_TO_TICKS(200)) == true) { //sem_post
            //Se obteu o semaforo entre os 200ms de espera, fara o toggle do pino
            //Para liberação imediata colocar pdMS_TO_TICKS(0) nao bloqueante
            blinkAndReport(GREEN_LED, 3, 200);
            /* xSemaphoreGive(SMF) não é necessário para sincronização com ISR. 
               Retorna ao início do laço e fica esperando
               o semáforo ser liberado novamente */
        } else {
            blinkAndReport(YELLOW_LED, 3, 200);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // suspensão 100ms, permitindo outra thread caso haja, executar
   }
 }

void setup() {
    //hw config
    Serial.begin(115200);
    pinMode(GREEN_LED, OUTPUT); //green
    pinMode(YELLOW_LED, OUTPUT); //yellow
    pinMode(BTN_PIN, INPUT_PULLUP);    ;
    attachInterrupt(BTN_PIN, isr, FALLING);

    SMF = xSemaphoreCreateBinary(); //semaforo binario
    xTaskCreatePinnedToCore(handleIsrTask, "handleIsrTask" , 4096 , NULL, 1 , &handleIsrTask, 0); 
    //Cria a tarefa que analisa o semaforo
    //handle da função, alias para debug, stack size em bytes, parametro void* para a task, prioridade (1-25), no FreeRTOS ESP32 quanto maior,
    // maior a prioridade, sexto parâmetro não usado (local para guardar ID da task), afinidade do núcleo (0 PRO_CPU, 1 APP_CPU, tskNO_AFFINITY)
    Serial.printf("Rodando o setup no core: %d\n",xPortGetCoreID());
}

void loop() {
    //Serial.printf("Rodando o loop no core: %d\n",xPortGetCoreID());
}
