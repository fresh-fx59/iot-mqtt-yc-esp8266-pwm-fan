#include <ESP8266WiFi.h>
#include <PubSubClient.h> // MQTT Client
#include <secret.h>
#include <fan-pwm.h>
#include <ota.h>

// https://github.com/yandex-cloud/examples/blob/master/iot/Samples/esp8266/Esp8266YandexIoTCoreSample.ino
// https://mysku.club/blog/diy/97421.html

const char *version = "pwm_0.1.4";

const char *mqttserver = "mqtt.cloud.yandex.net";
const int mqttport = 8883;

String topicCommands = String("$devices/") + String(YC_DEVICE_ID) + String("/commands/#");
String topicEvents = String("$devices/") + String(YC_DEVICE_ID) + String("/events/");

const char *test_root_ca =
    "-----BEGIN CERTIFICATE-----\n \
MIIFGTCCAwGgAwIBAgIQJMM7ZIy2SYxCBgK7WcFwnjANBgkqhkiG9w0BAQ0FADAf\
MR0wGwYDVQQDExRZYW5kZXhJbnRlcm5hbFJvb3RDQTAeFw0xMzAyMTExMzQxNDNa\
Fw0zMzAyMTExMzUxNDJaMB8xHTAbBgNVBAMTFFlhbmRleEludGVybmFsUm9vdENB\
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAgb4xoQjBQ7oEFk8EHVGy\
1pDEmPWw0Wgw5nX9RM7LL2xQWyUuEq+Lf9Dgh+O725aZ9+SO2oEs47DHHt81/fne\
5N6xOftRrCpy8hGtUR/A3bvjnQgjs+zdXvcO9cTuuzzPTFSts/iZATZsAruiepMx\
SGj9S1fGwvYws/yiXWNoNBz4Tu1Tlp0g+5fp/ADjnxc6DqNk6w01mJRDbx+6rlBO\
aIH2tQmJXDVoFdrhmBK9qOfjxWlIYGy83TnrvdXwi5mKTMtpEREMgyNLX75UjpvO\
NkZgBvEXPQq+g91wBGsWIE2sYlguXiBniQgAJOyRuSdTxcJoG8tZkLDPRi5RouWY\
gxXr13edn1TRDGco2hkdtSUBlajBMSvAq+H0hkslzWD/R+BXkn9dh0/DFnxVt4XU\
5JbFyd/sKV/rF4Vygfw9ssh1ZIWdqkfZ2QXOZ2gH4AEeoN/9vEfUPwqPVzL0XEZK\
r4s2WjU9mE5tHrVsQOZ80wnvYHYi2JHbl0hr5ghs4RIyJwx6LEEnj2tzMFec4f7o\
dQeSsZpgRJmpvpAfRTxhIRjZBrKxnMytedAkUPguBQwjVCn7+EaKiJfpu42JG8Mm\
+/dHi+Q9Tc+0tX5pKOIpQMlMxMHw8MfPmUjC3AAd9lsmCtuybYoeN2IRdbzzchJ8\
l1ZuoI3gH7pcIeElfVSqSBkCAwEAAaNRME8wCwYDVR0PBAQDAgGGMA8GA1UdEwEB\
/wQFMAMBAf8wHQYDVR0OBBYEFKu5xf+h7+ZTHTM5IoTRdtQ3Ti1qMBAGCSsGAQQB\
gjcVAQQDAgEAMA0GCSqGSIb3DQEBDQUAA4ICAQAVpyJ1qLjqRLC34F1UXkC3vxpO\
nV6WgzpzA+DUNog4Y6RhTnh0Bsir+I+FTl0zFCm7JpT/3NP9VjfEitMkHehmHhQK\
c7cIBZSF62K477OTvLz+9ku2O/bGTtYv9fAvR4BmzFfyPDoAKOjJSghD1p/7El+1\
eSjvcUBzLnBUtxO/iYXRNo7B3+1qo4F5Hz7rPRLI0UWW/0UAfVCO2fFtyF6C1iEY\
/q0Ldbf3YIaMkf2WgGhnX9yH/8OiIij2r0LVNHS811apyycjep8y/NkG4q1Z9jEi\
VEX3P6NEL8dWtXQlvlNGMcfDT3lmB+tS32CPEUwce/Ble646rukbERRwFfxXojpf\
C6ium+LtJc7qnK6ygnYF4D6mz4H+3WaxJd1S1hGQxOb/3WVw63tZFnN62F6/nc5g\
6T44Yb7ND6y3nVcygLpbQsws6HsjX65CoSjrrPn0YhKxNBscF7M7tLTW/5LK9uhk\
yjRCkJ0YagpeLxfV1l1ZJZaTPZvY9+ylHnWHhzlq0FzcrooSSsp4i44DB2K7O2ID\
87leymZkKUY6PMDa4GkDJx0dG4UXDhRETMf+NkYgtLJ+UIzMNskwVDcxO4kVL+Hi\
Pj78bnC5yCw8P5YylR45LdxLzLO68unoXOyFz1etGXzszw8lJI9LNubYxk77mK8H\
LpuQKbSbIERsmR+QqQ==\
-----END CERTIFICATE-----\n";

