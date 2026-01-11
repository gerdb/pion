/**
 sebulli.com
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#define PRG_VERSION "v1.0.0"

// https://www.waveshare.com/wiki/RP2040-Zero
#define HV_PWM          (2) // 2= active, 4= not active
#define EPD_RST_PIN     (12)
#define EPD_DC_PIN      (8)
#define EPD_BUSY_PIN    (13)
#define EPD_CS_PIN      (9)
#define EPD_CLK_PIN		(10)
#define EPD_MOSI_PIN	(11)
#define PIEZO           (14)
#define SENSOR          (29)
#define BAT_VOLT        (27)
#define ADC_SENSOR      (3)
#define ADC_BAT_VOLT    (1)

#define ADC_2_MV    (0.8056640625f) // 3300mV/4096
#define ADC_2_V     (0.0008056640625f) // 3.3V/4096

#endif