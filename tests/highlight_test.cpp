#include <iostream>
#include "catch2/catch_amalgamated.hpp"
#include "highlight.h"
#include "util.h"

using namespace NS_FASTHIGHLIGHT;

static const char* kSyntaxJavaPath = TESTS_DIR"/syntax/java.json";
static const char* kTestJavaPath = TESTS_DIR"/syntax/test.java";

TEST_CASE("Highlight Fully") {
  Ptr<HighlightEngine> engine = MAKE_PTR<HighlightEngine>();
  engine->compileSyntaxFromFile(kSyntaxJavaPath);
  String code_txt = FileUtil::readString(kTestJavaPath);
  Ptr<Document> document = MAKE_PTR<Document>("test.java", code_txt);
  Ptr<DocumentAnalyzer> analyzer = engine->loadDocument(document);
  Ptr<DocumentHighlight> highlight = analyzer->analyzeFully();
  highlight->dump();
}

TEST_CASE("Highlight Fully Benchmark") {
  Ptr<HighlightEngine> engine = MAKE_PTR<HighlightEngine>();
  engine->compileSyntaxFromFile(kSyntaxJavaPath);
  String code_txt = FileUtil::readString(kTestJavaPath);
  Ptr<Document> document = MAKE_PTR<Document>("test.java", code_txt);
  Ptr<DocumentAnalyzer> analyzer = engine->loadDocument(document);
  BENCHMARK("Highlight Fully Performance") {
    Ptr<DocumentHighlight> highlight = analyzer->analyzeFully();
  };
}