WiFiClientSecure net;
PubSubClient client(net);
BearSSL::X509List x509(test_root_ca);

#define DEBUG_SERIAL Serial
#define DEBUG_SERIAL_BAUDRATE 9600

void maxSpeedLightUpLed()
{
  digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
  SetFanLevel(100);
}

void minSpeedTurnOffLed()
{
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  SetFanLevel(0);
}

void connect()
{
  delay(5000);
  DEBUG_SERIAL.print("Conecting to wifi ...");
  while (WiFi.status() != WL_CONNECTED)
  {
    DEBUG_SERIAL.print(".");
    delay(1000);
  }
  DEBUG_SERIAL.println(" Connected");
  net.setInsecure();
  DEBUG_SERIAL.print("Connecting to Yandex IoT Core as ");
  DEBUG_SERIAL.print(YC_DEVICE_ID);
  DEBUG_SERIAL.print(" ...");
  while (!client.connect("Esp8266Client", YC_DEVICE_ID, MQTT_PASSWORD))
  {
    DEBUG_SERIAL.print(".");
    delay(1000);
  }
  DEBUG_SERIAL.println(" Connected");
  if (client.publish(topicEvents.c_str(), YC_DEVICE_ID))
  {
    DEBUG_SERIAL.println("Publish ok");
  }
  else
  {
    DEBUG_SERIAL.println("Publish failed");
  }
  DEBUG_SERIAL.println("Subscribe to: ");
  DEBUG_SERIAL.print(topicCommands.c_str());
  client.subscribe(topicCommands.c_str());
}

void performAction(byte *payload)
{
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
  {
    DEBUG_SERIAL.println("(char)payload[0] == '1'");
    maxSpeedLightUpLed();
  }
  else
  {
    DEBUG_SERIAL.println("(char)payload[0] != '1'");
    minSpeedTurnOffLed();
  }
}

void messageReceived(char *topic, byte *payload, unsigned int length)
{
  String topicString = String(topic);
  DEBUG_SERIAL.print("Message received. Topic: ");
  DEBUG_SERIAL.println(topicString.c_str());
  String payloadStr = "";
  for (int i = 0; i < length; i++)
  {
    payloadStr += (char)payload[i];
  }
  DEBUG_SERIAL.print("Payload: ");
  DEBUG_SERIAL.println(payloadStr);

  performAction(payload);
}

void setup()
{
  InitFan();
  pinMode(LED_BUILTIN, OUTPUT);
  DEBUG_SERIAL.begin(DEBUG_SERIAL_BAUDRATE);
  delay(10);
  DEBUG_SERIAL.println("Device started");
  DEBUG_SERIAL.println(version);
  WiFi.begin(SSID, SSID_PASSWORD);
  client.setServer(mqttserver, mqttport);
  client.setCallback(messageReceived);
  client.setBufferSize(1024);
  client.setKeepAlive(15);
  maxSpeedLightUpLed();
  connect();
  setupOta();
}

void loop()
{
  // put your main code here, to run repeatedly:
  CalcFanRPM();
  client.loop();
  if (!client.connected())
  {
    DEBUG_SERIAL.println("Trying to connect");
    connect();
  }
  loopOta();
}