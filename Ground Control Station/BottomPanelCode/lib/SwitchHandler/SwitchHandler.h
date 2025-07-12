#pragma once

#include <functional>
#include <vector>

class Switch {
public:
    using Callback = std::function<void(bool)>;

    Switch(int pin, Callback cb);
    void update();

private:
    int pin;
    bool lastState;
    Callback callback;
};

// Manager for multiple switches
namespace SwitchHandler {
    void addSwitch(int pin, Switch::Callback callback);
    void updateAll();
    void reset();
}
