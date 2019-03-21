//
// Created by per on 2019-03-20.
//

#ifndef CFW_WIN32_H
#define CFW_WIN32_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace {
constexpr unsigned int keyCodes[] = {
    // clang-format off
    VK_ESCAPE, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, VK_PAUSE,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', VK_BACK, VK_INSERT, VK_HOME, VK_PRIOR,
    VK_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', VK_DELETE, VK_END, VK_NEXT,
    VK_CAPITAL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', VK_RETURN,
    VK_SHIFT, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', VK_SHIFT, VK_UP,
    VK_CONTROL, VK_LWIN, VK_LMENU, VK_SPACE, VK_CONTROL, VK_RWIN, VK_APPS, VK_CONTROL, VK_LEFT, VK_DOWN, VK_RIGHT,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, VK_ADD, VK_SUBTRACT, VK_MULTIPLY, VK_DIVIDE
    // clang-format on
};

}; //ns

namespace cfw {

inline void sleep(const unsigned int milliseconds) {
  Sleep(milliseconds);
}


class Win32 : public WindowBase {

    bool mMouseIsTracked{};
    HANDLE mmEventThreadHandle{};
    HANDLE isCreatedSemaphoreHandle{};
    HANDLE mWindowMutexHandle{};
    HWND mWindowHandle{};
    CLIENTCREATESTRUCT mClientCreateStruct{};
    uint32_t* mPixels{};  // TODO: Don't use raw allocation
    BITMAPINFO mBitmapInfo{};
    HDC mDeviceContextHandle{};

    static LRESULT APIENTRY handleEvents(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto* const disp = reinterpret_cast<Win32*>(GetWindowLongPtr(window, GWLP_USERDATA));

        // TODO: Create function in Display class to handle event. Improve
        // encapsulation.
        switch (msg) {
            case WM_CLOSE:
                disp->mMousePosX = disp->mMousePosY = -1;
                disp->mWindowPosX = disp->mWindowPosY = 0;
                disp->setKey(0);
                disp->setKey(0, false);
                disp->mIsHidden = true;
                ReleaseMutex(disp->mWindowMutexHandle);
                ShowWindow(disp->mWindowHandle, SW_HIDE);
                disp->dispatchCloseCallback();
                return 0;
            case WM_MOVE: {
                MSG st_msg;
                while (PeekMessage(&st_msg, window, WM_SIZE, WM_SIZE, PM_REMOVE)) {
                }
                WaitForSingleObject(disp->mWindowMutexHandle, INFINITE);
                const int nx = MAKEPOINTS(lParam).x;  // NOLINT
                const int ny = MAKEPOINTS(lParam).y;  // NOLINT
                if (nx != disp->mWindowPosX || ny != disp->mWindowPosY) {
                    disp->mWindowPosX = nx;
                    disp->mWindowPosY = ny;
                }
                ReleaseMutex(disp->mWindowMutexHandle);
            } break;
            case WM_PAINT:
                disp->paint();
                break;
            case WM_KEYDOWN:
                disp->setKey(static_cast<unsigned int>(wParam));
                break;
            case WM_KEYUP:
                disp->setKey(static_cast<unsigned int>(wParam), false);
                break;
            case WM_MOUSEMOVE: {
                MSG st_msg;
                while (PeekMessage(&st_msg, window, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE)) {
                }
                disp->mMousePosX = MAKEPOINTS(lParam).x;  // NOLINT
                disp->mMousePosY = MAKEPOINTS(lParam).y;  // NOLINT
#if (_WIN32_WINNT >= 0x0400) && !defined(NOTRACKMOUSEEVENT)
                if (!disp->mMouseIsTracked) {
                    TRACKMOUSEEVENT tme;
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = disp->mWindowHandle;
                    if (TrackMouseEvent(&tme) != 0) {
                        disp->mMouseIsTracked = true;
                    }
                }
#endif
                if (disp->mMousePosX < 0 || disp->mMousePosY < 0 || disp->mMousePosX >= static_cast<int>(disp->mDataWidth) ||
                    disp->mMousePosY >= static_cast<int>(disp->mDataHeight)) {
                    disp->mMousePosX = disp->mMousePosY = -1;
                }
                disp->dispatchMouseCallback();
            } break;
            case WM_MOUSELEAVE: {
                disp->mMousePosX = disp->mMousePosY = -1;
                disp->mMouseIsTracked = false;
            } break;
            case WM_LBUTTONDOWN:
                disp->setMouseButtonState(1);
                disp->dispatchMouseCallback();
                break;
            case WM_RBUTTONDOWN:
                disp->setMouseButtonState(2);
                disp->dispatchMouseCallback();
                break;
            case WM_MBUTTONDOWN:
                disp->setMouseButtonState(3);
                disp->dispatchMouseCallback();
                break;
            case WM_LBUTTONUP:
                disp->setMouseButtonState(1, false);
                disp->dispatchMouseCallback();
                break;
            case WM_RBUTTONUP:
                disp->setMouseButtonState(2, false);
                disp->dispatchMouseCallback();
                break;
            case WM_MBUTTONUP:
                disp->setMouseButtonState(3, false);
                disp->dispatchMouseCallback();
                break;
            case WM_MOUSEWHEEL:
                //TODO:
                disp->setMouseWheelState(static_cast<int_fast32_t>(static_cast<uint16_t>HIWORD(wParam)) / 120);  // NOLINT
                disp->dispatchMouseCallback();
            default:
                break;
        }
        return DefWindowProc(window, msg, wParam, lParam);
    }

