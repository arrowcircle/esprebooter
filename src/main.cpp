#include <Arduino.h>
#include "WiFi.h"
#include <string>
#include <vector>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define RELAY_PIN 8
#define NETWORK String("Ivy Knob")
#define uS_TO_S_FACTOR 1000000
// Uncomment for development | Comment for production
#define SUCCESSFUL_SLEEP_DURATION 60 * 60 // 60 * 60 // 1 hour
#define FAILED_SLEEP_DURATION 60 * 60 // 15 // 60 * 60 // 1 hour
#define ATTEMPT_DURATION 2 * 60 // 2 mins
// Uncomment for production | Comment for development
// #define SUCCESSFUL_SLEEP_DURATION 24 * 60 * 60 // 24 hours
// #define FAILED_SLEEP_DURATION 60 * 60 // 1 hour
#define MAX_ATTEMPTS 5


std::vector<String> networks = {};
int attempts = 0;

void disable_wifi() {
  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

std::vector<String> scan_networks() {
  std::vector<String> result = {};
  WiFi.scanDelete();
  sleep(10);
  Serial.println("Starting scan ...");
  int n = WiFi.scanNetworks();
  Serial.printf("Found %d networks\n", n);
  if (n == 0) { return result; }

  for (int i = 0; i < n; ++i) {
    result.push_back(String(WiFi.SSID(i)));
    // Serial.println(String(WiFi.SSID(i)));
  }
  WiFi.scanDelete();
  return result;
}

void long_sleep(int seconds) {
  disable_wifi();
  esp_sleep_enable_timer_wakeup(seconds * uS_TO_S_FACTOR);
  Serial.flush();
  digitalWrite(RELAY_PIN, LOW);
  gpio_hold_en(GPIO_NUM_8);
  gpio_deep_sleep_hold_en();
  esp_deep_sleep_start();
  // This will never run
  ESP.restart();
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  gpio_hold_dis(GPIO_NUM_8);
  digitalWrite(RELAY_PIN, LOW);
  Serial.begin(115200);
  disable_wifi();
  Serial.println("Setup done");
}

void loop() {
  Serial.printf("Attempt: %d\n", attempts);
  networks = scan_networks();
  if (std::find(networks.begin(), networks.end(), NETWORK) != networks.end()) {
    Serial.println("Network found, sleeping.");
    long_sleep(SUCCESSFUL_SLEEP_DURATION);
  } else {
    Serial.println("Network NOT found, taking short break.");
    attempts++;
  }
  if (attempts > MAX_ATTEMPTS) {
    Serial.printf("Maximum attempts reached: %d. Restarting router.\n", MAX_ATTEMPTS);
    // connect pin
    digitalWrite(RELAY_PIN, HIGH);   // Turn on relay
    delay(2000); 
    // disconnect pin
    digitalWrite(RELAY_PIN, LOW);    // Turn off relay
    // sleep for 1 hour
    long_sleep(FAILED_SLEEP_DURATION);
  }
  // Wait a bit before scanning again.
  delay(ATTEMPT_DURATION * 1000);
}
