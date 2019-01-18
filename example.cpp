#include "cfw.h"
#include <vector>

int main(int  /*argc*/,char ** /*argv*/) {

  std::vector<unsigned char> img1(1000 * 800 * 3);
  std::vector<unsigned char> img2(500 * 800 * 3);

  cfw::Window disp1(1000, 800, "Disp1");
  cfw::Window disp2(500, 800);
  disp2.setTitle("Disp2");

  bool going = true;
  disp1.setKeyCallback([&going, &disp2](const cfw::Keys& key, bool pressed) {
    if (key == cfw::Keys::ESC || key == cfw::Keys::Q) {
      going = false;
    }
    if (key == cfw::Keys::H) {
      disp2.hide();
    }
    if (key == cfw::Keys::S) {
      disp2.show();
    }
  });

  disp1.setMouseCallback([](uint32_t x, uint32_t y, uint32_t keys, int32_t wheel) {
    printf("%d %d 0x%08x, %d\n", x, y, keys, wheel);
  });

  disp1.setCloseCallback([&going]() { going = false;  });

  while (going) {

    for (int i = 0; i < 1000 * 800 * 3; ++i) {
      img1[i] = rand() % 256;
    }

    for (int i = 0; i < 500 * 800 * 3; ++i) {
      img2[i] = rand() % 256;
    }

    disp1.render(img1.data(), 1000, 800);
    disp1.paint();
    disp2.render(img2.data(), 500, 800);
    disp2.paint();

    cfw::sleep(20);
  }

  return 0;
}
