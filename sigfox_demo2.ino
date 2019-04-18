/*
    Name:       sigfox_demo2.ino
    Created:	2019/04/16 12:00:36
    Author:     DENSIN:KOORI
*/

#include "SIGFOX.h"
#include <avr/sleep.h>
#include <avr/wdt.h>

// IMPORTANT: Check these settings with UnaBiz to use the SIGFOX library correctly.
static const String device = "g88pi";  // Set this to your device name if you're using UnaBiz Emulator.
static const bool useEmulator = false;  // Set to true if using UnaBiz Emulator.
static const bool echo = true;  // Set to true if the SIGFOX library should display the executed commands.
static const Country country = COUNTRY_TW;  //Set this to your country to configure the SIGFOX transmission frequencies.
static UnaShieldV2S transceiver(country, useEmulator, device, echo);  // Uncomment this for UnaBiz UnaShield V2S / V2S2 Dev Kit
// static UnaShieldV1 transceiver(country, useEmulator, device, echo);  // Uncomment this for UnaBiz UnaShield V1 Dev Kit

#define normally 100    //���펞(cm)
#define coefficient 0.2 //�W��

#define echoPin 2 // Echo Pin
#define trigPin 3 // Trigger Pin

/*****************************************/
/*
 * �E�H�b�`�h�b�O�����̎Q�l��:2014/11/17 ���W�I�y���`���� http://radiopench.blog96.fc2.com/
 */
 /*****************************************/
void delayWDT_setup(unsigned int ii) { // �E�H�b�`�h�b�O�^�C�}�[���Z�b�g�B
 // ������WDTCSR�ɃZ�b�g����WDP0-WDP3�̒l�B�ݒ�l�Ɠ��쎞�Ԃ͊T�����L
 // 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
 // 6=1sec, 7=2sec, 8=4sec, 9=8sec
    byte bb;
    if (ii > 9) { // �ςȒl��r��
        ii = 9;
    }
    bb = ii & 7; // ����3�r�b�g��bb��
    if (ii > 7) { // 7�ȏ�i7.8,9�j�Ȃ�
        bb |= (1 << 5); // bb��5�r�b�g��(WDP3)��1�ɂ���
    }
    bb |= (1 << WDCE);

    MCUSR &= ~(1 << WDRF); // MCU Status Reg. Watchdog Reset Flag ->0
    // start timed sequence
    WDTCSR |= (1 << WDCE) | (1 << WDE); // �E�H�b�`�h�b�O�ύX���iWDCE��4�T�C�N���Ŏ������Z�b�g�j
    // set new watchdog timeout value
    WDTCSR = bb; // ���䃌�W�X�^��ݒ�
    WDTCSR |= _BV(WDIE);
}

ISR(WDT_vect) { // WDT���^�C���A�b�v�������Ɏ��s����鏈��
 // wdt_cycle++; // �K�v�Ȃ�R�����g�A�E�g���O��
}

void delayWDT(unsigned long t) { // �p���[�_�E�����[�h��delay�����s
    Serial.println("Goodnight!"); //���쒆�̕\��
    delay(100);

    delayWDT_setup(t); // �E�H�b�`�h�b�O�^�C�}�[���荞�ݏ����ݒ�
    ADCSRA &= ~(1 << ADEN); // ADEN�r�b�g���N���A����ADC���~�i120��A�ߖ�j
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // �p���[�_�E�����[�h
    sleep_enable();

    sleep_mode(); // �����ŃX���[�v�ɓ���

    sleep_disable(); // WDT���^�C���A�b�v�ł������瓮��ĊJ 
    ADCSRA |= (1 << ADEN); // ADC�̓d����ON (|=��!=�ɂȂ��Ă����o�O���C��2014/11/17)
}

void setup() {
    
    // Sensor�s�����[�h�ݒ�
    pinMode(echoPin, INPUT);
    pinMode(trigPin, OUTPUT);
                
    // Initialize console so we can see debug messages (9600 bits per second).
    Serial.begin(9600);  Serial.println(F("Running setup..."));
    // Check whether the SIGFOX module is functioning.
    if (!transceiver.begin()) stop(F("Unable to init SIGFOX module, may be missing"));  //  Will never return.

    // Delay 10 seconds before sending next message.
    Serial.println(F("Waiting 10 seconds..."));
    delay(10000);
}

