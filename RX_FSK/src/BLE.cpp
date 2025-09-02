#include "../features.h"

#if FEATURE_BLE

#include <NimBLEDevice.h>
#include <Arduino.h>
#include "BLE.h"
#include "json.h"

#define TAG "BLE"
#include "logger.h"

// #include <BLEDevice.h>
// #include <BLEAdvertising.h>

// static BLEScan *bleScanTest;

static NimBLEServer* pServer;
static NimBLECharacteristic* statusChar;

BLE::BLE() {

}

void BLE::init(void) {

    Serial.printf("Starting BLE!\n");

    NimBLEDevice::init("RDZ TTGO Sonde");
    //NimBLEDevice::setPower(3); /** +3db */

    //NimBLEDevice::setSecurityAuth(true, true, false); /** bonding, MITM, don't need BLE secure connections as we are using passkey pairing */
    //NimBLEDevice::setSecurityPasskey(123456);
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); /** Display only passkey */
    
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true);

    NimBLEService* pService = pServer->createService("ABCD");
    // NimBLECharacteristic* pSecureCharacteristic =
    //     pService->createCharacteristic("1235",
    //                                    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);
    //pSecureCharacteristic->setValue("Hello Secure BLE");

    
    statusChar = pService->createCharacteristic("0001", NIMBLE_PROPERTY::READ, 1024);
    statusChar->setValue("");

    NimBLEDescriptor* statusDesc = statusChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 20);
    statusDesc->setValue("Status");
    NimBLE2904* status2904 = statusChar->create2904();
    status2904->setFormat(NimBLE2904::FORMAT_UTF8);

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("ABCD");
    pAdvertising->start();

    Serial.printf("Advertising Started\n");
}

void BLE::loop() {

}


void BLE::updateSonde(SondeInfo *si) {

    LOG_D(TAG, "Updating BLE char\n");

    char buf[1024];

    strcpy(buf, "{\"sonde\": {");
    sonde2json(buf + strlen(buf), 1024, si);
    strcat(buf, "}}");

    LOG_D(TAG, "Writing %d to BLE Char\n", strlen(buf));
    LOG_D(TAG, "Char: %s\n", buf);

    statusChar->setValue((const char*)buf);
}


BLE bleInstance;

#endif