#include "matrix.h"

Matrix::Matrix() : matrix_(
                       MATRIX_WIDTH, MATRIX_HEIGHT, PIN,
                       NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
                       NEO_GRB + NEO_KHZ800),
                   data_{0} {}

void Matrix::init()
{
    this->matrix_.begin();
    this->matrix_.setBrightness(30);
    this->matrix_.setRotation(1);
    this->matrix_.fill(0);
    this->matrix_.show();
}

uint8_t Matrix::width() const
{
    return MATRIX_WIDTH;
}

uint8_t Matrix::height() const
{
    return MATRIX_HEIGHT;
}

void Matrix::foreach (const std::function<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)> &callback) const
{
    for (uint8_t x = 0; x < this->width(); x++)
    {
        for (uint8_t y = 0; y < this->height(); y++)
        {
            callback(x, y, this->data_[x][y][0], this->data_[x][y][1], this->data_[x][y][2]);
        }
    }
}

void Matrix::draw(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x >= this->width() || y >= this->height())
    {
        Serial.printf("Invalid coordinates %d,%d\n", x, y);
        return;
    }

    data_[x][y][0] = r;
    data_[x][y][1] = g;
    data_[x][y][2] = b;

    uint16_t color = this->matrix_.Color(r, g, b);
    this->matrix_.drawPixel(x, y, color);
    this->matrix_.show();
}

void Matrix::draw565Image(const uint16_t *image, size_t size)
{
    size_t pixelCount = this->width() * this->height();
    if (size < pixelCount)
    {
        return;
    }

    for (uint16_t y = 0; y < this->height(); y++)
    {
        for (uint16_t x = 0; x < this->width(); x++)
        {
            uint16_t color = image[(y * 16) + x];
            this->updateDataFrom565Value(x, y, color);
            this->matrix_.drawPixel(x, y, color);
        }
    }
    this->matrix_.show();
}

void Matrix::updateDataFrom565Value(uint8_t x, uint8_t y, uint16_t color)
{
    uint16_t r5 = (color & 0xf800) >> 11;
    uint16_t g6 = (color & 0x07e0) >> 5;
    uint16_t b5 = color & 0x001f;

    uint8_t r8 = (r5 * 527 + 23) >> 6;
    uint8_t g8 = (g6 * 259 + 33) >> 6;
    uint8_t b8 = (b5 * 527 + 23) >> 6;

    this->data_[x][y][0] = r8;
    this->data_[x][y][1] = g8;
    this->data_[x][y][2] = b8;
}