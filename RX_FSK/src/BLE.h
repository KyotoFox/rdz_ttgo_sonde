#ifndef _BLE_H
#define _BLE_H

#include "Sonde.h"
class BLE {

public:
    BLE();
    void init();
    void loop();

    void updateSonde( SondeInfo *si );
};

extern BLE connBLE;

#endif