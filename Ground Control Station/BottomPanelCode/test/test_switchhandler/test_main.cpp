#include <Arduino.h>
#include <unity.h>
#include <SwitchHandler.h>

static bool callbackState = false;

void test_callback(bool state){
    callbackState = state;
}

void setUp(void){
    callbackState = false;
    setPinState(5, LOW);
    SwitchHandler::reset();
    SwitchHandler::addSwitch(5, test_callback);
}

void tearDown(void){ }

void test_switch_changes(){
    // initial state LOW -> no callback yet
    SwitchHandler::updateAll();
    TEST_ASSERT_FALSE(callbackState);

    // change state to HIGH
    setPinState(5, HIGH);
    SwitchHandler::updateAll();
    TEST_ASSERT_TRUE(callbackState);
}

int main(int argc, char **argv){
    UNITY_BEGIN();
    RUN_TEST(test_switch_changes);
    return UNITY_END();
}
