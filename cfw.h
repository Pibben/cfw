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

#if OS_TYPE == OS_UNIX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <atomic>
#include <set>

#elif OS_TYPE == OS_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
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

inline void sleep(const unsigned int milliseconds) {
#if OS_TYPE == OS_UNIX
    struct timespec tv {};
    tv.tv_sec = milliseconds / 1000;
    tv.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&tv, nullptr);
#elif OS_TYPE == OS_WINDOWS
    Sleep(milliseconds);
#endif
}

class Window {
private:                 // common
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
    std::function<void(uint32_t, uint32_t, uint32_t, int32_t)> mMouseCallback;
    std::function<void(void)> mCloseCallback;

#if OS_TYPE == OS_UNIX
    static constexpr unsigned int keyCodes[] = {
        // clang-format off
    XK_Escape, XK_F1, XK_F2, XK_F3, XK_F4, XK_F5, XK_F6, XK_F7, XK_F8, XK_F9, XK_F10, XK_F11, XK_F12, XK_Pause,
    XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9, XK_0, XK_BackSpace, XK_Insert, XK_Home, XK_Page_Up,
    XK_Tab, XK_q, XK_w, XK_e, XK_r, XK_t, XK_y, XK_u, XK_i, XK_o, XK_p, XK_Delete, XK_End, XK_Page_Down,
    XK_Caps_Lock, XK_a, XK_s, XK_d, XK_f, XK_g, XK_h, XK_j, XK_k, XK_l, XK_Return,
    XK_Shift_L, XK_z, XK_x, XK_c, XK_v, XK_b, XK_n, XK_m, XK_Shift_R, XK_Up,
    XK_Control_L, XK_Super_L, XK_Alt_L, XK_space, XK_Alt_R, XK_Super_R, XK_Menu, XK_Control_R, XK_Left, XK_Down, XK_Right,
    XK_KP_0, XK_KP_1, XK_KP_2, XK_KP_3, XK_KP_4, XK_KP_5, XK_KP_6, XK_KP_7, XK_KP_8, XK_KP_9, XK_KP_Add, XK_KP_Subtract, XK_KP_Multiply, XK_KP_Divide
        // clang-format on
    };

#elif OS_TYPE == OS_WINDOWS
    static constexpr unsigned int keyCodes[] = {
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

#endif

public:  // common
    ~Window() { destructImpl(); }

    Window(const unsigned int width, const unsigned int height, const char* const title = nullptr)
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
          mIsHidden(true) {
        constructImpl(width, height, title);
    }

    Window(const Window&) = delete;
    Window(Window&&) = delete;
    void operator=(const Window&) = delete;
    void operator=(Window&&) = delete;

    template <class Func>
    void setKeyCallback(Func&& func) {
        mKeyboardCallback = std::forward<Func>(func);
    }

    template <class Func>
    void setMouseCallback(Func&& func) {
        mMouseCallback = std::forward<Func>(func);
    }

    template <class Func>
    void setCloseCallback(Func&& func) {
        mCloseCallback = std::forward<Func>(func);
    }

private:  // common
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
            mMouseButtonState |= buttoncode;
        } else {
            mMouseButtonState &= ~buttoncode;
        }
    }

    void setMouseWheelState(const int_fast32_t amplitude) { mMouseWheelStatus += amplitude; }

    void setKey(const unsigned int keycode, const bool isPressed = true) {
        if (mKeyboardCallback) {
            for (int i = 0; i < static_cast<int>(Keys::NUM_KEYS); ++i) {
                if (keyCodes[i] == keycode) {
                    mKeyboardCallback(static_cast<Keys>(i), isPressed);
                }
            }
        }
    }

