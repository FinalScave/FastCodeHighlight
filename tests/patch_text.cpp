#include <iostream>
#include "catch2/catch_amalgamated.hpp"
#include "foundation.h"

using namespace NS_FASTHIGHLIGHT;

const char* text = R"(
行1: 你好
行2: World
行3: 结束)";

TEST_CASE("Patch Text") {
  Document document("test.txt", text);

  std::cout << "原始文本:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 单行更新：替换 "你好" 为 "您不好"
  document.patch(TextRange(TextPosition(1, 4), TextPosition(1, 6)), "您不好");
  std::cout << "单行替换后:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 跨行更新：替换 "World\n行3" 为 "宇宙\n最后一行"
  document.patch(TextRange(TextPosition(2, 4), TextPosition(3, 2)), "宇宙\n最后一行");
  std::cout << "跨行替换后:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 插入文本
  document.insert(TextPosition(2, 1), "=====");
  std::cout << "插入后:" << std::endl;
  std::cout << document.getText() << std::endl << std::endl;

  // 删除文本
  document.remove(TextRange(TextPosition(1, 0), TextPosition(2, 9)));
  std::cout << "删除后:" << std::endl;
  std::cout << document.getText() << std::endl;
}

TEST_CASE("Patch Benchmark") {
  BENCHMARK("Patch Performance") {
    Document document("test.txt", text);
    document.patch(TextRange(TextPosition(1, 0), TextPosition(1, 1)), "H");
  };
}
