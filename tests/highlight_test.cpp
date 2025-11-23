#include <iostream>
#include "catch2/catch_amalgamated.hpp"
#include "highlight.h"
#include "util.h"

using namespace NS_FASTHIGHLIGHT;

static const char* kSyntaxJavaPath = TESTS_DIR"/syntax/java.json";
static const char* kTestJavaPath = TESTS_DIR"/syntax/test.java";
static const char* kViewJavaPath = TESTS_DIR"/syntax/View.java";

TEST_CASE("Highlight test.ava") {
  Ptr<HighlightEngine> engine = MAKE_PTR<HighlightEngine>();
  try {
    engine->compileSyntaxFromFile(kSyntaxJavaPath);
  } catch (SyntaxRuleParseError& error) {
    std::cerr << error.what() << ": " << error.message() << std::endl;
    return;
  }
  String code_txt = FileUtil::readString(kTestJavaPath);
  Ptr<Document> document = MAKE_PTR<Document>("test.java", code_txt);
  Ptr<DocumentAnalyzer> analyzer = engine->loadDocument(document);
  Ptr<DocumentHighlight> highlight = analyzer->analyzeFully();
  highlight->dump();
}

TEST_CASE("Highlight View.java") {
  Ptr<HighlightEngine> engine = MAKE_PTR<HighlightEngine>();
  try {
    engine->compileSyntaxFromFile(kSyntaxJavaPath);
  } catch (SyntaxRuleParseError& error) {
    std::cerr << error.what() << ": " << error.message() << std::endl;
    return;
  }
  String code_txt = FileUtil::readString(kViewJavaPath);
  Ptr<Document> document = MAKE_PTR<Document>("View.java", code_txt);
  Ptr<DocumentAnalyzer> analyzer = engine->loadDocument(document);
  Ptr<DocumentHighlight> highlight = analyzer->analyzeFully();
  highlight->dump();
}

TEST_CASE("Highlight test.Java Benchmark") {
  Ptr<HighlightEngine> engine = MAKE_PTR<HighlightEngine>();
  engine->compileSyntaxFromFile(kSyntaxJavaPath);
  String code_txt = FileUtil::readString(kTestJavaPath);
  Ptr<Document> document = MAKE_PTR<Document>("test.java", code_txt);
  Ptr<DocumentAnalyzer> analyzer = engine->loadDocument(document);
  BENCHMARK("Highlight Short Performance") {
    Ptr<DocumentHighlight> highlight = analyzer->analyzeFully();
  };
}

TEST_CASE("Highlight View.java Benchmark") {
  Ptr<HighlightEngine> engine = MAKE_PTR<HighlightEngine>();
  engine->compileSyntaxFromFile(kSyntaxJavaPath);
  String code_txt = FileUtil::readString(kViewJavaPath);
  Ptr<Document> document = MAKE_PTR<Document>("View.java", code_txt);
  Ptr<DocumentAnalyzer> analyzer = engine->loadDocument(document);
  BENCHMARK("Highlight Long Performance") {
    Ptr<DocumentHighlight> highlight = analyzer->analyzeFully();
  };
}
