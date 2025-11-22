#include <nlohmann/json.hpp>
#include "highlight.h"
#include "util.h"

namespace NS_FASTHIGHLIGHT {
  // ===================================== SyntaxRuleParseError ============================================
  SyntaxRuleParseError::SyntaxRuleParseError(int err_code): err_code_(err_code) {
  }

  SyntaxRuleParseError::SyntaxRuleParseError(int err_code, const String& message): err_code_(err_code), message_(message) {
  }

  SyntaxRuleParseError::SyntaxRuleParseError(int err_code, const char* message): err_code_(err_code), message_(message) {
  }

  const char* SyntaxRuleParseError::what() const noexcept {
    switch (err_code_) {
    case kErrCodePropertyExpected:
      return "Miss property";
    case kErrCodePropertyInvalid:
      return "Property invalid";
    case kErrCodePatternInvalid:
      return "Pattern invalid";
    case kErrCodeStateInvalid:
      return "State invalid";
    case kErrCodeJsonInvalid:
      return "Json invalid";
    default:
      return "Unknown error";
    }
  }

  const String& SyntaxRuleParseError::message() const noexcept {
    return message_;
  }

  // ===================================== TokenRule ============================================
  const String& TokenRule::getGroupStyle(int32_t group) const {
    auto it = styles.find(group);
    if (it == styles.end()) {
      return kDefaultStyle;
    }
    return it->second;
  }

  String TokenRule::kDefaultStyle;
  TokenRule TokenRule::kEmpty;
  StateRule StateRule::kEmpty;
  // ===================================== SyntaxRule ============================================
  int32_t SyntaxRule::getOrCreateStateId(const String& state_name) {
    auto it = state_id_map_.find(state_name);
    if (it == state_id_map_.end()) {
      int32_t new_state_id_ = id_counter_++;
      state_id_map_.insert_or_assign(state_name, new_state_id_);
      return new_state_id_;
    } else {
      return it->second;
    }
  }

  bool SyntaxRule::containsRule(int32_t state_id) const {
    return state_rules_map_.find(state_id) != state_rules_map_.end();
  }

  StateRule& SyntaxRule::getStateRule(int32_t state_id) {
    return state_rules_map_[state_id];
  }

  SyntaxRule::SyntaxRule() {
    state_id_map_.insert_or_assign(kDefaultStateName, kDefaultStateId);
  }

