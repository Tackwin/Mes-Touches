#pragma once
#include "keyboard.hpp"
#include "Mouse.hpp"

extern void render_key_list(const KeyboardState& ks) noexcept;
extern void render_keyboard_heatmap(const KeyboardState& ks) noexcept;
extern void render_keyboard_activity_timeline(const KeyboardState& ms) noexcept;
extern void render_mouse_list(const MouseState& ms) noexcept;
extern void render_mouse_plot(const MouseState& ms) noexcept;
extern void render_display_stat(const MouseState& ms, const Display& d) noexcept;
