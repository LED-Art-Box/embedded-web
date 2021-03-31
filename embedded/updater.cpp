#include "updater.h"

#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "constants.h"

void OTAUpdater::updateIfNeeded()
{
    Serial.printf("Version: %s\n", VERSION);
    String url = LATEST_RELEASE_INFO_URL;

    HTTPClient http;
    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200)
    {
        StaticJsonDocument<2000> doc;
        deserializeJson(doc, http.getStream());

        String latest = doc["tag_name"].as<String>();

        if (latest != VERSION)
        {
            Serial.printf("Needs Update from %s to %s\n", VERSION, latest.c_str());
            update();
        }
    }

    http.end();
}

String OTAUpdater::followRedirect(String url, int count, int times)
{
    if (count >= times)
    { //stop following redirects after n times
        return url;
    }

    const char *headerKeys[] = {"Location"};
    const size_t numberOfHeaders = 1;

    HTTPClient http;

    http.begin(url);
    http.collectHeaders(headerKeys, numberOfHeaders);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200)
    {
        http.end();
        return url;
    }
    else if (httpResponseCode == 302)
    {
        String nextUrl = http.header(headerKeys[0]);
        http.end();
        return followRedirect(nextUrl, count++, times);
    }
    else
    {
        return url;
    }
}

void OTAUpdater::update()
{
    Serial.println("Starting update");
    String resourceUrl = followRedirect(LATEST_FIRMWARE_URL, 0, 4);

    WiFiClientSecure wiFiClientSecure;
    wiFiClientSecure.setCACert(rootCACertificate);
    t_httpUpdate_return ret = httpUpdate.update(wiFiClientSecure, resourceUrl);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}