private:  // UNIX
#if OS_TYPE == OS_UNIX
    class X11Globals {
    public:
        std::thread mEventThread;
        std::mutex mSetupMutex;
        std::set<Window*> mWins;
        Display* mDisplay{nullptr};
        unsigned int mBitDepth{0};
        std::atomic<bool> mThreadStopSemaphore;
        bool mIsBGR{false};
        bool mShmEnabled{false};
        bool mIsBigEndian{false};

        X11Globals() noexcept : mThreadStopSemaphore(false) { XInitThreads(); }

        ~X11Globals() {
            mThreadStopSemaphore = true;
            mEventThread.join();
        }

        X11Globals(const X11Globals&) = delete;
        X11Globals(X11Globals&&) = delete;
        void operator=(const X11Globals&) = delete;
        void operator=(X11Globals&&) = delete;
    };

    static X11Globals x11;

    Atom mWindowAtom{};
    Atom mProtocolAtom{};
    ::Window mWindow{};
    XImage* mXImage{};
    uint32_t* mData{};
    std::unique_ptr<XShmSegmentInfo> mShmInfo{};

    void handleEvents(const XEvent* const pevent) {
        Display* const dpy = x11.mDisplay;
        XEvent event = *pevent;
        switch (event.type) {
            case ClientMessage: {
                if (static_cast<int>(event.xclient.message_type) == static_cast<int>(mProtocolAtom) &&
                    static_cast<int>(event.xclient.data.l[0]) == static_cast<int>(mWindowAtom)) {
                    hide();
                }
            } break;
            case ConfigureNotify: {
                while (XCheckWindowEvent(dpy, mWindow, StructureNotifyMask, &event) != 0) {
                }
                const int nx = event.xconfigure.x, ny = event.xconfigure.y;
                if (nx != mWindowPosX || ny != mWindowPosY) {
                    mWindowPosX = nx;
                    mWindowPosY = ny;
                }
            } break;
            case Expose: {
                while (XCheckWindowEvent(dpy, mWindow, ExposureMask, &event) != 0) {
                }

                // Paint
                if (mIsHidden || (mXImage == nullptr)) {
                    return;
                }

                GC gc = DefaultGC(dpy, DefaultScreen(dpy));  // NOLINT

                XShmPutImage(dpy, mWindow, gc, mXImage, 0, 0, 0, 0, mDataWidth, mDataHeight, 1);
            } break;
            case ButtonPress: {
                bool haveMoreEvents = true;
                do {
                    mMousePosX = event.xmotion.x;
                    mMousePosY = event.xmotion.y;
                    if (mMousePosX < 0 || mMousePosY < 0 || mMousePosX >= static_cast<int>(mDataWidth) ||
                        mMousePosY >= static_cast<int>(mDataHeight)) {
                        mMousePosX = mMousePosY = -1;
                    }
                    switch (event.xbutton.button) {
                        case 1:
                            setMouseButtonState(1);
                            break;
                        case 3:
                            setMouseButtonState(2);
                            break;
                        case 2:
                            setMouseButtonState(3);
                            break;
                    }
                    haveMoreEvents = (XCheckWindowEvent(dpy, mWindow, ButtonPressMask, &event) != 0);
                } while (haveMoreEvents);
                dispatchMouseCallback();
            } break;
            case ButtonRelease: {
                bool haveMoreEvents = true;
                do {
                    mMousePosX = event.xmotion.x;
                    mMousePosY = event.xmotion.y;
                    if (mMousePosX < 0 || mMousePosY < 0 || mMousePosX >= static_cast<int>(mDataWidth) ||
                        mMousePosY >= static_cast<int>(mDataHeight)) {
                        mMousePosX = -1;
                        mMousePosY = -1;
                    }
                    switch (event.xbutton.button) {
                        case 1:
                            setMouseButtonState(1, false);
                            break;
                        case 3:
                            setMouseButtonState(2, false);
                            break;
                        case 2:
                            setMouseButtonState(3, false);
                            break;
                        case 4:
                            setMouseWheelState(1);
                            break;
                        case 5:
                            setMouseWheelState(-1);
                            break;
                    }
                    haveMoreEvents = (XCheckWindowEvent(dpy, mWindow, ButtonReleaseMask, &event) != 0);
                } while (haveMoreEvents);
                dispatchMouseCallback();
            } break;
            case KeyPress: {
                char tmp = 0;
                KeySym ksym;
                XLookupString(&event.xkey, &tmp, 1, &ksym, nullptr);
                setKey(static_cast<unsigned int>(ksym), true);
            } break;
            case KeyRelease: {
                char keys_return[32];
                XQueryKeymap(dpy, keys_return);
                const unsigned int kc = event.xkey.keycode, kc1 = kc / 8, kc2 = kc % 8;
                const bool is_key_pressed = kc1 >= 32 ? false : ((keys_return[kc1] >> kc2) & 1) != 0;
                if (!is_key_pressed) {
                    char tmp = 0;
                    KeySym ksym;
                    XLookupString(&event.xkey, &tmp, 1, &ksym, nullptr);
                    setKey(static_cast<unsigned int>(ksym), false);
                }
            } break;
            case EnterNotify: {
                while (XCheckWindowEvent(dpy, mWindow, EnterWindowMask, &event) != 0) {
                }
                mMousePosX = event.xmotion.x;
                mMousePosY = event.xmotion.y;
                if (mMousePosX < 0 || mMousePosY < 0 || mMousePosX >= static_cast<int>(mDataWidth) ||
                    mMousePosY >= static_cast<int>(mDataHeight)) {
                    mMousePosX = mMousePosY = -1;
                }
                dispatchMouseCallback();
            } break;
            case LeaveNotify: {
                while (XCheckWindowEvent(dpy, mWindow, LeaveWindowMask, &event) != 0) {
                }
                mMousePosX = mMousePosY = -1;
                dispatchMouseCallback();
            } break;
            case MotionNotify: {
                while (XCheckWindowEvent(dpy, mWindow, PointerMotionMask, &event) != 0) {
                }
                mMousePosX = event.xmotion.x;
                mMousePosY = event.xmotion.y;
                if (mMousePosX < 0 || mMousePosY < 0 || mMousePosX >= static_cast<int>(mDataWidth) ||
                    mMousePosY >= static_cast<int>(mDataHeight)) {
                    mMousePosX = mMousePosY = -1;
                }
                dispatchMouseCallback();
            } break;
        }
    }

    static void* eventThread() {
        Display* const dpy = x11.mDisplay;
        XEvent event;

        for (;;) {
            int event_flag = XCheckTypedEvent(dpy, ClientMessage, &event);
            if (event_flag == 0) {
                event_flag = XCheckMaskEvent(dpy,
                                             ExposureMask | StructureNotifyMask | ButtonPressMask | KeyPressMask |
                                                 PointerMotionMask | EnterWindowMask | LeaveWindowMask |
                                                 ButtonReleaseMask | KeyReleaseMask,
                                             &event);
            }
            if (event_flag != 0) {
                for (auto win : x11.mWins) {
                    if (!win->mIsHidden && event.xany.window == win->mWindow) {
                        win->handleEvents(&event);
                    }
                }
            }
            if (x11.mThreadStopSemaphore) {
                break;
            }
            sleep(8);
        }
        return nullptr;
    }

    void mapWindow() {
        Display* const dpy = x11.mDisplay;
        bool is_exposed = false, is_mapped = false;
        XWindowAttributes attr;
        XEvent event;
        XMapRaised(dpy, mWindow);
        do {  // Wait for the window to be mapped.
            XWindowEvent(dpy, mWindow, StructureNotifyMask | ExposureMask, &event);
            switch (event.type) {
                case MapNotify:
                    is_mapped = true;
                    break;
                case Expose:
                    is_exposed = true;
                    break;
            }
        } while (!is_exposed || !is_mapped);
        do {  // Wait for the window to be visible.
            XGetWindowAttributes(dpy, mWindow, &attr);
            if (attr.map_state != IsViewable) {
                XSync(dpy, 0);
                sleep(10);
            }
        } while (attr.map_state != IsViewable);
        mWindowPosX = attr.x;
        mWindowPosY = attr.y;
    }

    static int shmErrorHandler(Display* dpy, XErrorEvent* error) {
        (void)dpy;
        (void)error;
        x11.mShmEnabled = false;
        return 0;
    }

    void destructImpl() {
        Display* const dpy = x11.mDisplay;

        auto iter = std::find_if(x11.mWins.begin(), x11.mWins.end(), [this](Window* w) { return w == this; });
        assert(iter != x11.mWins.end());
        x11.mWins.erase(iter);

        XDestroyWindow(dpy, mWindow);
        mWindow = 0;

        XShmDetach(dpy, mShmInfo.get());
        XDestroyImage(mXImage);
        shmdt(mShmInfo->shmaddr);
        shmctl(mShmInfo->shmid, IPC_RMID, nullptr);
        mShmInfo.reset();

        mData = nullptr;
        mXImage = nullptr;
        XSync(dpy, 0);

        delete[] mWindowTitle;
        mDataWidth = mDataHeight = mWindowWidth = mWindowHeight = 0;
        mWindowPosX = mWindowPosY = 0;
        mIsHidden = true;
        mWindowTitle = nullptr;
        dispatchCloseCallback();
    }

    void constructImpl(const unsigned int dimw, const unsigned int dimh, const char* const title = nullptr) {
        if ((dimw == 0u) || (dimh == 0u)) {
            return destructImpl();
        }

        const char* const nptitle = title != nullptr ? title : "";
        const unsigned int s = static_cast<unsigned int>(std::strlen(nptitle)) + 1;
        char* const tmp_title = s != 0u ? new char[s] : nullptr;
        if (s != 0u) {
            std::memcpy(tmp_title, nptitle, s * sizeof(char));
        }

        x11.mSetupMutex.lock();

        Display*& dpy = x11.mDisplay;
        if (dpy == nullptr) {
            dpy = XOpenDisplay(nullptr);
            if (dpy == nullptr) {
                std::cerr << "Failed to open X11 display." << std::endl;
                exit(1);
            }

            x11.mBitDepth = DefaultDepth(dpy, DefaultScreen(dpy));  // NOLINT
            if (x11.mBitDepth != 8 && x11.mBitDepth != 16 && x11.mBitDepth != 24 && x11.mBitDepth != 32) {
                std::cerr << "Invalid screen mode detected (only 8, 16, 24 and "
                             "32 bits "
                             "modes are managed)."
                          << std::endl;
                exit(1);
            }

            XVisualInfo vtemplate;
            vtemplate.visualid = XVisualIDFromVisual(DefaultVisual(dpy, DefaultScreen(dpy)));  // NOLINT
            int nb_visuals;
            XVisualInfo* vinfo = XGetVisualInfo(dpy, VisualIDMask, &vtemplate, &nb_visuals);  // NOLINT
            if ((vinfo != nullptr) && vinfo->red_mask < vinfo->blue_mask) {
                x11.mIsBGR = true;
            }
            x11.mIsBigEndian = ImageByteOrder(dpy);  // NOLINT
            XFree(vinfo);

            x11.mEventThread = std::thread(eventThread);
        }

        mDataWidth = std::min(dimw, static_cast<unsigned int> DisplayWidth(dpy, DefaultScreen(dpy)));    // NOLINT
        mDataHeight = std::min(dimh, static_cast<unsigned int> DisplayHeight(dpy, DefaultScreen(dpy)));  // NOLINT
        mWindowPosX = mWindowPosY = 0;
        mIsHidden = false;
        mWindowTitle = tmp_title;

        mWindow = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, mDataWidth, mDataHeight, 0, 0L, 0L);  // NOLINT

        XSelectInput(dpy, mWindow,
                     ExposureMask | StructureNotifyMask | ButtonPressMask | KeyPressMask | PointerMotionMask |
                         EnterWindowMask | LeaveWindowMask | ButtonReleaseMask | KeyReleaseMask);

        XStoreName(dpy, mWindow, mWindowTitle != nullptr ? mWindowTitle : " ");

        static const char* const mWindow_class = "Fluffkiosk";
        XClassHint* const window_class = XAllocClassHint();
        window_class->res_name = const_cast<char*>(mWindow_class);   // NOLINT
        window_class->res_class = const_cast<char*>(mWindow_class);  // NOLINT
        XSetClassHint(dpy, mWindow, window_class);
        XFree(window_class);

        mWindowWidth = mDataWidth;
        mWindowHeight = mDataHeight;

        assert(XShmQueryExtension(dpy) != 0);  // NOLINT
        mShmInfo = std::make_unique<XShmSegmentInfo>();
        mXImage =
            XShmCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)), x11.mBitDepth, ZPixmap, nullptr,  // NOLINT
                            mShmInfo.get(), mDataWidth, mDataHeight);
        if (mXImage == nullptr) {
            mShmInfo.reset();
        } else {
            mShmInfo->shmid = shmget(IPC_PRIVATE, mXImage->bytes_per_line * mXImage->height, IPC_CREAT | 0777);
            if (mShmInfo->shmid == -1) {
                XDestroyImage(mXImage);
                mShmInfo.reset();
            } else {
                mData = static_cast<uint32_t*>(shmat(mShmInfo->shmid, nullptr, 0));
                mXImage->data = reinterpret_cast<char*>(mData);
                mShmInfo->shmaddr = reinterpret_cast<char*>(mData);
                if (mShmInfo->shmaddr == reinterpret_cast<char*>(-1)) {
                    shmctl(mShmInfo->shmid, IPC_RMID, nullptr);
                    XDestroyImage(mXImage);
                    mShmInfo.reset();
                } else {
                    mShmInfo->readOnly = 0;
                    x11.mShmEnabled = true;
                    XErrorHandler oldXErrorHandler = XSetErrorHandler(shmErrorHandler);
                    XShmAttach(dpy, mShmInfo.get());
                    XSync(dpy, 0);
                    XSetErrorHandler(oldXErrorHandler);
                    if (!x11.mShmEnabled) {
                        shmdt(mShmInfo->shmaddr);
                        shmctl(mShmInfo->shmid, IPC_RMID, nullptr);
                        XDestroyImage(mXImage);
                        mShmInfo.reset();
                    }
                }
            }
        }
        assert(mShmInfo);

        mWindowAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
        mProtocolAtom = XInternAtom(dpy, "WM_PROTOCOLS", 0);
        XSetWMProtocols(dpy, mWindow, &mWindowAtom, 1);

        x11.mWins.insert(this);
        if (!mIsHidden) {
            mapWindow();
        } else {
            mWindowPosX = mWindowPosY = std::numeric_limits<int>::min();
        }
        x11.mSetupMutex.unlock();

        assert(x11.mBitDepth == 24);
        std::memset(mData, 0, mDataWidth * mDataHeight * 4);
        paint();
    }

    static bool isBigEndian() {
        const int x = 1;
        return reinterpret_cast<const unsigned char*>(&x)[0] == 0u;
    }

