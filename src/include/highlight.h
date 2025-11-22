#ifndef FAST_HIGHLIGHT_ENGINE_H
#define FAST_HIGHLIGHT_ENGINE_H

#include <cstdint>
#include <iostream>
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
    /// 是否跨行匹配
    bool is_multi_line {false};
    /// 按捕获组区分的高亮样式
    HashMap<int32_t, String> styles;
    /// Json解析到的跳转state文本
    String goto_state_str;
    /// token包含的正则表达式捕获组数量
    int32_t group_count {0};
    /// token的捕获组在大表达式中的group偏移
    int32_t group_offset {0};
    /// 要跳转的state
    int32_t goto_state {-1};

    const String& getGroupStyle(int32_t group) const;

    static String kDefaultStyle;
    static TokenRule kEmpty;
#ifdef FH_DEBUG
    void dump() const {
      const nlohmann::json json = *this;
      std::cout << json.dump(2) << std::endl;
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TokenRule, pattern, is_multi_line, styles, goto_state_str, group_count, group_offset, goto_state);
#endif
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
    /// 合并后大表达式的总捕获组数量
    int32_t group_count {0};

    static StateRule kEmpty;
#ifdef FH_DEBUG
    void dump() const {
      const nlohmann::json json = *this;
      std::cout << json.dump(2) << std::endl;
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StateRule, name, token_rules, merged_pattern, group_count);
#endif
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
    bool containsRule(int32_t state_id) const;
    StateRule& getStateRule(int32_t state_id);
    SyntaxRule();

    constexpr static int32_t kDefaultStateId = 0;
    constexpr static const char* kDefaultStateName = "default";
#ifdef FH_DEBUG
    void dump() const {
      const nlohmann::json json = *this;
      std::cout << json.dump(2) << std::endl;
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SyntaxRule, name, file_extensions_, variables_map_, state_rules_map_, state_id_map_);
#endif
  private:
    int32_t id_counter_ {1};
  };

  /// 语法规则管理器
  class SyntaxRuleManager {
  public:
    /// 通过json解析语法规则
    /// @param json 语法规则文件的json
    Ptr<SyntaxRule> compileSyntaxFromJson(const String& json);

    /// 解析语法规则
    /// @param file 语法规则定义文件(json)
    Ptr<SyntaxRule> compileSyntaxFromFile(const String& file);

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
    static void parseState(const Ptr<SyntaxRule>& rule, StateRule& state_rule, const nlohmann::json& state_json);
    static void compileStatePattern(StateRule& state_rule);
    static void replaceVariable(String& text, HashMap<String, String>& variables_map);
  };

  /// 匹配的每一个高亮块
  struct TokenSpan {
    /// 高亮块的范围
    TextRange range;
    /// 匹配到的文本
    String matched_text;
    /// 高亮块所匹配的style
    String style;
    /// 高亮块被匹配时所处的状态
    int32_t state {0};
    /// 高亮块要跳转的别的state
    int32_t goto_state {-1};
#ifdef FH_DEBUG
    void dump() const {
      const nlohmann::json json = *this;
      std::cout << json.dump(2) << std::endl;
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TokenSpan, range, style, state, goto_state);
#endif
  };

  /// 每一行的高亮块序列
  struct LineHighlight {
    List<TokenSpan> spans;
#ifdef FH_DEBUG
    void dump() const {
      const nlohmann::json json = *this;
      std::cout << json.dump(2) << std::endl;
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LineHighlight, spans);
#endif
  };

  /// 整个文本内容的高亮
  struct DocumentHighlight {
    List<Ptr<LineHighlight>> lines;

    void addLine(const Ptr<LineHighlight>& line);
    void reset();

#ifdef FH_DEBUG
    void dump() const {
      nlohmann::json json = nlohmann::json::array();
      for (const Ptr<LineHighlight>& line : lines) {
        nlohmann::json line_obj;
        to_json(line_obj, *line);
        json.push_back(line_obj);
      }
      std::cout << json.dump(2) << std::endl;
    }
#endif
  };

  /// 正则匹配结果
  struct MatchResult {
    /// 是否匹配到了
    bool matched {false};
    /// 匹配到的起始字符位置
    size_t start {0};
    /// 匹配到的字符长度
    size_t length {0};
    /// 当前所处的state
    int32_t state {-1};
    /// 匹配到的所属token rule
    int32_t token_rule_idx {-1};
    /// 对应的TokenRule是否为跨行匹配
    bool is_potential_multi_line {false};
    /// 匹配到的捕获组
    int32_t matched_group {-1};
    /// 高亮样式
    String style;
    /// 要切换的state
    int32_t goto_state {-1};
    /// 匹配到的文本内容
    String matched_text;
  };

  /// 跨行匹配时的上下文
  struct MultiLineContext {
    int32_t state {-1};
    String style;
    size_t start_line {0};
    size_t start_column {0};
    String accumulated_text;
  };
  struct MultiLineStartResult {
    bool started {false};
    int32_t new_state {-1};
  };
  struct MultiLineContinueResult {
    bool completed {false};
    TokenSpan span {};
    int32_t new_state {-1};
  };

  /// 高亮分析器
  class DocumentAnalyzer {
  public:
    explicit DocumentAnalyzer(const Ptr<Document>& document, const Ptr<SyntaxRule>& rule);

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
    Ptr<SyntaxRule> rule_;
    HashMap<int32_t, MultiLineContext> multi_line_contexts_;
    List<int32_t> line_states_;

    Ptr<LineHighlight> analyzeLineWithState(size_t line, int32_t start_state);
    MultiLineStartResult startMultiLineMatch(size_t line, size_t char_pos,
      int32_t current_state, const MatchResult& match_result);
    MultiLineContinueResult continueMultiLineMatch(size_t line, size_t char_pos, MultiLineContext& context);
    bool isPotentialMultiLineMatch(const MatchResult& match_result, const String& line_text, size_t current_pos);
    void processSingleLineMatch(Ptr<LineHighlight> highlight, size_t line_num,
      size_t char_pos, int32_t state, const MatchResult& match_result);
    MatchResult matchAtPosition(const String& text, size_t start_char_pos, int32_t state);
    void findMatchedRuleAndGroup(const StateRule& state_rule, OnigRegion* region,
      size_t match_start_byte, size_t match_end_byte, MatchResult& result);
    size_t computeAffectedLines(const TextRange& range, const String& new_text);
  };

  /// 高亮引擎
  class HighlightEngine {
  public:
    HighlightEngine();

    /// 编译语法规则
    /// @param json 语法规则的json文本
    void compileSyntaxFromJson(const String& json) const;

    /// 编译语法规则
    /// @param file 语法规则文件
    void compileSyntaxFromFile(const String& file) const;

    /// 加载文本并进行首次分析
    /// @param document 文本内容
    /// @return 整个文本的高亮结果
    Ptr<DocumentAnalyzer> loadDocument(const Ptr<Document>& document);
  private:
    HashMap<String, Ptr<DocumentAnalyzer>> analyzer_map_;
    Ptr<SyntaxRuleManager> syntax_rule_manager_;
  };
}

#endif //FAST_HIGHLIGHT_ENGINE_H