    static DWORD WINAPI mEventThread(void* arg) {
        auto* const disp = static_cast<Win32*>((static_cast<void**>(arg))[0]);
        const char* const title = static_cast<const char*>((static_cast<void**>(arg))[1]);
        MSG msg;
        delete[] static_cast<void**>(arg);  // NOLINT
        disp->mBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        disp->mBitmapInfo.bmiHeader.biWidth = disp->mDataWidth;
        disp->mBitmapInfo.bmiHeader.biHeight = -static_cast<int>(disp->mDataHeight);
        disp->mBitmapInfo.bmiHeader.biPlanes = 1;
        disp->mBitmapInfo.bmiHeader.biBitCount = 32;
        disp->mBitmapInfo.bmiHeader.biCompression = BI_RGB;
        disp->mBitmapInfo.bmiHeader.biSizeImage = 0;
        disp->mBitmapInfo.bmiHeader.biXPelsPerMeter = 1;
        disp->mBitmapInfo.bmiHeader.biYPelsPerMeter = 1;
        disp->mBitmapInfo.bmiHeader.biClrUsed = 0;
        disp->mBitmapInfo.bmiHeader.biClrImportant = 0;
        disp->mPixels = new uint32_t[static_cast<size_t>(disp->mDataWidth) * disp->mDataHeight];  // NOLINT

        RECT rect;
        rect.left = rect.top = 0;
        rect.right = static_cast<LONG>(disp->mDataWidth) - 1;
        rect.bottom = static_cast<LONG>(disp->mDataHeight) - 1;
        AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0);  // NOLINT
        const int border1 = static_cast<int>((rect.right - rect.left + 1 - disp->mDataWidth) / 2);
        const int border2 = static_cast<int>(rect.bottom - rect.top + 1 - disp->mDataHeight - border1);
        disp->mWindowHandle =
                CreateWindowA("MDICLIENT", title ? title : " ", WS_OVERLAPPEDWINDOW | (disp->mIsHidden ? 0 : WS_VISIBLE),  // NOLINT
                              CW_USEDEFAULT, CW_USEDEFAULT, disp->mDataWidth + 2 * border1,
                              disp->mDataHeight + border1 + border2, nullptr, nullptr, nullptr, &disp->mClientCreateStruct);
        if (!disp->mIsHidden) {
            GetWindowRect(disp->mWindowHandle, &rect);
            disp->mWindowPosX = rect.left + border1;
            disp->mWindowPosY = rect.top + border2;
        } else {
            disp->mWindowPosX = disp->mWindowPosY = 0;
        }

        SetForegroundWindow(disp->mWindowHandle);
        disp->mDeviceContextHandle = GetDC(disp->mWindowHandle);
        disp->mWindowWidth = disp->mDataWidth;
        disp->mWindowHeight = disp->mDataHeight;

        SetWindowLongPtr(disp->mWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(disp));
        SetWindowLongPtr(disp->mWindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(handleEvents));

        SetEvent(disp->isCreatedSemaphoreHandle);
        while (GetMessage(&msg, nullptr, 0, 0)) { DispatchMessage(&msg);
        }
        return 0;
    }