void loop() {
  // Send message distance, temperature and voltage as a structured SIGFOX message.
    static int counter = 0, successCount = 0, failCount = 0;  //  Count messages sent and failed.
    static float savDistance = normally;

    Serial.print(F("\nRunning loop #")); Serial.println(counter);

    // Get temperature and voltage of the SIGFOX module.
    float temperature;
    float voltage;
    float distance;
    transceiver.getTemperature(temperature);
    transceiver.getVoltage(voltage);

    // �������擾
    distance = getDistance(temperature);

    //�ϓ��l�����߂�
    float result = distance / savDistance;
    if (distance > savDistance)
    {
        result = result - 1;
    }
    else
    {
        result = 1- result;
    }

    //�v���񐔂����Z
    counter++;
    
    Serial.print("distance:");
    Serial.println(distance);
    Serial.println(savDistance);
    Serial.println(result);
    Serial.print("counter:");
    Serial.println(counter);

    //�v���l�����펞�ȉ��̏ꍇ�A�O��v���l���W���l���ϓ����Ă���ꍇ�A
    //�ߋ�6��v�����đ��M���Ă��Ȃ��ꍇ�ɑ��M
    if (distance <= normally || result > coefficient || counter >= 6)
    {
        // sendMessage���쐬(12 bytes)
        String msg = transceiver.toHex(temperature) // 4 bytes
            + transceiver.toHex(voltage)    // 4 bytes
            + transceiver.toHex(distance);  // 4 bytes

        // Send the message.
        transceiver.sendMessage(msg);
        Serial.println("Send the message!!!");

        //�v���񐔂��N���A
        counter = 0;
    }

    //�v���l��ޔ�
    savDistance = distance;

    //  Delay 10 seconds before sending next message.
    Serial.println("Waiting 10 seconds...");
    
    //�v����10�����i8*75=600�b�X���[�v�j
    for (size_t i = 0; i < 75; i++)
    {
        //�X���[�v��Ԃֈڍs
        delayWDT(9);
    }
}

//�������擾
int getDistance(float temp) {

    float Duration = 0; //��M�����Ԋu
    float Distance = 0; //����

    float arrayDistance[8];
    int length = 8;

    for (size_t i = 0; i < length; i++) {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH); //�����g���o��
        delayMicroseconds(10); //
        digitalWrite(trigPin, LOW);
        Duration = pulseIn(echoPin, HIGH); //�Z���T����̓���

        if (Duration > 0) {
            Duration = Duration / 2; //���������𔼕��ɂ���
            float sspeed = 331.5 + 0.6 * temp;
            //Distance = Duration * 340 * 100 / 1000000; // ������340m/s�ɐݒ�
            Distance = Duration * sspeed * 100 / 1000000; // ���������x���l�������l�ɐݒ�
            Serial.print("Distance:");
            Serial.print(Distance);
            Serial.println(" cm");

            // �l��z��Ɋi�[
            arrayDistance[i] = Distance;
        }
        delay(1000);
    }

    // ���l�������Ƀ\�[�g
    float tmp;
    for (size_t i = 0; i < length; ++i) {
        for (size_t j = i + 1; j < length; ++j) {
            if (arrayDistance[i] > arrayDistance[j]) {
                tmp = arrayDistance[i];
                arrayDistance[i] = arrayDistance[j];
                arrayDistance[j] = tmp;
            }
        }
    }

    int ix1 = 0;
    float sumDistance = 0;
    for (size_t i = 1; i < length -1 ; i++) {

        // ZERO�A�ŏ��l�A�ő�l�������T�}���[���쐬
        if (arrayDistance[i] != 0) {
            sumDistance += arrayDistance[i];
            ix1 += 1;
        }
    }
    
    Serial.print("return:");
    Serial.println(sumDistance / ix1);
    Serial.println(sumDistance);
    Serial.println(ix1);

    // ���ϒl��Ԃ�
    return sumDistance / ix1;
}
