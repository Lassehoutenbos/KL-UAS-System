#include <unity.h>
#include "TempSensors.h"
#include "leds.h"
#include "pins.h"
#include "Arduino.h"


void setUp(void) {
    // nothing
}

void tearDown(void) {
    // nothing
}

static uint16_t adc_value_for_temp(float c) {
    return static_cast<uint16_t>((c / 330.0f) * 4095.0f);
}

void test_temperature_conversion() {
    TempSensors t;
    t.begin();
    analogValues[PIN_SENS2] = adc_value_for_temp(33.0f);
    analogValues[PIN_SENS3] = adc_value_for_temp(44.0f);
    analogValues[PIN_SENS4] = adc_value_for_temp(55.0f);
    analogValues[PIN_SENS5] = adc_value_for_temp(66.0f);
    t.update();
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 33.0f, t.getTemperatureC(0));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 44.0f, t.getTemperatureC(1));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 55.0f, t.getTemperatureC(2));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 66.0f, t.getTemperatureC(3));
}

void test_fan_speed_mapping() {
    TempSensors t;
    t.begin();
    analogValues[PIN_SENS2] = adc_value_for_temp(30.0f); // below nominal
    analogValues[PIN_SENS3] = adc_value_for_temp(70.0f); // above max
    analogValues[PIN_SENS4] = adc_value_for_temp(50.0f); // mid
    analogValues[PIN_SENS5] = adc_value_for_temp(70.0f); // above max
    t.update();
    TEST_ASSERT_EQUAL_UINT16(4095, rgbDriver.channels[FAN1_PWM_CH]);
    TEST_ASSERT_EQUAL_UINT16(4095, rgbDriver.channels[FAN2_PWM_CH]);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_temperature_conversion);
    RUN_TEST(test_fan_speed_mapping);
    return UNITY_END();
}
