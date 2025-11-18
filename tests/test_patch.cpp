#include <iostream>
#include "catch2/catch_amalgamated.hpp"
#include "foundation.h"

using namespace NS_FASTHIGHLIGHT;

TEST_CASE("Patch Text") {
  Document document("test.txt", "Line 1: Hello\nLine 2: World\nLine 3: End");

  std::cout << "Original text:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 单行更新：替换 "Hello" 为 "Hi"
  document.patch(TextRange(TextPosition(0, 8), TextPosition(0, 13)), "Hi");
  std::cout << "After single line patch:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 跨行更新：替换 "World\nLine 3" 为 "Universe\nNew Line"
  document.patch(TextRange(TextPosition(1, 8), TextPosition(2, 6)), "Universe\nNew Line");
  std::cout << "After multi-line patch:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 插入文本
  document.insert(TextPosition(1, 0), "Inserted ");
  std::cout << "After insertion:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 删除文本
  document.remove(TextRange(TextPosition(0, 0), TextPosition(0, 6)));
  std::cout << "After removal:" << std::endl;
  std::cout << document.getText() << std::endl;
}

TEST_CASE("Patch Benchmark") {
  BENCHMARK("Patch Performance") {
    Document document("test.txt", "Line 1: Hello\nLine 2: World\nLine 3: End");
    document.patch(TextRange(TextPosition(0, 8), TextPosition(0, 9)), "H");
  };
}