#include "../features.h"

#if FEATURE_BLE

#include <NimBLEDevice.h>
#include <Arduino.h>
#include "BLE.h"
#include "json.h"

#define TAG "BLE"
#include "logger.h"

#define RDZ_BLE_SERVICE_UUID    "226c9470-dafe-40df-a51e-1aa03e502cb7"

static NimBLEServer* pServer;
static NimBLECharacteristic* statusChar;

BLE::BLE() {

}

void BLE::init(void) {

    LOG_I(TAG, "Initializing\n");

    NimBLEDevice::init("rdzTTGOsonde");
    NimBLEDevice::setPower(3); /** +3db */

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true);

    NimBLEService* pService = pServer->createService(RDZ_BLE_SERVICE_UUID);
        
    statusChar = pService->createCharacteristic("226c9470-dafe-40df-a51e-000000000001", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, 1024);
    statusChar->setValue("");

    NimBLEDescriptor* statusDesc = statusChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 20);
    statusDesc->setValue("Status");
    NimBLE2904* status2904 = statusChar->create2904();
    status2904->setFormat(NimBLE2904::FORMAT_UTF8);

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

    char buf[1024]; // TODO: Write directly to characteristic buffer?

    strcpy(buf, "{\"sonde\": {");
    sonde2json(buf + strlen(buf), 1024, si);
    strcat(buf, "}}");

    LOG_D(TAG, "Writing %d to BLE Char\n", strlen(buf));
    LOG_D(TAG, "Char: %s\n", buf);

    statusChar->setValue((const char*)buf);
    statusChar->notify();
}


BLE connBLE;

#endif