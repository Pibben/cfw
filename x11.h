//
// Created by per on 2019-03-20.
//

#ifndef CFW_X11_H
#define CFW_X11_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <atomic>
#include <set>

namespace cfw {

inline void sleep(const unsigned int milliseconds) {
    struct timespec tv {};
    tv.tv_sec = milliseconds / 1000;
    tv.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&tv, nullptr);
}

namespace {
constexpr unsigned int keyCodes[] = {
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

}; //

class X11 : public WindowBase{
private:
    class X11Globals {
    public:
        std::thread mEventThread;
        std::mutex mSetupMutex;
        std::set<X11*> mWins;
        Display* mDisplay{nullptr};
        unsigned int mBitDepth{0};
        std::atomic<bool> mThreadStopSemaphore;
        bool mIsBGR{false};
        bool mShmEnabled{false};
        bool mIsBigEndian{false};

        static X11Globals& ref() {
            static X11Globals x11;
            return x11;
        }

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


    Atom mWindowAtom{};
    Atom mProtocolAtom{};
    ::Window mWindow{};
    XImage* mXImage{};
    uint32_t* mData{};
    std::unique_ptr<XShmSegmentInfo> mShmInfo{};

    void handleEvents(const XEvent* const pevent) {
        Display* const dpy = X11Globals::ref().mDisplay;
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
                unsigned char chars[32] = {};
                KeySym ksym;
                XLookupString(&event.xkey, (char*)chars, 32, &ksym, nullptr);

                setKey(static_cast<unsigned int>(ksym), true);

                // Convert to utf8
                char utf8[3] = {};
                if (chars[0] < 128) {
                    utf8[0] = chars[0];
                } else {
                    utf8[0] = 0xc2+(chars[0] > 0xbf);
                    utf8[1] = (chars[0] & 0x3f) + 0x80;
                }

                setChar(utf8);
            } break;
            case KeyRelease: {
                char keys_return[32];
                XQueryKeymap(dpy, keys_return);
                const unsigned int kc = event.xkey.keycode;
                const unsigned int kc1 = kc / 8;
                const unsigned int kc2 = kc % 8;
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
        Display* const dpy = X11Globals::ref().mDisplay;
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
                for (auto win : X11Globals::ref().mWins) {
                    if (!win->mIsHidden && event.xany.window == win->mWindow) {
                        win->handleEvents(&event);
                    }
                }
            }
            if (X11Globals::ref().mThreadStopSemaphore) {
                break;
            }
            sleep(8);
        }
        return nullptr;
    }

    void mapWindow() {
        Display* const dpy = X11Globals::ref().mDisplay;
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
        X11Globals::ref().mShmEnabled = false;
        return 0;
    }

    void destructImpl() {
        Display* const dpy = X11Globals::ref().mDisplay;

        auto iter = std::find_if(X11Globals::ref().mWins.begin(), X11Globals::ref().mWins.end(),
                                 [this](X11* w) { return w == this; });
        assert(iter != X11Globals::ref().mWins.end());
        X11Globals::ref().mWins.erase(iter);

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

        X11Globals::ref().mSetupMutex.lock();

        Display*& dpy = X11Globals::ref().mDisplay;
        if (dpy == nullptr) {
            dpy = XOpenDisplay(nullptr);
            if (dpy == nullptr) {
                std::cerr << "Failed to open X11 display." << std::endl;
                exit(1);
            }

            X11Globals::ref().mBitDepth = DefaultDepth(dpy, DefaultScreen(dpy));  // NOLINT
            if (X11Globals::ref().mBitDepth != 8 && X11Globals::ref().mBitDepth != 16 &&
                X11Globals::ref().mBitDepth != 24 && X11Globals::ref().mBitDepth != 32) {
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
                X11Globals::ref().mIsBGR = true;
            }
            X11Globals::ref().mIsBigEndian = ImageByteOrder(dpy);  // NOLINT
            XFree(vinfo);

            X11Globals::ref().mEventThread = std::thread(eventThread);
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
        mXImage = XShmCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)), X11Globals::ref().mBitDepth,  // NOLINT
                                  ZPixmap, nullptr, mShmInfo.get(), mDataWidth, mDataHeight);
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
                    X11Globals::ref().mShmEnabled = true;
                    XErrorHandler oldXErrorHandler = XSetErrorHandler(shmErrorHandler);
                    XShmAttach(dpy, mShmInfo.get());
                    XSync(dpy, 0);
                    XSetErrorHandler(oldXErrorHandler);
                    if (!X11Globals::ref().mShmEnabled) {
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

        X11Globals::ref().mWins.insert(this);
        if (!mIsHidden) {
            mapWindow();
        } else {
            mWindowPosX = mWindowPosY = std::numeric_limits<int>::min();
        }
        X11Globals::ref().mSetupMutex.unlock();

        assert(X11Globals::ref().mBitDepth == 24);
        std::memset(mData, 0, mDataWidth * mDataHeight * 4);
        paint();
    }

    static bool isBigEndian() {
        const int x = 1;
        return reinterpret_cast<const unsigned char*>(&x)[0] == 0u;
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

public:  /// UNIX
    X11(const unsigned int width, const unsigned int height, const char* const title = nullptr)
      : WindowBase(width, height, title) {
        constructImpl(width, height, title);
    }
    ~X11() {
        destructImpl();
    }
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
        Display* const dpy = X11Globals::ref().mDisplay;
        XUnmapWindow(dpy, mWindow);
        mWindowPosX = mWindowPosY = -1;
        mIsHidden = true;
        dispatchCloseCallback();
    }

    void move(const int posx, const int posy) {
        if (mWindowPosX != posx || mWindowPosY != posy) {
            show();
            Display* const dpy = X11Globals::ref().mDisplay;
            XMoveWindow(dpy, mWindow, posx, posy);
            mWindowPosX = posx;
            mWindowPosY = posy;
        }
        paint();
    }

    void setTitle(const std::string& title) {
        delete[] mWindowTitle;  // NOLINT
        const unsigned int size = title.size() + 1;
        mWindowTitle = new char[size];  // NOLINT
        std::memcpy(mWindowTitle, title.c_str(), size);
        Display* const dpy = X11Globals::ref().mDisplay;
        XStoreName(dpy, mWindow, mWindowTitle);
    }

    void paint() {
        if (mIsHidden || (mXImage == nullptr)) {
            return;
        }
        Display* const dpy = X11Globals::ref().mDisplay;
        XClearArea(dpy, mWindow, 0, 0, 1, 1, 1);
    }

    void render(const unsigned char* data, int width, int height) {
        assert(!X11Globals::ref().mIsBGR);
        auto* ndata = static_cast<unsigned int*>(mData);

        static_assert(sizeof(int) == 4);
        assert(X11Globals::ref().mBitDepth == 24);

        if (X11Globals::ref().mIsBigEndian == isBigEndian()) {
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

};

}; //ns

#endif //CFW_X11_H
