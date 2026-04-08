#include "../features.h"

#if FEATURE_BLE

#include <NimBLEDevice.h>
#include <Arduino.h>
#include <cstring>
#include <cmath>
#include "BLE.h"
#include "json.h"
#include "Sonde.h"

#define TAG "BLE"
#include "logger.h"

#define RDZ_BLE_SERVICE_UUID    "226c9470-dafe-40df-a51e-1aa03e502cb7"

static NimBLEServer* pServer;
static NimBLECharacteristic* statusChar;

/* ------------------------------------------------------------------ */
/*  Minimal protobuf wire-format encoder                              */
/*  See sonde.proto for the message definition used by clients.       */
/* ------------------------------------------------------------------ */

#define PB_WT_VARINT  0
#define PB_WT_32BIT   5
#define PB_WT_BYTES   2

static size_t pb_put_varint(uint8_t *buf, uint64_t v) {
    size_t i = 0;
    while (v > 0x7F) {
        buf[i++] = (uint8_t)((v & 0x7F) | 0x80);
        v >>= 7;
    }
    buf[i++] = (uint8_t)(v & 0x7F);
    return i;
}

static size_t pb_put_tag(uint8_t *buf, uint32_t field, uint8_t wt) {
    return pb_put_varint(buf, ((uint64_t)field << 3) | wt);
}

static size_t pb_put_float(uint8_t *buf, uint32_t field, float v) {
    size_t p = pb_put_tag(buf, field, PB_WT_32BIT);
    memcpy(buf + p, &v, 4);
    return p + 4;
}

static size_t pb_put_string(uint8_t *buf, uint32_t field, const char *s) {
    size_t len = strlen(s);
    if (len == 0) return 0;
    size_t p = pb_put_tag(buf, field, PB_WT_BYTES);
    p += pb_put_varint(buf + p, len);
    memcpy(buf + p, s, len);
    return p + len;
}

static size_t pb_put_uint32(uint8_t *buf, uint32_t field, uint32_t v) {
    size_t p = pb_put_tag(buf, field, PB_WT_VARINT);
    p += pb_put_varint(buf + p, v);
    return p;
}

static size_t pb_put_sint32(uint8_t *buf, uint32_t field, int32_t v) {
    uint32_t zz = ((uint32_t)v << 1) ^ (uint32_t)(v >> 31);
    size_t p = pb_put_tag(buf, field, PB_WT_VARINT);
    p += pb_put_varint(buf + p, zz);
    return p;
}

/* ------------------------------------------------------------------ */
/*  Encode SondeInfo into a protobuf blob (field numbers match        */
/*  sonde.proto).  Float fields that are NaN are omitted entirely,    */
/*  giving the client a clean "not present" signal via proto3         */
/*  optional semantics.                                               */
/* ------------------------------------------------------------------ */

static size_t encodeSondeProtobuf(uint8_t *buf, size_t bufsize, SondeInfo *si) {
    SondeData *s = &(si->d);
    size_t p = 0;

    p += pb_put_string(buf + p,  1, s->ser);

    if (!isnan(s->lat))              p += pb_put_float(buf + p,  2, s->lat);
    if (!isnan(s->lon))              p += pb_put_float(buf + p,  3, s->lon);
    if (!isnan(s->alt))              p += pb_put_float(buf + p,  4, s->alt);
    if (!isnan(s->vs))               p += pb_put_float(buf + p,  5, s->vs);
    if (!isnan(s->hs))               p += pb_put_float(buf + p,  6, s->hs);
    if (!isnan(s->dir))              p += pb_put_float(buf + p,  7, s->dir);

    p += pb_put_string(buf + p,  8, getType(si));
    p += pb_put_uint32(buf + p,  9, s->frame);
    p += pb_put_uint32(buf + p, 10, s->time);
    p += pb_put_float (buf + p, 11, si->freq);
    p += pb_put_float (buf + p, 12, (float)si->rssi);
    p += pb_put_uint32(buf + p, 13, s->sats);

    if (!isnan(s->temperature))      p += pb_put_float(buf + p, 14, s->temperature);
    if (!isnan(s->relativeHumidity)) p += pb_put_float(buf + p, 15, s->relativeHumidity);
    if (!isnan(s->pressure))         p += pb_put_float(buf + p, 16, s->pressure);

    p += pb_put_string(buf + p, 17, s->id);
    p += pb_put_sint32(buf + p, 18, s->vframe);
    p += pb_put_sint32(buf + p, 19, si->afc);
    p += pb_put_uint32(buf + p, 20, s->launchKT);
    p += pb_put_uint32(buf + p, 21, s->burstKT);
    p += pb_put_uint32(buf + p, 22, s->countKT);
    p += pb_put_uint32(buf + p, 23, s->crefKT);
    p += pb_put_string(buf + p, 24, si->launchsite);
    p += pb_put_uint32(buf + p, 25, (uint32_t)si->rxStat[0]);

    if (s->batteryVoltage > 0)       p += pb_put_float(buf + p, 26, s->batteryVoltage);

    p += pb_put_uint32(buf + p, 27, s->validPos);

    return p;
}

/* ------------------------------------------------------------------ */

BLE::BLE() {

}

void BLE::init(void) {

    LOG_I(TAG, "Initializing\n");

    NimBLEDevice::init(sonde.config.mdnsname);
    NimBLEDevice::setPower(3); /** +3db */

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true);

    NimBLEService* pService = pServer->createService(RDZ_BLE_SERVICE_UUID);
        
    statusChar = pService->createCharacteristic("226c9470-dafe-40df-a51e-000000000001", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, 512);
    statusChar->setValue("");

    NimBLEDescriptor* statusDesc = statusChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 20);
    statusDesc->setValue("Status");

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(RDZ_BLE_SERVICE_UUID);
    pAdvertising->start();

    LOG_D(TAG, "Advertising started\n");
}

void BLE::loop() {

}

void BLE::updateSonde(SondeInfo *si) {

    LOG_D(TAG, "Updating BLE char\n");

    uint8_t buf[512];
    size_t len = encodeSondeProtobuf(buf, sizeof(buf), si);

    LOG_D(TAG, "Writing %d bytes to BLE Char (protobuf)\n", len);

    statusChar->setValue(buf, len);
    statusChar->notify();
}


BLE connBLE;

#endif
