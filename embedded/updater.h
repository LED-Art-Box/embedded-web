#ifndef LEDART_UPDATER_H_
#define LEDART_UPDATER_H_

#include <Arduino.h>

class OTAUpdater
{
public:
    void updateIfNeeded();
    void update();

private:
    String followRedirect(String url, int count, int times);
};

#endif
