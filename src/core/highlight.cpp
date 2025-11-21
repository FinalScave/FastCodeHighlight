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
    HashMap<String, String> variable_replacements;
    for (const auto& item : variables_json.items()) {
      const String& key = item.key();
      const nlohmann::json& variable_json = item.value();
      if (!variable_json.is_string()) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, key);
      }
      rule->variables_map_.insert_or_assign(key, variable_json);
      variable_replacements.insert_or_assign("${" + key + "}", variable_json);
    }
    // 变量有可能引用别的变量，所以全部遍历完后替换变量引用
    for (std::pair<const String, String>& pair : rule->variables_map_) {
      for (const std::pair<const String, String>& replacement : variable_replacements) {
        StrUtil::replaceAll(pair.second, replacement.first, replacement.second);
      }
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
      parseState(state_rule, state_json);
      int32_t state_id = rule->getOrCreateStateId(state_rule.name);
      rule->state_rules_map_.insert_or_assign(state_id, std::move(state_rule));
    }
  }

  void SyntaxRuleManager::parseState(StateRule& state_rule, const nlohmann::json& state_json) {
    for (const nlohmann::json& token_json : state_json) {
      if (!token_json.is_object()) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "state element");
      }
      if (!token_json.contains("pattern")) {
        throw SyntaxRuleParseError(SyntaxRuleParseError::kErrCodePropertyInvalid, "pattern");
      }
      TokenRule token_rule;
      token_rule.pattern = token_json["pattern"];
      if (token_json.contains("state")) {
        token_rule.goto_state_str = token_json["state"];
      }
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
      state_rule.token_rules.push_back(std::move(token_rule));
    }
  }
}