public:  /// UNIX
    void show() {
        if (!mIsHidden) {
            return;
        }
        mapWindow();
        mIsHidden = false;
        paint();
    }

    void hide() {
        if (mIsHidden) {
            return;
        }
        Display* const dpy = x11.mDisplay;
        XUnmapWindow(dpy, mWindow);
        mWindowPosX = mWindowPosY = -1;
        mIsHidden = true;
        dispatchCloseCallback();
    }

    void move(const int posx, const int posy) {
        if (mWindowPosX != posx || mWindowPosY != posy) {
            show();
            Display* const dpy = x11.mDisplay;
            XMoveWindow(dpy, mWindow, posx, posy);
            mWindowPosX = posx;
            mWindowPosY = posy;
        }
        paint();
    }

    void setTitle(const std::string& title) {
        delete[] mWindowTitle;
        const unsigned int size = title.size() + 1;
        mWindowTitle = new char[size];
        std::memcpy(mWindowTitle, title.c_str(), size);
        Display* const dpy = x11.mDisplay;
        XStoreName(dpy, mWindow, mWindowTitle);
    }

    void paint() {
        if (mIsHidden || (mXImage == nullptr)) {
            return;
        }
        Display* const dpy = x11.mDisplay;
        XClearArea(dpy, mWindow, 0, 0, 1, 1, 1);
    }

    void render(const unsigned char* data, int width, int height) {
        assert(!x11.mIsBGR);
        auto* ndata = static_cast<unsigned int*>(mData);

        static_assert(sizeof(int) == 4);
        assert(x11.mBitDepth == 24);

        if (x11.mIsBigEndian == isBigEndian()) {
            for (int xy = width * height; xy > 0; --xy) {
                *(ndata++) = (*(data) << 16) | (*(data + 1) << 8) | *(data + 2);
                data += 3;
            }
        } else {
            for (int xy = width * height; xy > 0; --xy) {
                *(ndata++) = (*(data) << 24) | (*(data + 1) << 16) | (*(data + 2) << 8);
                data += 3;
            }
        }
    }

