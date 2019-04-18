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

#define normally 100    //平常時(cm)
#define coefficient 0.2 //係数

#define echoPin 2 // Echo Pin
#define trigPin 3 // Trigger Pin

/*****************************************/
/*
 * ウォッチドッグ処理の参考元:2014/11/17 ラジオペンチさん http://radiopench.blog96.fc2.com/
 */
 /*****************************************/
void delayWDT_setup(unsigned int ii) { // ウォッチドッグタイマーをセット。
 // 引数はWDTCSRにセットするWDP0-WDP3の値。設定値と動作時間は概略下記
 // 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
 // 6=1sec, 7=2sec, 8=4sec, 9=8sec
    byte bb;
    if (ii > 9) { // 変な値を排除
        ii = 9;
    }
    bb = ii & 7; // 下位3ビットをbbに
    if (ii > 7) { // 7以上（7.8,9）なら
        bb |= (1 << 5); // bbの5ビット目(WDP3)を1にする
    }
    bb |= (1 << WDCE);

    MCUSR &= ~(1 << WDRF); // MCU Status Reg. Watchdog Reset Flag ->0
    // start timed sequence
    WDTCSR |= (1 << WDCE) | (1 << WDE); // ウォッチドッグ変更許可（WDCEは4サイクルで自動リセット）
    // set new watchdog timeout value
    WDTCSR = bb; // 制御レジスタを設定
    WDTCSR |= _BV(WDIE);
}

ISR(WDT_vect) { // WDTがタイムアップした時に実行される処理
 // wdt_cycle++; // 必要ならコメントアウトを外す
}

void delayWDT(unsigned long t) { // パワーダウンモードでdelayを実行
    Serial.println("Goodnight!"); //動作中の表示
    delay(100);

    delayWDT_setup(t); // ウォッチドッグタイマー割り込み条件設定
    ADCSRA &= ~(1 << ADEN); // ADENビットをクリアしてADCを停止（120μA節約）
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // パワーダウンモード
    sleep_enable();

    sleep_mode(); // ここでスリープに入る

    sleep_disable(); // WDTがタイムアップでここから動作再開 
    ADCSRA |= (1 << ADEN); // ADCの電源をON (|=が!=になっていたバグを修正2014/11/17)
}

void setup() {
    
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
    static float savDistance = normally;

    Serial.print(F("\nRunning loop #")); Serial.println(counter);

    // Get temperature and voltage of the SIGFOX module.
    float temperature;
    float voltage;
    float distance;
    transceiver.getTemperature(temperature);
    transceiver.getVoltage(voltage);

    // 距離を取得
    distance = getDistance(temperature);

    //変動値を求める
    float result = distance / savDistance;
    if (distance > savDistance)
    {
        result = result - 1;
    }
    else
    {
        result = 1- result;
    }

    //計測回数を加算
    counter++;
    
    Serial.print("distance:");
    Serial.println(distance);
    Serial.println(savDistance);
    Serial.println(result);
    Serial.print("counter:");
    Serial.println(counter);

    //計測値が平常時以下の場合、前回計測値より係数値分変動している場合、
    //過去6回計測して送信していない場合に送信
    if (distance <= normally || result > coefficient || counter >= 6)
    {
        // sendMessageを作成(12 bytes)
        String msg = transceiver.toHex(temperature) // 4 bytes
            + transceiver.toHex(voltage)    // 4 bytes
            + transceiver.toHex(distance);  // 4 bytes

        // Send the message.
        transceiver.sendMessage(msg);
        Serial.println("Send the message!!!");

        //計測回数をクリア
        counter = 0;
    }

    //計測値を退避
    savDistance = distance;

    //  Delay 10 seconds before sending next message.
    Serial.println("Waiting 10 seconds...");
    
    //計測は10分毎（8*75=600秒スリープ）
    for (size_t i = 0; i < 75; i++)
    {
        //スリープ状態へ移行
        delayWDT(9);
    }
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
