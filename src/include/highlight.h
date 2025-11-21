#ifndef FAST_HIGHLIGHT_ENGINE_H
#define FAST_HIGHLIGHT_ENGINE_H

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <oniguruma/oniguruma.h>
#include "foundation.h"

namespace NS_FASTHIGHLIGHT {
  template<typename T>
  using List = std::vector<T>;
  template<typename K, typename V, typename KeyHash = std::hash<K>, typename KeyEqualTo = std::equal_to<K>>
  using HashMap = std::unordered_map<K, V, KeyHash, KeyEqualTo>;
  template<typename T, typename Hash = std::hash<T>, typename EqualTo = std::equal_to<T>>
  using HashSet = std::unordered_set<T, Hash, EqualTo>;

  /// 语法规则Json解析时的错误
  class SyntaxRuleParseError : public std::exception {
  public:
    /// 缺少属性
    static constexpr int kErrCodePropertyExpected = -1;
    /// 属性内容错误
    static constexpr int kErrCodePropertyInvalid = -2;
    /// 正则表达式错误
    static constexpr int kErrCodePatternInvalid = -3;
    /// state错误
    static constexpr int kErrCodeStateInvalid = -4;
    /// json存在语法错误
    static constexpr int kErrCodeJsonInvalid = -5;

    explicit SyntaxRuleParseError(int err_code);
    explicit SyntaxRuleParseError(int err_code, const String& message);
    explicit SyntaxRuleParseError(int err_code, const char* message);

    const char* what() const noexcept override;
    const String& message() const noexcept;
  private:
    int err_code_;
    String message_;
  };

  /// 匹配分析的最小单元(token)的规则
  struct TokenRule {
    /// 正则表达式
    String pattern;
    /// 按捕获组区分的高亮样式
    HashMap<int32_t, String> styles;
    /// Json解析到的跳转state文本
    String goto_state_str;
    /// token的捕获组在大表达式中的group偏移
    int32_t group_offset {0};
    /// 要跳转的state
    int32_t goto_state {-1};
  };

  /// 每个state的规则
  struct StateRule {
    /// state名称
    String name;
    /// 每个token的规则
    List<TokenRule> token_rules;
    /// 每个token的表达式合并的大表达式
    String merged_pattern;
    /// 编译后的正则表达式指针
    OnigRegex regex;
  };

  /// 语法规则
  struct SyntaxRule {
    /// 语法规则的名称
    String name;
    /// 支持的文件扩展名
    HashSet<String> file_extensions_;
    /// variables
    HashMap<String, String> variables_map_;
    /// state id 到 StateRule 的映射
    HashMap<int32_t, StateRule> state_rules_map_;
    /// state名称 到 id 的映射
    HashMap<String, int32_t> state_id_map_;

    int32_t getOrCreateStateId(const String& state_name);
    SyntaxRule();
  private:
    int32_t id_counter_ {1};
    constexpr static int32_t kDefaultStateId = 0;
    constexpr static const char* kDefaultStateName = "default";
  };

  /// 语法规则管理器
  class SyntaxRuleManager {
  public:
    /// 通过json解析语法规则
    /// @param json 语法规则文件的json
    Ptr<SyntaxRule> compileSyntaxRuleFromJson(const String& json);

    /// 获取指定名称的语法规则(如 java)
    /// @param extension 语法规则名称
    Ptr<SyntaxRule> getSyntaxRuleByName(const String& extension);

    /// 获取指定后缀名匹配的的语法规则(如 .t)
    /// @param extension 后缀名
    Ptr<SyntaxRule> getSyntaxRuleByExtension(const String& extension) const;
  private:
    HashMap<String, Ptr<SyntaxRule>> name_rules_map_;

    static void parseSyntaxName(const Ptr<SyntaxRule>& rule, nlohmann::json& root);
    static void parseFileExtensions(const Ptr<SyntaxRule>& rule, nlohmann::json& root);
    static void parseVariables(const Ptr<SyntaxRule>& rule, nlohmann::json& root);
    static void parseStates(const Ptr<SyntaxRule>& rule, nlohmann::json& root);
    static void parseState(StateRule& state_rule, const nlohmann::json& state_json);
  };

  /// 匹配的每一个高亮块
  struct TokenSpan {
    /// 高亮块的范围
    TextRange range;
    /// 高亮块所匹配的style
    String style;
    /// 高亮块被匹配时所处的状态
    int32_t state {0};
    /// 高亮块要跳转的别的state
    int32_t goto_state {-1};
  };

  /// 每一行的高亮块序列
  struct LineHighlight {
    List<TokenSpan> spans;
  };

  /// 整个文本内容的高亮
  struct DocumentHighlight {
    List<LineHighlight> lines;
  };

  /// 高亮分析器
  class DocumentAnalyzer {
  public:
    explicit DocumentAnalyzer(const Ptr<Document>& document);

    /// 对整个文本进行高亮分析
    /// @return 整个文本的高亮结果
    Ptr<DocumentHighlight> analyzeFully();

    /// 根据patch内容重新分析整个文本的高亮结果
    /// @param range patch的变更范围
    /// @param new_text patch的文本
    /// @return 整个文本的高亮结果
    Ptr<DocumentHighlight> updateHighlight(const TextRange& range, const String& new_text);

    /// 分析一行的高亮结果
    /// @param line 行号
    /// @return 一行的高亮结果
    Ptr<LineHighlight> analyzeLine(size_t line);
  private:
    Ptr<Document> document_;
    Ptr<DocumentHighlight> highlight_;
  };

  /// 高亮引擎
  class HighlightEngine {
  public:
    /// 加载文本并进行首次分析
    /// @param document 文本内容
    /// @return 整个文本的高亮结果
    Ptr<DocumentAnalyzer> loadDocument(const Ptr<Document>& document);
  private:
    HashMap<String, Ptr<DocumentAnalyzer>> analyzer_map_;
    SyntaxRuleManager syntax_rule_manager_;
  };
}

#endif //FAST_HIGHLIGHT_ENGINE_H