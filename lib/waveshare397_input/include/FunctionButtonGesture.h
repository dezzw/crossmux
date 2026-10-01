#pragma once

#include <cstdint>

namespace freeink::input {

// Waveshare ESP32-S3-ePaper-3.97 override (CrossMux vendor copy). Function+dial
// chords emit Up/Down; Left/Right short presses never map to Up/Down by duration.
class FunctionButtonGesture {
 public:
  static constexpr uint8_t BACK = 1u << 0;
  static constexpr uint8_t CONFIRM = 1u << 1;
  static constexpr uint8_t LEFT = 1u << 2;
  static constexpr uint8_t RIGHT = 1u << 3;
  static constexpr uint8_t UP = 1u << 4;
  static constexpr uint8_t DOWN = 1u << 5;

  struct State {
    uint8_t down = 0;
    uint8_t pressed = 0;
    uint8_t released = 0;
    uint8_t physicalPressed = 0;
    uint32_t startedMs = 0;
  };

  State update(uint8_t raw, uint32_t nowMs) {
    State state;
    uint8_t pressed = 0;
    uint8_t released = 0;

    if (raw != candidate_) {
      candidate_ = raw;
      candidateChangedMs_ = nowMs;
    }

    if (candidate_ != stable_ && elapsed(candidateChangedMs_, nowMs) >= DEBOUNCE_MS) {
      const uint8_t old = stable_;
      stable_ = candidate_;
      pressed = stable_ & static_cast<uint8_t>(~old);
      released = old & static_cast<uint8_t>(~stable_);
      state.physicalPressed = pressed;
      state.pressed = pressed & BACK;
      state.released = released & BACK;

      if (pressed & CONFIRM) handleFunctionPress(old, nowMs, state);
      if (released & CONFIRM) handleFunctionRelease(state);

      if (stable_ & CONFIRM) {
        updateDirectionChord(LEFT, UP, leftPressedMs_, pressed, released, nowMs, state);
        updateDirectionChord(RIGHT, DOWN, rightPressedMs_, pressed, released, nowMs, state);
      } else {
        updateDirectionPlain(LEFT, leftPressedMs_, pressed, released, nowMs, state);
        updateDirectionPlain(RIGHT, rightPressedMs_, pressed, released, nowMs, state);
      }
    }

    if ((stable_ & CONFIRM) && confirmPhase_ == ConfirmPhase::Down && !confirmChordDialUsed_ &&
        elapsed(functionPressedMs_, nowMs) >= CONFIRM_HOLD_MS) {
      confirmPhase_ = ConfirmPhase::Hold;
      state.pressed |= CONFIRM;
      state.startedMs = functionPressedMs_;
    }

    state.down = stable_ & BACK;
    state.down |= directionDown_;
    if (confirmPhase_ == ConfirmPhase::Hold) state.down |= CONFIRM;
    return state;
  }

  bool isDebouncePending() const { return candidate_ != stable_; }

  static constexpr uint32_t DEBOUNCE_MS = 5;
  static constexpr uint32_t CONFIRM_HOLD_MS = 300;

 private:
  enum class ConfirmPhase : uint8_t { Idle, Down, Hold };

  static constexpr uint32_t elapsed(uint32_t start, uint32_t now) { return now - start; }

  void updateDirectionPlain(uint8_t physical, uint32_t& pressedMs, uint8_t pressed, uint8_t released, uint32_t nowMs,
                            State& state) {
    if (pressed & physical) pressedMs = nowMs;

    if (!(released & physical)) return;

    state.pressed |= physical;
    state.released |= physical;
    state.startedMs = pressedMs;
  }

  void updateDirectionChord(uint8_t physical, uint8_t logical, uint32_t& pressedMs, uint8_t pressed, uint8_t released,
                            uint32_t nowMs, State& state) {
    const uint8_t preHeld = confirmPreHeldDirections_ & physical;

    if ((pressed & physical) && !preHeld) {
      pressedMs = nowMs;
      confirmChordDialUsed_ = true;
      directionDown_ |= logical;
      state.pressed |= logical;
      state.startedMs = pressedMs;
    }

    if (!(released & physical)) return;

    if (preHeld) {
      confirmPreHeldDirections_ &= static_cast<uint8_t>(~physical);
      state.pressed |= physical;
      state.released |= physical;
      state.startedMs = pressedMs;
      return;
    }

    if (directionDown_ & logical) {
      directionDown_ &= static_cast<uint8_t>(~logical);
      state.released |= logical;
      state.startedMs = pressedMs;
    }
  }

  void handleFunctionPress(uint8_t previousStable, uint32_t nowMs, State& state) {
    (void)state;
    (void)nowMs;
    functionPressedMs_ = candidateChangedMs_;
    confirmChordDialUsed_ = false;
    confirmPreHeldDirections_ = previousStable & static_cast<uint8_t>(LEFT | RIGHT);
    if (confirmPreHeldDirections_ & LEFT) leftPressedMs_ = functionPressedMs_;
    if (confirmPreHeldDirections_ & RIGHT) rightPressedMs_ = functionPressedMs_;
    confirmPhase_ = ConfirmPhase::Down;
  }

  void handleFunctionRelease(State& state) {
    if (confirmChordDialUsed_) {
      confirmPhase_ = ConfirmPhase::Idle;
      confirmPreHeldDirections_ = 0;
      return;
    }

    if (confirmPhase_ == ConfirmPhase::Hold) {
      state.released |= CONFIRM;
      state.startedMs = functionPressedMs_;
      confirmPhase_ = ConfirmPhase::Idle;
      return;
    }

    if (confirmPhase_ == ConfirmPhase::Down) {
      state.pressed |= CONFIRM;
      state.released |= CONFIRM;
      state.startedMs = functionPressedMs_;
      confirmPhase_ = ConfirmPhase::Idle;
    }
  }

  ConfirmPhase confirmPhase_ = ConfirmPhase::Idle;
  uint8_t candidate_ = 0;
  uint8_t stable_ = 0;
  uint8_t directionDown_ = 0;
  uint8_t confirmPreHeldDirections_ = 0;
  bool confirmChordDialUsed_ = false;
  uint32_t candidateChangedMs_ = 0;
  uint32_t functionPressedMs_ = 0;
  uint32_t leftPressedMs_ = 0;
  uint32_t rightPressedMs_ = 0;
};

}  // namespace freeink::input
