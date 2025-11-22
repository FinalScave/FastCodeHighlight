#include <iostream>
#include "catch2/catch_amalgamated.hpp"
#include "highlight.h"

using namespace NS_FASTHIGHLIGHT;

const char* rule_json = R"(
{
  "name": "java",
  "fileExtensions": [".java"],
  "variables": {
    "identifierStart": "[\\p{Han}\\w_$]+",
    "identifierPart": "[\\p{Han}\\w_$0-9]*",
    "identifier": "${identifierStart}${identifierPart}"
  },
  "states": {
    "default": [
      {
        "pattern": "\\b(class|interface|enum|package|import)\\b",
        "style": "keyword"
      },
      {
        "pattern": "\"(?:[^\"\\\\]|\\\\.)*\"",
        "style": "string"
      },
      {
        "pattern": "(${identifier})\\(",
        "styles": [0, "method", 1, "operator"]
      },
      {
        "pattern": "//.*",
        "style": "comment"
      },
      {
        "pattern": "/\\*",
        "style": "comment",
        "state": "longComment"
      }
    ],
    "longComment": [
      {
        "pattern": "\\s\\S",
        "style": "comment"
      },
      {
        "pattern": "\\*/",
        "style": "comment",
        "state": "default"
      }
    ]
  }
}
)";

TEST_CASE("Parse Rule") {
  Ptr<SyntaxRuleManager> manager = MAKE_PTR<SyntaxRuleManager>();
  try {
    manager->compileSyntaxRuleFromJson(rule_json);
  } catch (SyntaxRuleParseError& error) {
    std::cerr << error.what() << ": " << error.message() << std::endl;
  }
}

TEST_CASE("Parse Rule Benchmark") {
  BENCHMARK("Parse Rule Performance") {
    Ptr<SyntaxRuleManager> manager = MAKE_PTR<SyntaxRuleManager>();
    manager->compileSyntaxRuleFromJson(rule_json);
  };
}
