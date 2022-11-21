/*
     CÓDIGO:	  Q0962
     AUTOR:		  BrincandoComIdeias
     APRENDA: 	https://cursodearduino.net/ ; https://cursoderobotica.net
     SKETCH:    Controle IR
     DATA:		  04/11/2022
*/

#include <Arduino.h>

//#if RAMEND <= 0x4FF || (defined(RAMSIZE) && RAMSIZE < 0x4FF)
//#define RAW_BUFFER_LENGTH  120
//#elif RAMEND <= 0xAFF || (defined(RAMSIZE) && RAMSIZE < 0xAFF) // 0xAFF for LEONARDO
//#define RAW_BUFFER_LENGTH  500 // 600 is too much here, because we have additional uint8_t rawCode[RAW_BUFFER_LENGTH];
//#else
#define RAW_BUFFER_LENGTH  750
//#endif

#define MARK_EXCESS_MICROS  20 // 20 is recommended for the cheap VS1838 modules

#define pinLED 27

#include "PinDefinitionsAndMore.h" // Define macros for input and output pin etc.
#include <IRremote.hpp>            

// Storage for the recorded code
struct storedIRDataStruct {
    IRData receivedIRData;  // extensions for sendRaw
    uint8_t rawCode[RAW_BUFFER_LENGTH]; // The durations if raw
    uint16_t rawCodeLength; // The length of the code
} sStoredIRData[10];

int STATUS_PIN = LED_BUILTIN;

int DELAY_BETWEEN_REPEAT = 50;
int DEFAULT_NUMBER_OF_REPEATS_TO_SEND = 1;

int estadoAnt = 0; // VARIÁVEL PARA CONTROLAR A TROCA DE MODO DE OPERAÇÃO

void storeCode(IRData *aIRReceivedData);
void sendCode(storedIRDataStruct *aIRDataToSend);

void setup() {
    pinMode(pinLED, OUTPUT);
    digitalWrite(pinLED, LOW);

    Serial.begin(115200);

    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK);
    
    pinMode(STATUS_PIN, OUTPUT);

    Serial.println(F("Para gravar digite '#' seguido da posicao entre 0 e 9"));
    Serial.println(F("Para enviar digite o numero entre 0 e 9 de um codigo ja gravado"));
}

void loop() {
    static int index = -1;
    static int estadoAtual = 0;

    //RECEBE COMANDO DO SERIAL
    if (Serial.available()) {
      char recebido = Serial.read();

      if (recebido == '#') {
        // MODO GRAVAÇÃO
        estadoAtual = 1;
        Serial.println(F("Modo Gravacao"));
        delay(50);
        recebido = Serial.read();
        index = atoi(&recebido); // CONVERTE DE CHAR PARA INTEIRO
        Serial.print(F("Aguardando Codigo #"));
        Serial.println(recebido);
        digitalWrite(pinLED, HIGH);

      } else if ( recebido >= '0' && recebido <= '9'){
        // MODO ENVIO
        Serial.println(F("Modo Envio"));
        estadoAtual = 2;
        index = atoi(&recebido); // CONVERTE DE CHAR PARA INTEIRO

      } else {
        IrReceiver.stop();
        Serial.println(F("Receptor desabilitado."));
        estadoAtual = 0;
      }
    }

    //INICIA GRAVAÇÃO DE COMANDO
    if (estadoAtual != estadoAnt && estadoAtual == 1) {
        IrReceiver.start();
    }

    //ENVIA COMANDO
    if (estadoAtual == 2 && index >= 0) {
        IrReceiver.stop();

        Serial.println(F("Enviando..."));
        digitalWrite(STATUS_PIN, HIGH);
        sendCode(&sStoredIRData[index]);
        digitalWrite(STATUS_PIN, LOW);

        index = -1; // INDEX < 0 PARA NÃO REPETIR O ENVIO
    } 

    //GRAVA COMANDO
    if (estadoAtual == 1 && IrReceiver.available()) {
        storeCode(IrReceiver.read(), index);
        IrReceiver.resume(); // resume receiver
        digitalWrite(pinLED, LOW);
    }

    estadoAnt = estadoAtual;
}

void storeCode(IRData *aIRReceivedData, int index) {
    if (aIRReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) {
        Serial.println(F("Ignore repeat"));
        return;
    }
    if (aIRReceivedData->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
        Serial.println(F("Ignore autorepeat"));
        return;
    }
    if (aIRReceivedData->flags & IRDATA_FLAGS_PARITY_FAILED) {
        Serial.println(F("Ignore parity error"));
        return;
    }
    /*
     * Copy decoded data
     */
    sStoredIRData[index].receivedIRData = *aIRReceivedData;

    if (sStoredIRData[index].receivedIRData.protocol == UNKNOWN) {
        Serial.print(F("Received unknown code and store "));
        Serial.print(IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
        Serial.println(F(" timing entries as raw "));
        IrReceiver.printIRResultRawFormatted(&Serial, true); // Output the results in RAW format
        sStoredIRData[index].rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
        /*
         * Store the current raw data in a dedicated array for later usage
         */
        IrReceiver.compensateAndStoreIRResultInArray(sStoredIRData[index].rawCode);
    } else {
        IrReceiver.printIRResultShort(&Serial);
        IrReceiver.printIRSendUsage(&Serial);
        sStoredIRData[index].receivedIRData.flags = 0; // clear flags -esp. repeat- for later sending
        Serial.println();
    }
}

void sendCode(storedIRDataStruct *aIRDataToSend) {
    if (aIRDataToSend->receivedIRData.protocol == UNKNOWN /* i.e. raw */) {
        // Assume 38 KHz
        IrSender.sendRaw(aIRDataToSend->rawCode, aIRDataToSend->rawCodeLength, 38);

        Serial.print(F("Sent raw "));
        Serial.print(aIRDataToSend->rawCodeLength);
        Serial.println(F(" marks or spaces"));
    } else {

        /*
         * Use the write function, which does the switch for different protocols
         */
        IrSender.write(&aIRDataToSend->receivedIRData, DEFAULT_NUMBER_OF_REPEATS_TO_SEND);

        Serial.print(F("Sent: "));
        printIRResultShort(&Serial, &aIRDataToSend->receivedIRData, false);
    }
}
