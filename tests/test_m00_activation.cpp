#include "core/engine/engine_event_processor.h"
#include "core/settings/json_config_loader.h"
#include "core/settings/setting.h"
#include "core/engine/lookup_table.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== M00 Virtual Modifier Activation Test ===" << std::endl;

    // Load vim-mode config
    Setting setting;
    yamy::settings::JsonConfigLoader loader(&std::cout);

    if (!loader.load(&setting, "/home/rmondo/repos/yamy/keymaps/vim-mode.json")) {
        std::cerr << "✗ FAILED: Could not load config!" << std::endl;
        return 1;
    }

    std::cout << "✓ Config loaded" << std::endl;
    std::cout << "  Virtual modifier triggers: " << setting.m_virtualModTriggers.size() << std::endl;
    std::cout << "  Mod tap actions: " << setting.m_modTapActions.size() << std::endl;

    // Create EventProcessor
    yamy::EventProcessor processor;

    // Register virtual modifier triggers from config
    for (const auto& [trigger, modNum] : setting.m_virtualModTriggers) {
        uint16_t tapOutput = 0;
        auto it = setting.m_modTapActions.find(modNum);
        if (it != setting.m_modTapActions.end()) {
            tapOutput = it->second;
        }
        processor.registerVirtualModifierTrigger(trigger, modNum, tapOutput);
        std::cout << "  Registered trigger 0x" << std::hex << trigger
                  << " -> M" << std::dec << (int)modNum
                  << " (tap=0x" << std::hex << tapOutput << std::dec << ")" << std::endl;
    }

    // Get the lookup table from processor and add rules
    // Manually add the critical test rules: M00+H/J/K/L -> arrows
    // These are what the JSON config should compile to
    using namespace yamy::input;

    auto* lookupTable = processor.getLookupTable();
    if (!lookupTable) {
        std::cerr << "✗ FAILED: Could not get lookup table!" << std::endl;
        return 1;
    }

    // M00+H -> LEFT (0xCB)
    {
        yamy::engine::CompiledRule rule;
        rule.outputScanCode = 0xCB; // LEFT
        rule.requiredOn.set(ModifierState::VIRTUAL_OFFSET + 0); // M00 must be ON
        lookupTable->addRule(0x23, rule); // H key
        std::cout << "  Added rule: H (0x23) + M00 -> LEFT (0xCB)" << std::endl;
    }

    // M00+J -> DOWN (0xD0)
    {
        yamy::engine::CompiledRule rule;
        rule.outputScanCode = 0xD0; // DOWN
        rule.requiredOn.set(ModifierState::VIRTUAL_OFFSET + 0);
        lookupTable->addRule(0x24, rule); // J key
        std::cout << "  Added rule: J (0x24) + M00 -> DOWN (0xD0)" << std::endl;
    }

    std::cout << "✓ Lookup table configured" << std::endl;

    // Create modifier state
    yamy::input::ModifierState modState;

    std::cout << "\n=== TEST 1: Press CapsLock (should suppress) ===" << std::endl;
    auto result1 = processor.processEvent(58, yamy::EventType::PRESS, &modState); // evdev 58 = CapsLock
    std::cout << "Result: evdev=" << result1.output_evdev
              << " yamy=0x" << std::hex << result1.output_yamy << std::dec
              << " valid=" << result1.valid << std::endl;

    if (result1.output_evdev == 0) {
        std::cout << "✓ CapsLock press suppressed (correct - waiting for threshold)" << std::endl;
    } else {
        std::cerr << "✗ FAILED: CapsLock should be suppressed, got evdev " << result1.output_evdev << std::endl;
        return 1;
    }

    std::cout << "\n=== TEST 2: Wait 250ms (exceed threshold) ===" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    std::cout << "✓ Threshold time exceeded" << std::endl;

    std::cout << "\n=== TEST 3: Press H with CapsLock held ===" << std::endl;
    auto result2 = processor.processEvent(35, yamy::EventType::PRESS, &modState); // evdev 35 = H
    std::cout << "Result: evdev=" << result2.output_evdev
              << " yamy=0x" << std::hex << result2.output_yamy << std::dec
              << " valid=" << result2.valid << std::endl;

    if (result2.output_evdev == 105) {  // LEFT arrow = evdev 105
        std::cout << "✓ SUCCESS: H remapped to LEFT (M00 activation working!)" << std::endl;
    } else {
        std::cerr << "✗ FAILED: Expected LEFT (evdev 105), got " << result2.output_evdev << std::endl;
        std::cerr << "  This means M00 did not activate properly" << std::endl;
        return 1;
    }

    std::cout << "\n=== TEST 4: Release H ===" << std::endl;
    auto result3 = processor.processEvent(35, yamy::EventType::RELEASE, &modState);
    std::cout << "Result: evdev=" << result3.output_evdev << std::endl;

    std::cout << "\n=== TEST 5: Release CapsLock (should deactivate M00) ===" << std::endl;
    auto result4 = processor.processEvent(58, yamy::EventType::RELEASE, &modState);
    std::cout << "Result: evdev=" << result4.output_evdev << std::endl;

    std::cout << "\n=== TEST 6: Press J without CapsLock (should pass through) ===" << std::endl;
    auto result5 = processor.processEvent(36, yamy::EventType::PRESS, &modState); // evdev 36 = J
    std::cout << "Result: evdev=" << result5.output_evdev
              << " yamy=0x" << std::hex << result5.output_yamy << std::dec << std::endl;

    if (result5.output_evdev == 36) {  // J should pass through
        std::cout << "✓ SUCCESS: J passes through without M00" << std::endl;
    } else {
        std::cerr << "✗ FAILED: J should pass through as evdev 36, got " << result5.output_evdev << std::endl;
        return 1;
    }

    std::cout << "\n=== TEST 7: Tap CapsLock <200ms (should output Escape) ===" << std::endl;
    auto result6 = processor.processEvent(58, yamy::EventType::PRESS, &modState);
    std::cout << "Press result: evdev=" << result6.output_evdev << std::endl;

    // Release quickly (before 200ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto result7 = processor.processEvent(58, yamy::EventType::RELEASE, &modState);
    std::cout << "Release result: evdev=" << result7.output_evdev
              << " yamy=0x" << std::hex << result7.output_yamy << std::dec
              << " is_tap=" << result7.is_tap << std::endl;

    if (result7.output_evdev == 1) {  // Escape = evdev 1
        std::cout << "✓ SUCCESS: CapsLock tap outputs Escape" << std::endl;
    } else {
        std::cerr << "✗ FAILED: Tap should output Escape (evdev 1), got " << result7.output_evdev << std::endl;
        return 1;
    }

    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    return 0;
}