    void updateWindowPosition() {
        if (mIsHidden) {
            mWindowPosX = mWindowPosY = -1;
        } else {
            RECT rect;
            rect.left = rect.top = 0;
            rect.right = static_cast<LONG>(mDataWidth) - 1;
            rect.bottom = static_cast<LONG>(mDataHeight) - 1;
            AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0);  // NOLINT
            const int border1 = static_cast<int>((rect.right - rect.left + 1 - mDataWidth) / 2);
            const int border2 = static_cast<int>(rect.bottom - rect.top + 1 - mDataHeight - border1);
            GetWindowRect(mWindowHandle, &rect);
            mWindowPosX = rect.left + border1;
            mWindowPosY = rect.top + border2;
        }
    }

    void destructImpl() {
        DestroyWindow(mWindowHandle);
        TerminateThread(mmEventThreadHandle, 0);
        delete[] mPixels;
        delete[] mWindowTitle;
        mPixels = nullptr;
        mWindowTitle = nullptr;
        mDataWidth = mDataHeight = mWindowWidth = mWindowHeight = 0;
        mWindowPosX = mWindowPosY = 0;
        mIsHidden = true;
        dispatchCloseCallback();
    }

    void constructImpl(const unsigned int dimw, const unsigned int dimh, const char* const title = nullptr) {
        if ((dimw == 0u) || (dimh == 0u)) {
            destructImpl();
        }

        const char* const nptitle = title != nullptr ? title : "";
        const unsigned int size = static_cast<unsigned int>(std::strlen(nptitle)) + 1;
        char* const tmp = size != 0u ? new char[size] : nullptr;
        if (size != 0u) {
            std::memcpy(tmp, nptitle, size);
        }

        DEVMODE mode;
        mode.dmSize = sizeof(DEVMODE);
        mode.dmDriverExtra = 0;
        EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &mode);

        mDataWidth = std::min(dimw, static_cast<unsigned int>(mode.dmPelsWidth));
        mDataHeight = std::min(dimh, static_cast<unsigned int>(mode.dmPelsHeight));
        mWindowPosX = mWindowPosY = 0;
        mIsHidden = false;
        mMouseIsTracked = false;
        mWindowTitle = tmp;

        void* const arg = reinterpret_cast<void*>(new void*[2]);
        (static_cast<void**>(arg))[0] = reinterpret_cast<void*>(this);
        (static_cast<void**>(arg))[1] = reinterpret_cast<void*>(mWindowTitle);
        mWindowMutexHandle = CreateMutex(nullptr, FALSE, nullptr);
        isCreatedSemaphoreHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        mmEventThreadHandle = CreateThread(nullptr, 0, mEventThread, arg, 0,
                                           nullptr);  // TODO: Use std::thread instead.
        WaitForSingleObject(isCreatedSemaphoreHandle, INFINITE);

        std::memset(mPixels, 0, sizeof(unsigned int) * mDataWidth * mDataHeight);
        paint();
    }

    void setKey(const unsigned int keycode, const bool isPressed = true) {
      if (mKeyboardCallback) {
        for (int i = 0; i < static_cast<int>(Keys::NUM_KEYS); ++i) {
          if (keyCodes[i] == keycode) {
            mKeyboardCallback(static_cast<Keys>(i), isPressed);
          }
        }
      }
    }

public:  // WINDOWS
    Win32(const unsigned int width, const unsigned int height, const char* const title = nullptr)
        : WindowBase(width, height, title) {
        constructImpl(width, height, title);
    }
    ~Win32() {
        destructImpl();
    }

    void show() {
        if (!mIsHidden) {
            return;
        }
        mIsHidden = false;
        ShowWindow(mWindowHandle, SW_SHOW);
        updateWindowPosition();
        paint();
    }

    void hide() {
        if (mIsHidden) {
            return;
        }
        mIsHidden = true;
        ShowWindow(mWindowHandle, SW_HIDE);
        mWindowPosX = mWindowPosY = 0;
        dispatchCloseCallback();
    }

    void move(const int posx, const int posy) {
        if (mWindowPosX != posx || mWindowPosY != posy) {
            RECT rect;
            rect.left = rect.top = 0;
            rect.right = static_cast<LONG>(mWindowWidth) - 1;
            rect.bottom = static_cast<LONG>(mWindowHeight) - 1;
            AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0);  // NOLINT
            const int border1 = static_cast<int>((rect.right - rect.left + 1 - mDataWidth) / 2);
            const int border2 = static_cast<int>(rect.bottom - rect.top + 1 - mDataHeight - border1);
            SetWindowPos(mWindowHandle, nullptr, posx - border1, posy - border2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);  // NOLINT
            mWindowPosX = posx;
            mWindowPosY = posy;
            show();
        }
    }

    void setTitle(const std::string& title) {
        delete[] mWindowTitle;
        const size_t size = title.size() + 1;
        mWindowTitle = new char[size];  // NOLINT
        std::memcpy(mWindowTitle, title.c_str(), size);
        SetWindowTextA(mWindowHandle, mWindowTitle);
    }

    void paint() {
        if (mIsHidden) {
            return;
        }
        WaitForSingleObject(mWindowMutexHandle, INFINITE);
        SetDIBitsToDevice(mDeviceContextHandle, 0, 0, mDataWidth, mDataHeight, 0, 0, 0, mDataHeight, mPixels,
                          &mBitmapInfo, DIB_RGB_COLORS);
        ReleaseMutex(mWindowMutexHandle);
    }

    void render(const uint8_t* data, int width, int height) {
        WaitForSingleObject(mWindowMutexHandle, INFINITE);

        uint32_t* ndata = mPixels;

        for (int xy = width * height; xy > 0; --xy) {
            const uint8_t R = *(data);
            const uint8_t G = *(data + 1);
            const uint8_t B = *(data + 2);
            *(ndata++) = static_cast<uint32_t>(R << 16U) | static_cast<uint32_t>(G << 8U) | B;
            data += 3;
        }

        ReleaseMutex(mWindowMutexHandle);
    }

};

};

#endif //CFW_WIN32_H
