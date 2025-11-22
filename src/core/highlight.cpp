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

  SyntaxRule::SyntaxRule() {
    state_id_map_.insert_or_assign(kDefaultStateName, kDefaultStateId);
  }

  // ===================================== SyntaxRuleManager ============================================
  Ptr<SyntaxRule> SyntaxRuleManager::compileSyntaxRuleFromJson(const String& json) {
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
}
