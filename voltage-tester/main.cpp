#include "mbed.h"
#include <vector>

// 1. SPI PINS (Standard Nucleo-L432KC SPI pins)
#define SPI_MOSI    PA_7
#define SPI_MISO    PA_6
#define SPI_SCK     PA_1

// 2. CHIP SELECT PINS (CS)

PinName adccs_pins[] = {
    PA_9, // U1_CS (CN3_13)
    PA_10,  // U2_CS (CN3_12)
    PA_12,  // U3_CS (CN3_9)
    PB_0,  // U4_CS (CN3_8)
    PB_7,  // U5_CS (CN3_7)
    PB_6,  // U6_CS (CN3_6)
    PB_1, // U7_CS (CN3_5)
    PA_8, // U8_CS (CN3_2)
    PA_11,  // U9_CS (CN3_1)
    PB_5,  // U10 Placeholder
    PB_4,  // U11 Placeholder
    PA_3,  // U12 Placeholder
    PA_0   // U13 Placeholder
};

// 3. VOLTAGE CALIBRATION
// VREF was 5.0 as of 12/18/25 ON THE ADC BOARD DOUBLE CHECK BEFORE RUNNING***
#define VREF_VOLTAGE  5.0f 
#define VOLTAGE_DIVIDER_RATIO 1.0f 

// Main Code

class MCP3208 {
public:
    MCP3208(SPI& _spi, PinName _cs) : spi(_spi), cs(_cs) {
        cs = 1; 
    }

    float readVoltage(int channel) {
        if (channel < 0 || channel > 7) return 0.0f;

        cs = 0; 

        
        uint8_t cmd_byte1 = 0x06 | ((channel >> 2) & 0x01); 
        uint8_t cmd_byte2 = (channel << 6) & 0xC0;          

        spi.write(cmd_byte1);
        uint8_t b1 = spi.write(cmd_byte2); 
        uint8_t b2 = spi.write(0x00);

        cs = 1;

        uint16_t raw_adc = ((b1 & 0x0F) << 8) | b2;

        return (raw_adc / 4095.0f) * VREF_VOLTAGE * VOLTAGE_DIVIDER_RATIO;
    }

private:
    SPI& spi;
    DigitalOut cs;
};


int main() {
   
    printf("\r\n--- 100-Cell Battery Monitor Started ---\r\n");

    SPI spi(SPI_MOSI, SPI_MISO, SPI_SCK);
    spi.format(8, 0); 
    spi.frequency(1000000); 

    int num_adcs = sizeof(adccs_pins) / sizeof(PinName);
    std::vector<MCP3208*> adcs;
    for (int i = 0; i < num_adcs; i++) {
        adcs.push_back(new MCP3208(spi, adccs_pins[i]));
    }

    while (true) {
        printf("\r\n--- New Scan ---\r\n");
        printf("ID, Voltage (V)\r\n"); 

        int battery_count = 0;

        for (int i = 0; i < num_adcs; i++) {
            for (int ch = 0; ch < 8; ch++) {
                battery_count++;
                
                if (battery_count > 100) break;

                float v = adcs[i]->readVoltage(ch);

                printf("Battery_%03d, %.3f\r\n", battery_count, v);
                
                thread_sleep_for(2); 
            }
        }

        ThisThread::sleep_for(1s);
    }
}