  // ===================================== SyntaxRuleManager ============================================
  Ptr<SyntaxRule> SyntaxRuleManager::compileSyntaxFromJson(const String& json) {
    Ptr<SyntaxRule> syntax_rule = MAKE_PTR<SyntaxRule>();
    nlohmann::json root;
    try {
      root = nlohmann::json::parse(json);
    } catch (nlohmann::json::parse_error& e) {
      throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodeJsonInvalid, e.what());
    }
    parseSyntaxName(syntax_rule, root);
    parseFileExtensions(syntax_rule, root);
    parseVariables(syntax_rule, root);
    parseStates(syntax_rule, root);
    name_rules_map_.insert_or_assign(syntax_rule->name, syntax_rule);
    // 每个state都编译成一个大表达式
    for (std::pair<const int32_t, StateRule>& pair : syntax_rule->state_rules_map_) {
      compileStatePattern(pair.second);
    }
#ifdef FH_DEBUG
    syntax_rule->dump();
#endif
    return syntax_rule;
  }

  Ptr<SyntaxRule> SyntaxRuleManager::compileSyntaxFromFile(const String& file) {
    if (!FileUtil::isFile(file)) {
      return nullptr;
    }
    String content = FileUtil::readString(file);
    if (content.empty()) {
      return nullptr;
    }
    return compileSyntaxFromJson(content);
  }

  Ptr<SyntaxRule> SyntaxRuleManager::getSyntaxRuleByName(const String& extension) {
    const HashMap<String, Ptr<SyntaxRule>>::iterator it = name_rules_map_.find(extension);
    if (it == name_rules_map_.end()) {
      return nullptr;
    }
    return it->second;
  }

  Ptr<SyntaxRule> SyntaxRuleManager::getSyntaxRuleByExtension(const String& extension) const {
    if (extension.empty()) {
      return nullptr;
    }
    String fixed_extension = extension;
    if (fixed_extension[0] != '.') {
      fixed_extension.insert(0, ".");
    }
    for (const std::pair<const String, Ptr<SyntaxRule>>& pair : name_rules_map_) {
      if (pair.second->file_extensions_.find(fixed_extension) != pair.second->file_extensions_.end()) {
        return pair.second;
      }
    }
    return nullptr;
  }

  void SyntaxRuleManager::parseSyntaxName(const Ptr<SyntaxRule>& rule, nlohmann::json& root) {
    if (!root.contains("name")) {
      throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyExpected, "name");
    }
    nlohmann::json name_json = root["name"];
    if (name_json.is_string()) {
      rule->name = name_json;
    } else {
      throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "name");
    }
  }

  void SyntaxRuleManager::parseFileExtensions(const Ptr<SyntaxRule>& rule, nlohmann::json& root) {
    if (root.contains("fileExtensions")) {
      nlohmann::json extensions_json = root["fileExtensions"];
      if (!extensions_json.is_array()) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "fileExtensions");
      }
      for (const nlohmann::json& element_json : extensions_json) {
        if (element_json.is_string()) {
          rule->file_extensions_.emplace(element_json);
        } else {
          throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "fileExtensions");
        }
      }
    } else {
      if (root.contains("fileExtension")) {
        nlohmann::json extensions_json = root["fileExtension"];
        if (extensions_json.is_string()) {
          rule->file_extensions_.emplace(extensions_json);
        } else {
          throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "fileExtension");
        }
      } else {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyExpected, "fileExtensions or fileExtension");
      }
    }
  }

  void SyntaxRuleManager::parseVariables(const Ptr<SyntaxRule>& rule, nlohmann::json& root) {
    if (!root.contains("variables")) {
      return;
    }
    nlohmann::json variables_json = root["variables"];
    if (!variables_json.is_object()) {
      throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "variables");
    }
    for (const auto& item : variables_json.items()) {
      const String& key = item.key();
      const nlohmann::json& variable_json = item.value();
      if (!variable_json.is_string()) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, key);
      }
      rule->variables_map_.insert_or_assign(key, variable_json);
    }
    // 变量有可能引用别的变量，所以全部遍历完后替换变量引用
    for (std::pair<const String, String>& pair : rule->variables_map_) {
      replaceVariable(pair.second, rule->variables_map_);
    }
  }

  void SyntaxRuleManager::parseStates(const Ptr<SyntaxRule>& rule, nlohmann::json& root) {
    if (!root.contains("states")) {
      throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyExpected, "states");
    }
    nlohmann::json states_json = root["states"];
    if (!states_json.is_object()) {
      throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "states");
    }
    for (const auto& item : states_json.items()) {
      const String& key = item.key();
      const nlohmann::json& state_json = item.value();
      if (!state_json.is_array()) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, key);
      }
      StateRule state_rule;
      state_rule.name = key;
      parseState(rule, state_rule, state_json);
      int32_t state_id = rule->getOrCreateStateId(state_rule.name);
      rule->state_rules_map_.insert_or_assign(state_id, std::move(state_rule));
    }
    // states解析完之后将每个token要跳转的state替换成state id
    for (std::pair<const int32_t, StateRule>& pair : rule->state_rules_map_) {
      for (TokenRule& token_rule : pair.second.token_rules) {
        if (token_rule.goto_state_str.empty()) {
          continue;
        }
        token_rule.goto_state = rule->getOrCreateStateId(token_rule.goto_state_str);
        if (!rule->containsRule(token_rule.goto_state)) {
          throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid,
            "state: " + token_rule.goto_state_str);
        }
      }
    }
  }

  void SyntaxRuleManager::parseState(const Ptr<SyntaxRule>& rule, StateRule& state_rule, const nlohmann::json& state_json) {
    for (const nlohmann::json& token_json : state_json) {
      if (!token_json.is_object()) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "state element");
      }
      if (!token_json.contains("pattern")) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "pattern");
      }
      TokenRule token_rule;
      token_rule.pattern = token_json["pattern"];
      // pattern有可能引用变量，进行变量替换
      replaceVariable(token_rule.pattern, rule->variables_map_);
      // state
      if (token_json.contains("state")) {
        token_rule.goto_state_str = token_json["state"];
      }
      // style / styles
      if (token_json.contains("style")) {
        token_rule.styles.insert_or_assign(0, token_json["style"]);
      } else if (token_json.contains("styles")) {
        nlohmann::json styles_json = token_json["styles"];
        if (!styles_json.is_array()) {
          throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "styles");
        }
        size_t size = styles_json.size();
        if (size % 2 != 0) {
          throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "styles elements count % 2 != 0");
        }
        for (size_t i = 0; i < size; i += 2) {
          token_rule.styles.insert_or_assign(styles_json[i], styles_json[i + 1]);
        }
      } else {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "style/styles");
      }
      // multiLine
      if (token_json.contains("multiLine")) {
        const nlohmann::json& multi_line_json = token_json["multiLine"];
        if (!multi_line_json.is_boolean()) {
          throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "multiLine");
        }
        token_rule.is_multi_line = multi_line_json;
      } else {
        token_rule.is_multi_line = PatternUtil::isMultiLinePattern(token_rule.pattern);
      }
      state_rule.token_rules.push_back(std::move(token_rule));
    }
  }

  void SyntaxRuleManager::compileStatePattern(StateRule& state_rule) {
    String merged_pattern;
    int32_t total_group_count {0};
    size_t token_size = state_rule.token_rules.size();
    // 将所有token的表达式合成一个大表达式
    for (size_t i = 0; i < token_size; ++i) {
      TokenRule& token_rule = state_rule.token_rules[i];
      // 计算每个token的捕获组数量
      token_rule.group_count = PatternUtil::countCaptureGroups(token_rule.pattern);
      token_rule.group_offset = total_group_count;
      if (i > 0) {
        merged_pattern += "|";
      }
      merged_pattern += "(";
      merged_pattern += token_rule.pattern;
      merged_pattern += ")";
      total_group_count += 1 + token_rule.group_count;
    }
    state_rule.group_count = total_group_count;
    // 编译合并的大表达
    OnigErrorInfo error;
    OnigRegion* region = onig_region_new();
    int status = onig_new(&state_rule.regex,
      (OnigUChar*)merged_pattern.c_str(),
      (OnigUChar*)(merged_pattern.c_str() + merged_pattern.length()),
      ONIG_OPTION_DEFAULT,
      ONIG_ENCODING_UTF8,
      ONIG_SYNTAX_DEFAULT,
      &error);
    if (status != ONIG_NORMAL) {
      onig_region_free(region, 1);
      throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePatternInvalid, merged_pattern);
    }
    onig_region_free(region, 1);
    state_rule.merged_pattern = std::move(merged_pattern);
  }

  void SyntaxRuleManager::replaceVariable(String& text, HashMap<String, String>& variables_map) {
    for (const std::pair<const String, String>& pair : variables_map) {
      text = StrUtil::replaceAll(text, "${" + pair.first + "}", pair.second);
    }
  }

  // ===================================== DocumentHighlight ============================================
  void DocumentHighlight::addLine(const Ptr<LineHighlight>& line) {
    lines.push_back(line);
  }

  void DocumentHighlight::reset() {
    lines.clear();
  }

  // ===================================== DocumentAnalyzer ============================================
  DocumentAnalyzer::DocumentAnalyzer(const Ptr<Document>& document, const Ptr<SyntaxRule>& rule)
    : document_(document), rule_(rule) {
    highlight_ = MAKE_PTR<DocumentHighlight>();
  }

  Ptr<DocumentHighlight> DocumentAnalyzer::analyzeFully() {
    int32_t current_state = SyntaxRule::kDefaultStateId;
    const size_t line_count = document_->getLineCount();
    line_states_.resize(line_count, SyntaxRule::kDefaultStateId);
    highlight_->reset();
    for (size_t line_num = 0; line_num < line_count; ++line_num) {
      Ptr<LineHighlight> line_highlight = analyzeLineWithState(line_num, current_state);
      highlight_->addLine(line_highlight);
      current_state = line_states_[line_num];
    }
    return highlight_;
  }

  Ptr<DocumentHighlight> DocumentAnalyzer::updateHighlight(const TextRange& range, const String& new_text) {
    document_->patch(range, new_text);
    size_t new_line_count = document_->getLineCount();
    if (line_states_.size() != new_line_count) {
      line_states_.resize(new_line_count, SyntaxRule::kDefaultStateId);
    }
    if (highlight_->lines.size() != new_line_count) {
      highlight_->lines.resize(new_line_count);
    }
    // 清理受影响的跨行上下文
    List<int32_t> contexts_to_remove;
    for (const std::pair<const int32_t, MultiLineContext>& context : multi_line_contexts_) {
      if (context.second.start_line >= range.start.line) {
        contexts_to_remove.push_back(context.first);
      }
    }
    for (int32_t state : contexts_to_remove) {
      multi_line_contexts_.erase(state);
    }

    size_t start_line = range.start.line;
    size_t end_line = computeAffectedLines(range, new_text);
    int32_t current_state = (start_line > 0) ? line_states_[start_line - 1] : SyntaxRule::kDefaultStateId;

    bool state_stabilized = false;
    for (size_t line_num = start_line; line_num < new_line_count && !state_stabilized; ++line_num) {
      auto old_state = line_states_[line_num];
      auto line_highlight = analyzeLineWithState(line_num, current_state);
      highlight_->lines[line_num] = line_highlight;
      current_state = line_states_[line_num];

      if (line_num > end_line && old_state == current_state) {
        state_stabilized = true;
        for (size_t check_line = line_num + 1; check_line < new_line_count; ++check_line) {
          if (line_states_[check_line] != highlight_->lines[check_line]->spans.back().state) {
            state_stabilized = false;
            break;
          }
        }
      }
    }
    return highlight_;
  }

  Ptr<LineHighlight> DocumentAnalyzer::analyzeLine(size_t line) {
    int32_t start_state = (line > 0) ? line_states_[line - 1] : SyntaxRule::kDefaultStateId;
    return analyzeLineWithState(line, start_state);
  }

  Ptr<LineHighlight> DocumentAnalyzer::analyzeLineWithState(size_t line, int32_t start_state) {
    Ptr<LineHighlight> highlight = MAKE_PTR<LineHighlight>();
    const String& line_text = document_->getLine(line);

    if (line_text.empty()) {
      line_states_[line] = start_state;
      return highlight;
    }

    size_t current_char_pos = 0;
    int32_t current_state = start_state;
    size_t line_char_count = Utf8Util::countChars(line_text);

    // 检查跨行上下文
    auto context_it = multi_line_contexts_.find(current_state);
    if (context_it != multi_line_contexts_.end()) {
      MultiLineContext& context = context_it->second;
      MultiLineContinueResult result = continueMultiLineMatch(line, current_char_pos, context);
      // 跨行匹配结束
      if (result.completed) {
        highlight->spans.push_back(result.span);
        current_char_pos = result.span.range.end.column;
        current_state = result.new_state;
        multi_line_contexts_.erase(context_it);
      } else {
        // 整行都属于当前状态下的跨行匹配，继续当前状态和style
        TokenSpan span;
        span.range.start = {line, 0};
        span.range.end = {line, line_char_count};
        span.state = current_state;
        span.style = context.style;
        span.matched_text = line_text;
        highlight->spans.push_back(span);
        line_states_[line] = current_state;
        return highlight;
      }
    }

    // 正常单行匹配
    while (current_char_pos < line_char_count) {
      MatchResult match_result = matchAtPosition(line_text, current_char_pos, current_state);
      if (!match_result.matched) {
        TokenSpan span;
        span.range.start = {line, current_char_pos};
        span.range.end = {line, current_char_pos + 1};
        span.state = current_state;
        span.style = "";
        span.matched_text = Utf8Util::utf8Substr(line_text, current_char_pos, 1);
        highlight->spans.push_back(span);
        current_char_pos++;
        continue;
      }
      // 检查跨行匹配
      if (isPotentialMultiLineMatch(match_result, line_text, current_char_pos)) {
        MultiLineStartResult multi_line_result = startMultiLineMatch(line, current_char_pos, current_state, match_result);
        if (multi_line_result.started) {
          // 开始跨行匹配
          TokenSpan span;
          span.range.start = {line, current_char_pos};
          span.range.end = {line, line_char_count};
          span.state = current_state;
          span.style = match_result.style;
          span.matched_text = Utf8Util::utf8Substr(line_text, current_char_pos, line_char_count - current_char_pos);
          highlight->spans.push_back(span);

          current_char_pos = line_char_count;
          current_state = multi_line_result.new_state;
        } else {
          // 正常处理
          processSingleLineMatch(highlight, line, current_char_pos, current_state, match_result);
          current_char_pos += match_result.length;
          if (match_result.goto_state > 0) {
            current_state = match_result.goto_state;
          }
        }
      } else {
        // 正常单行匹配
        processSingleLineMatch(highlight, line, current_char_pos, current_state, match_result);
        current_char_pos += match_result.length;
        if (match_result.goto_state > 0) {
          current_state = match_result.goto_state;
        }
      }
    }

    line_states_[line] = current_state;
    return highlight;
  }

  MultiLineStartResult DocumentAnalyzer::startMultiLineMatch(size_t line, size_t char_pos, int32_t current_state,
    const MatchResult& match_result) {
    if (match_result.token_rule_idx < 0) {
      return {false, -1};
    }
    if (match_result.goto_state < 0) {
      return {false, -1};
    }
    MultiLineContext context;
    context.state = current_state;
    context.start_line = line;
    context.start_column = char_pos;
    context.accumulated_text = match_result.matched_text;
    multi_line_contexts_[match_result.goto_state] = context;
    return {true, match_result.goto_state};
  }

  MultiLineContinueResult DocumentAnalyzer::continueMultiLineMatch(size_t line, size_t char_pos,
                                                                   MultiLineContext& context) {
    const String& line_text = document_->getLine(line);
    MatchResult match_result = matchAtPosition(line_text, char_pos, context.state);
    if (match_result.matched) {
      MultiLineContinueResult result;
      result.completed = true;
      result.span.range.start = {context.start_line, context.start_column};
      result.span.range.end = {line, char_pos + match_result.length};
      result.span.state = context.state;
      result.span.style = context.style;
      result.span.matched_text = context.accumulated_text + match_result.matched_text;
      result.span.goto_state = match_result.goto_state;
      result.new_state = match_result.goto_state;
      return result;
    } else {
      // 继续累积
      context.accumulated_text += line_text;
      return {false, {}, -1};
    }
  }

  bool DocumentAnalyzer::isPotentialMultiLineMatch(const MatchResult& match_result, const String& line_text,
    size_t current_pos) {
    if (match_result.token_rule_idx < 0) {
      return false;
    }
    if (match_result.is_potential_multi_line) {
      return true;
    }
    size_t line_char_count = Utf8Util::countChars(line_text);
    if (current_pos + match_result.length >= line_char_count) {
      return match_result.goto_state > 0;
    }
    return false;
  }

  void DocumentAnalyzer::processSingleLineMatch(Ptr<LineHighlight> highlight, size_t line_num, size_t char_pos,
    int32_t state, const MatchResult& match_result) {
    TokenSpan span;
    span.range.start = {line_num, char_pos};
    span.range.end = {line_num, char_pos + match_result.length};
    span.state = state;
    span.matched_text = match_result.matched_text;
    span.style = match_result.style;
    span.goto_state = match_result.goto_state;
    highlight->spans.push_back(span);
  }

  MatchResult DocumentAnalyzer::matchAtPosition(const String& text, size_t start_char_pos, int32_t state) {
    MatchResult result;
    if (!rule_->containsRule(state)) {
      return result;
    }
    StateRule& state_rule = rule_->getStateRule(state);
    size_t start_byte_pos = Utf8Util::charPosToBytePos(text, start_char_pos);

    OnigRegion* region = onig_region_new();
    const OnigUChar* start = (const OnigUChar*)(text.c_str() + start_byte_pos);
    const OnigUChar* end = (const OnigUChar*)(text.c_str() + text.length());
    const OnigUChar* range_end = end;

    int match_byte_pos = onig_search(state_rule.regex, (OnigUChar*)text.c_str(),
      end, start, range_end, region, ONIG_OPTION_NONE);
    if (match_byte_pos >= 0) {
      size_t match_start_byte = match_byte_pos;
      size_t match_end_byte = region->end[0];
      size_t match_length_bytes = match_end_byte - match_start_byte;

      size_t match_start_char = Utf8Util::bytePosToCharPos(text, match_start_byte);
      size_t match_end_char = Utf8Util::bytePosToCharPos(text, match_end_byte);
      size_t match_length_chars = match_end_char - match_start_char;

      result.matched = true;
      result.start = match_start_char;
      result.length = match_length_chars;
      result.state = state;
      result.matched_text = Utf8Util::utf8Substr(text, match_start_char, match_length_chars);

      findMatchedRuleAndGroup(state_rule, region, match_start_byte, match_end_byte, result);
    }
    onig_region_free(region, 1);
    return result;
  }

  void DocumentAnalyzer::findMatchedRuleAndGroup(const StateRule& state_rule, OnigRegion* region,
    size_t match_start_byte, size_t match_end_byte, MatchResult& result) {
    for (int32_t rule_idx = 0; rule_idx < static_cast<int32_t>(state_rule.token_rules.size()); ++rule_idx) {
      const TokenRule& token_rule = state_rule.token_rules[rule_idx];
      int32_t rule_group_offset = token_rule.group_offset;

      if (region->beg[rule_group_offset] == static_cast<int>(match_start_byte) &&
        region->end[rule_group_offset] == static_cast<int>(match_end_byte)) {
        result.token_rule_idx = rule_idx;
        result.is_potential_multi_line = token_rule.is_multi_line;
        result.goto_state = token_rule.goto_state;
        result.style = token_rule.getGroupStyle(0);
        result.matched_group = rule_group_offset;

        for (int32_t group = rule_group_offset + 1;group < rule_group_offset + token_rule.group_count;++group) {
          if (region->beg[group] == static_cast<int>(match_start_byte) &&
            region->end[group] == static_cast<int>(match_end_byte)) {
            result.matched_group = group;
            result.style = token_rule.getGroupStyle(group);
            break;
          }
        }
        return;
      }
    }
  }

  size_t DocumentAnalyzer::computeAffectedLines(const TextRange& range, const String& new_text) {
    size_t new_line_count = 0;
    for (char c : new_text) {
      if (c == '\n') {
        new_line_count++;
      }
    }
    size_t old_line_count = range.end.line - range.start.line + 1;
    size_t line_change = (new_line_count + 1) - old_line_count;
    return std::max(range.start.line + new_line_count, range.end.line + line_change);
  }

  HighlightEngine::HighlightEngine() {
    syntax_rule_manager_ = MAKE_PTR<SyntaxRuleManager>();
  }

  // ===================================== HighlightEngine ============================================
  void HighlightEngine::compileSyntaxFromJson(const String& json) const {
    syntax_rule_manager_->compileSyntaxFromJson(json);
  }

  void HighlightEngine::compileSyntaxFromFile(const String& file) const {
    syntax_rule_manager_->compileSyntaxFromFile(file);
  }

  Ptr<DocumentAnalyzer> HighlightEngine::loadDocument(const Ptr<Document>& document) {
    auto it = analyzer_map_.find(document->getUri());
    if (it == analyzer_map_.end()) {
      String uri = document->getUri();
      Ptr<SyntaxRule> rule = syntax_rule_manager_->getSyntaxRuleByExtension(FileUtil::getExtension(uri));
      if (rule == nullptr) {
        return nullptr;
      }
      Ptr<DocumentAnalyzer> analyzer = MAKE_PTR<DocumentAnalyzer>(document, rule);
      analyzer_map_.insert_or_assign(uri, analyzer);
      return analyzer;
    } else {
      return it->second;
    }
  }
}
