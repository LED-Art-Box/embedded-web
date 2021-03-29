#ifndef MATRIX_H_
#define MATRIX_H_

#include <functional>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 4
#define MATRIX_WIDTH 16
#define MATRIX_HEIGHT 16

class Matrix
{
public:
    Matrix();

    void init();
    void draw(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
    void draw565Image(const uint16_t *image, size_t size);

    void foreach (const std::function<void(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)> &callback) const;
    uint8_t width() const;
    uint8_t height() const;

private:
    void updateDataFrom565Value(uint8_t x, uint8_t y, uint16_t color);

    Adafruit_NeoMatrix matrix_;
    uint8_t data_[MATRIX_WIDTH][MATRIX_HEIGHT][3];
};

#endif
