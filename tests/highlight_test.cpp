#include <iostream>
#include "catch2/catch_amalgamated.hpp"
#include "highlight.h"

using namespace NS_FASTHIGHLIGHT;

const char* java_rule_json = R"(
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
const char* java_code_text = R"(
package com.test;

import java.util.*;

public class Main {
  public static void main() {
    /**
    aaaa
    bbbb
    */
    System.out.println("HelloWorld");
  }
}
)";

TEST_CASE("Highlight Fully") {
  Ptr<HighlightEngine> engine = MAKE_PTR<HighlightEngine>();
  engine->compileSyntaxFromJson(java_rule_json);
  Ptr<Document> document = MAKE_PTR<Document>("test.java", java_code_text);
  Ptr<DocumentAnalyzer> analyzer = engine->loadDocument(document);
  Ptr<DocumentHighlight> highlight = analyzer->analyzeFully();
  highlight->dump();
}

TEST_CASE("Highlight Fully Benchmark") {
  BENCHMARK("Highlight Fully Performance") {

  };
}
