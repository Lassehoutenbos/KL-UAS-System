#pragma once

// Lightweight abstraction for handling toggle switches. Each Switch object
// calls a callback whenever its input state changes.

#include <functional>
#include <vector>

class Switch {
public:
    using Callback = std::function<void(bool)>;

    // Construct a switch object on the given pin with a callback.
    Switch(int pin, Callback cb);
    // Poll the pin and invoke the callback on changes.
    void update();

private:
    int pin;
    bool lastState;
    Callback callback;
};

// Manager for multiple switches
namespace SwitchHandler {
    // Register a switch to be polled.
    void addSwitch(int pin, Switch::Callback callback);
    // Update all registered switches.
    void updateAll();
}
