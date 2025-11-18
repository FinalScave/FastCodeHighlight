#ifndef FAST_HIGHLIGHT_ENGINE_H
#define FAST_HIGHLIGHT_ENGINE_H

#include <unordered_map>
#include "foundation.h"

namespace NS_FASTHIGHLIGHT {
  template<typename T>
  using List = std::vector<T>;
  template<typename K, typename V, typename KeyHash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
  using HashMap = std::unordered_map<K, V, KeyHash, KeyEqual>;

  /// 匹配的每一个高亮块
  struct Highlight {
    /// 高亮块的范围
    TextRange range;
    /// 高亮块所匹配的style
    String style;
    /// 高亮块被匹配时所处的状态
    String state;
  };

  /// 每一行的高亮块序列
  struct LineHighlight {
    List<Highlight> spans;
  };

  /// 整个文本内容的高亮
  struct DocumentHighlight {
    List<LineHighlight> lines;
  };

  /// 匹配分析的最小单元(token)的规则
  struct TokenRule {
    String pattern;
    String style;
    String state;
  };

  /// 语法规则
  struct SyntaxRule {
    /// 语法规则的名称
    String name;
    /// state 到 token规则的映射
    HashMap<String, List<TokenRule>> state_rules_map_;
  };

  /// 语法规则管理器
  class SyntaxRuleManager {
  public:
    /// 通过json解析语法规则
    /// @param json 语法规则文件的json
    SyntaxRule& loadSyntaxRule(const String& json);

    /// 获取指定名称的语法规则(如 java)
    /// @param extension 语法规则名称
    SyntaxRule& getSyntaxRuleByName(const String& extension);

    /// 获取指定后缀名匹配的的语法规则(如 .t)
    /// @param extension 后缀名
    SyntaxRule& getSyntaxRuleByExtension(const String& extension);
  private:
    HashMap<String, SyntaxRule> name_rules_map_;
    static SyntaxRule kEmptyRule;
  };

  /// 高亮分析
  class HighlightAnalyzer {
  public:
    /// 加载文本并进行首次分析
    /// @param text 文本内容
    /// @return 整个文本的高亮结果
    DocumentHighlight loadText(const String& text);
  private:
    PTR<Document> document_;
  };

  /// 高亮引擎
  class HighlightEngine {
  public:
    /// 加载文本并进行首次分析
    /// @param text 文本内容
    /// @param uri 文本所在文件的uri
    /// @return 整个文本的高亮结果
    DocumentHighlight loadText(const String& text, const String& uri);
  private:
    HashMap<String, DocumentHighlight> highlight_map_;
    HashMap<String, PTR<Document>> document_map_;
    SyntaxRuleManager syntax_rule_manager_;
  };
}

#endif //FAST_HIGHLIGHT_ENGINE_H