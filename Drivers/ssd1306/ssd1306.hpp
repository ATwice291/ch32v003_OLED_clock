#pragma once

#include <cstdint>
#include "../../Periph/i2c_ch32v00x.hpp"

enum struct SSD1306MemoryAddressing {horizontal, vertical, page};

class SSD1306 {
public:
    static constexpr uint32_t WIDTH = 128;
    static constexpr uint32_t HEIGHT = 64;
    static constexpr uint32_t BUFFER_SIZE = (WIDTH * HEIGHT / 8 + 8);
    static constexpr uint8_t addressingMode = static_cast<uint8_t>(SSD1306MemoryAddressing::horizontal);
    static constexpr uint8_t contrast = 0xCF;

    SSD1306(I2CInterface i2c, uint8_t devAddress, uint8_t* buffer)
        : _i2c(i2c), _devAddress(devAddress), _buffer(buffer) {}

    void init() {
        static const uint8_t initSequence[] = {
            0xAE,       // display OFF
            0xA8, HEIGHT-1, // MUX ratio
            0xD3, 0x00, // shift
            0x40,       // display start line
            0xA0,
            0xC0,
            //0xA1,       // segment re-map
            //0xC8,       // scan direction
            0xDA, 0x12, // pin config
            0x20, addressingMode, // addressing mode
            0x81, contrast, // contrast
            0xA4,       // disable entire display on
            0xA6,       // Нормальный режим (не инвертированный)
            0xD5, 0xF0, // OSC frequency
            0xD9, 0x22, // pre-charge
            0xD8, 0x20,
            0xDB, 0x40, // VCOMH
            0x8D, 0x14, // charge pump enable
            0xAF        // display ON
        };
        writeCommands(initSequence, sizeof(initSequence));
    }
    void fill(bool isWhite) {
        for(uint32_t i = 0; i < BUFFER_SIZE; i++) {
            _buffer[i] = isWhite?0xFF:0x00;
        }
    }
    void updateScreen() {
        uint8_t commands[] = {0xB0, 0x00, 0x10};
        for(uint8_t i = 0; i < HEIGHT / 8; i++) {
            commands[0] = 0xB0+i;
            writeCommands(commands, sizeof(commands));
            writeData(&_buffer[WIDTH * i], WIDTH);
        }
    }
    void drawPixel(uint8_t x, uint8_t y, bool isWhite) {
        if (x >= WIDTH || y >= HEIGHT) {
            return;
        }
        if (isWhite) {
            _buffer[x + (y / 8) * WIDTH] |= 1 << (y % 8);
        } else {
            _buffer[x + (y / 8) * WIDTH] &= ~(1 << (y % 8));
        }
    }
    void invertPixel(uint8_t x, uint8_t y) {
        if(_buffer[x + (y / 8) * WIDTH] & (1 << (y % 8))) {
            drawPixel(x, y, false);
        } else {
            drawPixel(x, y, true);
        }
    }
private:
    I2CInterface _i2c;
    uint8_t _devAddress;
    uint8_t* _buffer;

    void writeCommands(const uint8_t* commands, uint8_t size) {
        _i2c.memoryWrite(_devAddress, 0x00, I2cMemAddrSize::oneByte, commands, size, 10);
    }
    void writeData(const uint8_t* data, uint32_t size) {
        _i2c.memoryWrite(_devAddress, 0x40, I2cMemAddrSize::oneByte, data, size, 10);
    }
};