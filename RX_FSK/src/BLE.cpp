#include "../features.h"

#if FEATURE_BLE

#include <NimBLEDevice.h>
#include <Arduino.h>
#include "BLE.h"

// #include <BLEDevice.h>
// #include <BLEAdvertising.h>

// static BLEScan *bleScanTest;

static NimBLEServer* pServer;

BLE::BLE() {

}
    

void BLE::init(void) {

    Serial.printf("Starting BLE!\n");

    NimBLEDevice::init("RDZ_TTGO_SONDE");
    //NimBLEDevice::setPower(3); /** +3db */

    //NimBLEDevice::setSecurityAuth(true, true, false); /** bonding, MITM, don't need BLE secure connections as we are using passkey pairing */
    //NimBLEDevice::setSecurityPasskey(123456);
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); /** Display only passkey */
    
    NimBLEServer*         pServer                  = NimBLEDevice::createServer();
    NimBLEService*        pService                 = pServer->createService("ABCD");
    NimBLECharacteristic* pNonSecureCharacteristic = pService->createCharacteristic("1234", NIMBLE_PROPERTY::READ);
    // NimBLECharacteristic* pSecureCharacteristic =
    //     pService->createCharacteristic("1235",
    //                                    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);

    pService->start();
    pNonSecureCharacteristic->setValue("Hello Non Secure BLE");
    //pSecureCharacteristic->setValue("Hello Secure BLE");

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("ABCD");
    pAdvertising->start();

    Serial.printf("Advertising Started\n");
}

BLE bleInstance;

#endif