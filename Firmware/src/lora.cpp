// Robbie Leslie
// April 2026

#include <Arduino.h>
#include <SPI.h>
#include <arduino_lmic.h>
#include "lora.h"
#include "keys.h"

cMyLoRaWAN myLoRaWAN;

bool g_joined = false;

static uint16_t txCount = 0;

static const cMyLoRaWAN::lmic_pinmap myPinMap = {
    .nss            = PIN_CS,
    .rxtx           = cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN,
    .rst            = cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN,
    .dio            = { PIN_DIO0, PIN_DIO1, cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN },
    .rxtx_rx_active = 0,
    .rssi_cal       = 0,
    .spi_freq       = 8000000,
};

static void sendCallback(void *pCtx, bool fSuccess) {
    if (fSuccess) {
        Serial.printf("TX succeeded (devaddr=0x%08X, fcnt=%d, dr=%d, txpow=%d)\n",
            LMIC.devaddr, LMIC.seqnoUp, LMIC.datarate, LMIC.txpow);
        if (LMIC.txrxFlags & TXRX_ACK)  Serial.println("  Downlink: ACK received");
        if (LMIC.txrxFlags & TXRX_NOPORT && !(LMIC.txrxFlags & TXRX_ACK))
            Serial.println("  Downlink: unconfirmed TX, no downlink expected");
    } else {
        Serial.printf("TX failed (devaddr=0x%08X)\n", LMIC.devaddr);
        Serial.printf("  Frame counter : %d\n", LMIC.seqnoUp);
        Serial.printf("  Data rate     : DR%d\n", LMIC.datarate);
        Serial.printf("  TX power      : %d dBm\n", LMIC.txpow);
        Serial.printf("  opmode        : 0x%04X\n", LMIC.opmode);

        // Decode opmode bitmask
        if (LMIC.opmode & OP_TXDATA)    Serial.println("  [OP_TXDATA]    data waiting to send");
        if (LMIC.opmode & OP_JOINING)   Serial.println("  [OP_JOINING]   join in progress");
        if (LMIC.opmode & OP_TXRXPEND)  Serial.println("  [OP_TXRXPEND]  waiting for RX window");
        if (LMIC.opmode & OP_UNJOIN)    Serial.println("  [OP_UNJOIN]    lost network, re-joining");
        if (LMIC.opmode & OP_SHUTDOWN)  Serial.println("  [OP_SHUTDOWN]  radio shut down");
        if (LMIC.opmode & OP_NEXTCHNL)  Serial.println("  [OP_NEXTCHNL]  waiting for next channel");
        if (LMIC.opmode & OP_LINKDEAD)  Serial.println("  [OP_LINKDEAD]  no downlinks received, link may be dead");
        if (LMIC.txrxFlags & TXRX_NACK) Serial.println("  Downlink: NACK received");

        // The most useful one for your situation:
        if (LMIC.opmode & OP_LINKDEAD) {
            Serial.println("  >>> Link dead: confirmed uplinks not ACKed, gateway may not be hearing you");
        }
    }
}

bool setup_lora() {
    Serial.println("LoRa: initializing SPI...");
    SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);
    Serial.printf("LoRa: SPI started (SCLK=%d MISO=%d MOSI=%d CS=%d)\n",
                  PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);

    Serial.println("LoRa: calling begin()...");
    bool ok = myLoRaWAN.begin(myPinMap);
    Serial.printf("LoRa: begin() returned %s\n", ok ? "OK" : "FAIL");


    return ok;
}

void send_lora(const pkt_fmt &pkt) {
    bool confirmed = (++txCount % DOWNLINK_EVERY == 0);

    Serial.printf("LoRa: queuing TX (ch4=%dppm h2s=%.2fppm nox=%.2fppm vocs=%u temp=%.2fC hum=%.2f%% pres=%.1fhPa)\n",
                  pkt.ch4, pkt.h2s / 100.0f, pkt.nox / 100.0f,
                  pkt.voc_load, pkt.temp / 100.0f, pkt.humidity / 100.0f,
                  pkt.pressure / 10.0f);

    #ifdef LONG_RANGE
    LMIC_setAdrMode(0);              // Disable ADR so TTN can't dial it back down
    LMIC_setDrTxpow(DR_SF10, 14);   // SF10, max transmit power 
    #else
    LMIC_setAdrMode(0);
    LMIC_setDrTxpow(DR_SF9, 14); //Use SF9 to get better range. Can only transmit every 11 minutes on this
    #endif
    

    #ifdef DEBUG_LORA
    myLoRaWAN.SendBuffer(
        (const uint8_t *) &pkt, sizeof(pkt),
        sendCallback, nullptr, true, 1   // true = confirmed, TTN sends ACK (can only get 10 a day). Maybe use for prod
    );
    #else
    myLoRaWAN.SendBuffer(
    (const uint8_t *) &pkt, sizeof(pkt),
        sendCallback, nullptr, confirmed, 1 //Sends a downlink every 24 msgs. Need to go to 10 msgs per hour to get it to not violate TTN fair use
    );
    #endif
}

bool cMyLoRaWAN::GetOtaaProvisioningInfo(OtaaProvisioningInfo *pInfo) {
    Serial.println("LoRa: loading OTAA keys");
    if (pInfo) {
        memcpy_P(pInfo->AppEUI, APPEUI, 8);
        memcpy_P(pInfo->DevEUI, DEVEUI, 8);
        memcpy_P(pInfo->AppKey, APPKEY, 16);
    }
    return true;
}

void cMyLoRaWAN::NetSaveSessionInfo(const SessionInfo &Info, const uint8_t *pExtraInfo, size_t nExtraInfo) {
    Serial.printf("LoRa: join complete — devaddr=0x%08X\n", LMIC.devaddr);
    // not persisting — will re-join on reboot
}

void cMyLoRaWAN::NetSaveSessionState(const SessionState &State) {
    // not persisting — frame counters reset on reboot
}

bool cMyLoRaWAN::NetGetSessionState(SessionState &State) {
    return false;  // no saved state, triggers fresh join
}

bool cMyLoRaWAN::GetAbpProvisioningInfo(Arduino_LoRaWAN::AbpProvisioningInfo *pInfo) {
    return false;  // using OTAA only
}

void onEvent(ev_t ev) {
    switch(ev) {
        case EV_JOINING:    Serial.println("LMIC: joining..."); break;
        case EV_JOINED:     g_joined = true; Serial.printf("LMIC: joined (devaddr=0x%08X)\n", LMIC.devaddr); break;
        case EV_JOIN_FAILED: Serial.println("LMIC: join FAILED"); break;
        case EV_REJOIN_FAILED: Serial.println("LMIC: rejoin FAILED"); break;
        case EV_LINK_DEAD:  Serial.println("LMIC: link dead — no ACKs received"); break;
        case EV_LINK_ALIVE: Serial.println("LMIC: link alive again"); break;
        case EV_TXCOMPLETE: 
            Serial.printf("LMIC: TX complete, FCnt=%d\n", LMIC.seqnoUp);
            if (LMIC.txrxFlags & TXRX_ACK) Serial.println("  ACK received");
            if (LMIC.txrxFlags & TXRX_NACK) Serial.println("  NACK received");
            break;
        default: break;
    }
}