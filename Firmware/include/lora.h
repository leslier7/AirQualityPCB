// Robbie Leslie
// April 2026

#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include <Arduino_LoRaWAN_ttn.h>
#include <arduino_lmic.h>
#include <hal/hal.h>

#define PIN_SCLK  3
#define PIN_MOSI  6
#define PIN_MISO  7
#define PIN_CS    10
#define PIN_DIO0  21
#define PIN_DIO1  20

struct __attribute__((__packed__)) pkt_fmt {
      uint8_t intNum; // A 8-bit unsigned integer

      uint8_t myString[6];   // A fixed-size array of 6 bytes to store a short string or byte sequence.

      float floatNum;    // A 32-bit floating-point value

      float thermistorTemp;
      float digitalTemp;
};

class cMyLoRaWAN : public Arduino_LoRaWAN_ttn {
    using Super = Arduino_LoRaWAN_ttn;
public:
    cMyLoRaWAN() {};
protected:
    virtual bool GetOtaaProvisioningInfo(Arduino_LoRaWAN::OtaaProvisioningInfo*) override;
    virtual void NetSaveSessionInfo(const SessionInfo &Info, const uint8_t *pExtraInfo, size_t nExtraInfo) override;
    virtual void NetSaveSessionState(const SessionState &State) override;
    virtual bool NetGetSessionState(SessionState &State) override;
    virtual bool GetAbpProvisioningInfo(Arduino_LoRaWAN::AbpProvisioningInfo*) override;
};

extern cMyLoRaWAN myLoRaWAN;

bool setup_lora();
void send_lora(const pkt_fmt &pkt);

#ifdef __cplusplus
extern "C" {
#endif

void onEvent(ev_t ev);

#ifdef __cplusplus
}
#endif

#endif
