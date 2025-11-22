#include <iostream>
#include "catch2/catch_amalgamated.hpp"
#include "highlight.h"
#include "util.h"

using namespace NS_FASTHIGHLIGHT;

TEST_CASE("Parse Rule") {
  Ptr<SyntaxRuleManager> manager = MAKE_PTR<SyntaxRuleManager>();
  try {
    manager->compileSyntaxFromFile(TESTS_DIR"/syntax/java.json");
  } catch (SyntaxRuleParseError& error) {
    std::cerr << error.what() << ": " << error.message() << std::endl;
  }
}

TEST_CASE("Parse Rule Benchmark") {
  Ptr<SyntaxRuleManager> manager = MAKE_PTR<SyntaxRuleManager>();
  String text = FileUtil::readString(TESTS_DIR"/syntax/java.json");
  BENCHMARK("Parse Rule Performance") {
    manager->compileSyntaxFromJson(text);
  };
}
