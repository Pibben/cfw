#ifndef CFW_H
#define CFW_H

#include <algorithm>
#include <array>
#include <cassert>
#include <condition_variable>
#include <cstdio>
#include <cstring>  //memcpy()
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#define OS_UNIX 1
#define OS_WINDOWS 2

// TODO: Test cygwin/mingw
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define OS_TYPE OS_UNIX

#elif defined(_MSC_VER) || defined(_WIN32)
#define OS_TYPE OS_WINDOWS
#else
#error Unknown OS
#endif

namespace cfw {

enum class Keys {
    // clang-format off
  ESC, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, PAUSE,
  _1, _2, _3, _4, _5, _6, _7, _8, _9, _0, BACKSPACE, INSERT, HOME, PAGEUP,
  TAB, Q, W, E, R, T, Y, U, I, O, P, _DELETE, END, PAGEDOWN, CAPSLOCK,
  A, S, D, F, G, H, J, K, L, ENTER,
  SHIFTLEFT, Z, X, C, V, B, N, M, SHIFTRIGHT, ARROWUP,
  CTRLLEFT, APPLEFT, ALT, SPACE, ALTGR, APPRIGHT, MENU, CTRLRIGHT, ARROWLEFT, ARROWDOWN, ARROWRIGHT,
  PAD0, PAD1, PAD2, PAD3, PAD4, PAD5, PAD6, PAD7, PAD8, PAD9, PADADD, PADSUB, PADMUL, PADDIV,

  NUM_KEYS
    // clang-format on
};

class WindowBase {
protected:                 // common
    char* mWindowTitle;  // TODO: std::string
    uint_fast16_t mDataWidth;
    uint_fast16_t mDataHeight;
    uint_fast16_t mWindowWidth;
    uint_fast16_t mWindowHeight;
    int_fast16_t mWindowPosX;
    int_fast16_t mWindowPosY;
    int_fast16_t mMousePosX;
    int_fast16_t mMousePosY;
    uint_fast8_t mMouseButtonState;
    int_fast32_t mMouseWheelStatus;

    bool mIsHidden;

    std::function<void(Keys, bool)> mKeyboardCallback;
    std::function<void(const char*)> mCharCallback;
    std::function<void(uint32_t, uint32_t, uint32_t, int32_t)> mMouseCallback;
    std::function<void(void)> mCloseCallback;

public:  // common

    WindowBase(const unsigned int width, const unsigned int height, const char* const title = nullptr)
        : mWindowTitle(nullptr),
          mDataWidth(0),
          mDataHeight(0),
          mWindowWidth(0),
          mWindowHeight(0),
          mWindowPosX(0),
          mWindowPosY(0),
          mMousePosX(-1),
          mMousePosY(-1),
          mMouseButtonState(0),
          mMouseWheelStatus(0),
          mIsHidden(true) {}

    WindowBase(const WindowBase&) = delete;
    WindowBase(WindowBase&&) = delete;
    void operator=(const WindowBase&) = delete;
    void operator=(WindowBase&&) = delete;

    template <class Func>
    void setKeyCallback(Func&& func) {
        mKeyboardCallback = std::forward<Func>(func);
    }

    template <class Func>
    void setCharCallback(Func&& func) {
        mCharCallback = std::forward<Func>(func);
    }

    template <class Func>
    void setMouseCallback(Func&& func) {
        mMouseCallback = std::forward<Func>(func);
    }

    template <class Func>
    void setCloseCallback(Func&& func) {
        mCloseCallback = std::forward<Func>(func);
    }

protected:  // common
    void dispatchMouseCallback() {
        if (mMouseCallback) {
            mMouseCallback(mMousePosX, mMousePosY, mMouseButtonState, mMouseWheelStatus);
        }
    }

    void dispatchCloseCallback() {
        if (mCloseCallback) {
            mCloseCallback();
        }
    }

    void setMouseButtonState(const unsigned int button, const bool isPressed = true) {
        const uint_fast8_t buttoncode = button == 1U ? 1U : button == 2U ? 2U : button == 3U ? 4U : 0U;
        if (isPressed) {
            mMouseButtonState |= buttoncode;  // NOLINT
        } else {
            mMouseButtonState &= ~buttoncode;  // NOLINT
        }
    }

    void setMouseWheelState(const int_fast32_t amplitude) { mMouseWheelStatus += amplitude; }


    void setChar(const char* chars) {
        if (mCharCallback) {
            mCharCallback(chars);
        }
    }
};

}  // namespace cfw

#if OS_TYPE == OS_UNIX

#include "x11.h"
namespace cfw {
    using Window = cfw::X11;
};

#elif OS_TYPE == OS_WINDOWS

#include "win32.h"
using Base = cfw::Win32;

#endif

#endif
