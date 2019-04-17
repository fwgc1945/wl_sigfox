/*
    Name:       sigfox_demo2.ino
    Created:	2019/04/16 12:00:36
    Author:     DENSIN:KOORI
*/

#include "SIGFOX.h"

// IMPORTANT: Check these settings with UnaBiz to use the SIGFOX library correctly.
static const String device = "g88pi";  // Set this to your device name if you're using UnaBiz Emulator.
static const bool useEmulator = false;  // Set to true if using UnaBiz Emulator.
static const bool echo = true;  // Set to true if the SIGFOX library should display the executed commands.
static const Country country = COUNTRY_TW;  //Set this to your country to configure the SIGFOX transmission frequencies.
static UnaShieldV2S transceiver(country, useEmulator, device, echo);  // Uncomment this for UnaBiz UnaShield V2S / V2S2 Dev Kit
// static UnaShieldV1 transceiver(country, useEmulator, device, echo);  // Uncomment this for UnaBiz UnaShield V1 Dev Kit

#define echoPin 2 // Echo Pin
#define trigPin 3 // Trigger Pin

void setup() {  // Will be called only once.
    
    // Sensorピンモード設定
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
    Serial.print(F("\nRunning loop #")); Serial.println(counter);

    // Get temperature and voltage of the SIGFOX module.
    float temperature;
    float voltage;
    float distance;
    transceiver.getTemperature(temperature);
    transceiver.getVoltage(voltage);

    // 距離を取得
    distance = getDistance(temperature);

    // sendMessageを作成(12 bytes)
    String msg = transceiver.toHex(temperature) // 4 bytes
        + transceiver.toHex(voltage)    // 4 bytes
        + transceiver.toHex(distance);  // 4 bytes

    // Send the message.
    transceiver.sendMessage(msg);

    counter++;

    // Send only 10 messages.
    if (counter >= 3) {
        //  If more than 10 times, display the results and hang here forever.
        stop(String(F("Messages sent successfully: ")) + successCount +
            F(", failed: ") + failCount);  //  Will never return.
    }

    //  Delay 10 seconds before sending next message.
    Serial.println("Waiting 10 seconds...");
    delay(10000);
}

//距離を取得
int getDistance(float temp) {

    float Duration = 0; //受信した間隔
    float Distance = 0; //距離

    float arrayDistance[8];
    int length = 8;

    for (size_t i = 0; i < length; i++) {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH); //超音波を出力
        delayMicroseconds(10); //
        digitalWrite(trigPin, LOW);
        Duration = pulseIn(echoPin, HIGH); //センサからの入力

        if (Duration > 0) {
            Duration = Duration / 2; //往復距離を半分にする
            float sspeed = 331.5 + 0.6 * temp;
            //Distance = Duration * 340 * 100 / 1000000; // 音速を340m/sに設定
            Distance = Duration * sspeed * 100 / 1000000; // 音速を温度を考慮した値に設定
            Serial.print("Distance:");
            Serial.print(Distance);
            Serial.println(" cm");

            // 値を配列に格納
            arrayDistance[i] = Distance;
        }
        delay(1000);
    }

    // 数値を昇順にソート
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

        // ZERO、最小値、最大値を除くサマリーを作成
        if (arrayDistance[i] != 0) {
            sumDistance += arrayDistance[i];
            ix1 += 1;
        }
    }
    
    Serial.print("return:");
    Serial.println(sumDistance / ix1);
    Serial.println(sumDistance);
    Serial.println(ix1);

    // 平均値を返す
    return sumDistance / ix1;
}