private:  // WINDOWS
#elif OS_TYPE == OS_WINDOWS

    bool mMouseIsTracked;
    HANDLE mmEventThreadHandle;
    HANDLE isCreatedSemaphoreHandle;
    HANDLE mWindowMutexHandle;
    HWND mWindowHandle;
    CLIENTCREATESTRUCT mClientCreateStruct;
    uint32_t* mPixels;  // TODO: Don't use raw allocation
    BITMAPINFO mBitmapInfo;
    HDC mDeviceContextHandle;

    static LRESULT APIENTRY handleEvents(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
#ifdef _WIN64
        Window* const disp = (Window*)GetWindowLongPtr(window, GWLP_USERDATA);
#else
        Window* const disp = (Window*)GetWindowLong(window, GWL_USERDATA);
#endif
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
                const int nx = (int)(short)(LOWORD(lParam)), ny = (int)(short)(HIWORD(lParam));
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
                disp->setKey((unsigned int)wParam);
                break;
            case WM_KEYUP:
                disp->setKey((unsigned int)wParam, false);
                break;
            case WM_MOUSEMOVE: {
                MSG st_msg;
                while (PeekMessage(&st_msg, window, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE)) {
                }
                disp->mMousePosX = LOWORD(lParam);
                disp->mMousePosY = HIWORD(lParam);
#if (_WIN32_WINNT >= 0x0400) && !defined(NOTRACKMOUSEEVENT)
                if (!disp->mMouseIsTracked) {
                    TRACKMOUSEEVENT tme;
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = disp->mWindowHandle;
                    if (TrackMouseEvent(&tme)) {
                        disp->mMouseIsTracked = true;
                    }
                }
#endif
                if (disp->mMousePosX < 0 || disp->mMousePosY < 0 || disp->mMousePosX >= (int)disp->mDataWidth ||
                    disp->mMousePosY >= (int)disp->mDataHeight) {
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
                disp->setMouseWheelState((int_fast32_t)((uint16_t)HIWORD(wParam)) / 120);
                disp->dispatchMouseCallback();
        }
        return DefWindowProc(window, msg, wParam, lParam);
    }

    static DWORD WINAPI mEventThread(void* arg) {
        Window* const disp = (Window*)(((void**)arg)[0]);
        const char* const title = (const char*)(((void**)arg)[1]);
        MSG msg;
        delete[](void**) arg;
        disp->mBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        disp->mBitmapInfo.bmiHeader.biWidth = disp->mDataWidth;
        disp->mBitmapInfo.bmiHeader.biHeight = -(int)disp->mDataHeight;
        disp->mBitmapInfo.bmiHeader.biPlanes = 1;
        disp->mBitmapInfo.bmiHeader.biBitCount = 32;
        disp->mBitmapInfo.bmiHeader.biCompression = BI_RGB;
        disp->mBitmapInfo.bmiHeader.biSizeImage = 0;
        disp->mBitmapInfo.bmiHeader.biXPelsPerMeter = 1;
        disp->mBitmapInfo.bmiHeader.biYPelsPerMeter = 1;
        disp->mBitmapInfo.bmiHeader.biClrUsed = 0;
        disp->mBitmapInfo.bmiHeader.biClrImportant = 0;
        disp->mPixels = new uint32_t[(size_t)disp->mDataWidth * disp->mDataHeight];

        RECT rect;
        rect.left = rect.top = 0;
        rect.right = (LONG)disp->mDataWidth - 1;
        rect.bottom = (LONG)disp->mDataHeight - 1;
        AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, false);
        const int border1 = (int)((rect.right - rect.left + 1 - disp->mDataWidth) / 2);
        const int border2 = (int)(rect.bottom - rect.top + 1 - disp->mDataHeight - border1);
        disp->mWindowHandle =
            CreateWindowA("MDICLIENT", title ? title : " ", WS_OVERLAPPEDWINDOW | (disp->mIsHidden ? 0 : WS_VISIBLE),
                          CW_USEDEFAULT, CW_USEDEFAULT, disp->mDataWidth + 2 * border1,
                          disp->mDataHeight + border1 + border2, 0, 0, 0, &disp->mClientCreateStruct);
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
#ifdef _WIN64
        SetWindowLongPtr(disp->mWindow, GWLP_USERDATA, (LONG_PTR)disp);
        SetWindowLongPtr(disp->mWindow, GWLP_WNDPROC, (LONG_PTR)handleEvents);
#else
        SetWindowLong(disp->mWindowHandle, GWL_USERDATA, (LONG)disp);
        SetWindowLong(disp->mWindowHandle, GWL_WNDPROC, (LONG)handleEvents);
#endif
        SetEvent(disp->isCreatedSemaphoreHandle);
        while (GetMessage(&msg, 0, 0, 0)) DispatchMessage(&msg);
        return 0;
    }

    void updateWindowPosition() {
        if (mIsHidden) {
            mWindowPosX = mWindowPosY = -1;
        } else {
            RECT rect;
            rect.left = rect.top = 0;
            rect.right = (LONG)mDataWidth - 1;
            rect.bottom = (LONG)mDataHeight - 1;
            AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, false);
            const int border1 = (int)((rect.right - rect.left + 1 - mDataWidth) / 2);
            const int border2 = (int)(rect.bottom - rect.top + 1 - mDataHeight - border1);
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
        mPixels = 0;
        mWindowTitle = 0;
        mDataWidth = mDataHeight = mWindowWidth = mWindowHeight = 0;
        mWindowPosX = mWindowPosY = 0;
        mIsHidden = true;
        dispatchCloseCallback();
    }

    void constructImpl(const unsigned int dimw, const unsigned int dimh, const char* const title = 0) {
        if (!dimw || !dimh) {
            destructImpl();
        }

        const char* const nptitle = title ? title : "";
        const unsigned int size = (unsigned int)std::strlen(nptitle) + 1;
        char* const tmp = size ? new char[size] : 0;
        if (size) {
            std::memcpy(tmp, nptitle, size);
        }

        DEVMODE mode;
        mode.dmSize = sizeof(DEVMODE);
        mode.dmDriverExtra = 0;
        EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &mode);

        mDataWidth = std::min(dimw, (unsigned int)mode.dmPelsWidth);
        mDataHeight = std::min(dimh, (unsigned int)mode.dmPelsHeight);
        mWindowPosX = mWindowPosY = 0;
        mIsHidden = false;
        mMouseIsTracked = false;
        mWindowTitle = tmp;

        void* const arg = (void*)(new void*[2]);
        ((void**)arg)[0] = (void*)this;
        ((void**)arg)[1] = (void*)mWindowTitle;
        mWindowMutexHandle = CreateMutex(0, FALSE, 0);
        isCreatedSemaphoreHandle = CreateEvent(0, FALSE, FALSE, 0);
        mmEventThreadHandle = CreateThread(0, 0, mEventThread, arg, 0,
                                           0);  // TODO: Use std::thread instead.
        WaitForSingleObject(isCreatedSemaphoreHandle, INFINITE);

        std::memset(mPixels, 0, sizeof(unsigned int) * mDataWidth * mDataHeight);
        paint();
    }

