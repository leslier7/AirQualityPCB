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
    uint16_t ch4;        // 0–10000, scale ×100 → 0.00–100.00 % vol,   res 0.01%
    uint16_t h2s;        // 0–5000,  scale ×100 → 0.00–50.00 ppm,      res 0.01 ppm
    uint16_t nox;        // 0–30000, scale ×100 → 0.00–300.00 ppm,     res 0.01 ppm
    uint16_t voc_load;       // 0–500,   IAQ index (BME688 processed output, no scaling)
    int16_t  temp;       // -4000–8500, scale ×100 → -40.00–85.00 °C,  res 0.01 °C
    uint16_t humidity;   // 0–10000, scale ×100 → 0.00–100.00 % RH,    res 0.01 %
    uint16_t pressure;
    uint8_t iaq_accuracy;
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