public:  // WINDOWS
    void show() {
        if (!mIsHidden) return;
        mIsHidden = false;
        ShowWindow(mWindowHandle, SW_SHOW);
        updateWindowPosition();
        paint();
    }

    void hide() {
        if (mIsHidden) return;
        mIsHidden = true;
        ShowWindow(mWindowHandle, SW_HIDE);
        mWindowPosX = mWindowPosY = 0;
        dispatchCloseCallback();
    }

    void move(const int posx, const int posy) {
        if (mWindowPosX != posx || mWindowPosY != posy) {
            RECT rect;
            rect.left = rect.top = 0;
            rect.right = (LONG)mWindowWidth - 1;
            rect.bottom = (LONG)mWindowHeight - 1;
            AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, false);
            const int border1 = (int)((rect.right - rect.left + 1 - mDataWidth) / 2);
            const int border2 = (int)(rect.bottom - rect.top + 1 - mDataHeight - border1);
            SetWindowPos(mWindowHandle, 0, posx - border1, posy - border2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            mWindowPosX = posx;
            mWindowPosY = posy;
            show();
        }
    }

    void setTitle(const std::string& title) {
        delete[] mWindowTitle;
        const unsigned int size = title.size() + 1;
        mWindowTitle = new char[size];
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
            *(ndata++) = (uint32_t)((R << 16) | (G << 8) | B);
            data += 3;
        }

        ReleaseMutex(mWindowMutexHandle);
    }

#endif
};
#if OS_TYPE == OS_UNIX
Window::X11Globals Window::x11;
#endif
}  // namespace cfw

#endif
