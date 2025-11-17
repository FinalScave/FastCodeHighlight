#include "foundation.h"

#include <stdexcept>

namespace NS_FASTHIGHLIGHT {
  // ===================================== TextPosition ============================================
  TextPosition::TextPosition(size_t line, size_t column): line(line), column(column) {
  }

  bool TextPosition::operator<(const TextPosition& other) const {
    if (line != other.line) return line < other.line;
    return column < other.column;
  }

  bool TextPosition::operator==(const TextPosition& other) const {
    return line == other.line && column == other.column;
  }

  // ===================================== TextRange ============================================
  TextRange::TextRange(const TextPosition& start, const TextPosition& end): start(start), end(end) {
    if (end < start) throw std::invalid_argument("Invalid range: end before start");
  }

  // ===================================== LineBasedText ============================================
  Document::Document(const String& initial_text) {
    setText(initial_text);
  }

  void Document::setText(const String& text) {
    lines.clear();
    String currentLine;
    for (char c : text) {
      if (c == '\n') {
        lines.push_back(currentLine);
        currentLine.clear();
      } else {
        currentLine += c;
      }
    }
    lines.push_back(currentLine); // 最后一行
  }

  String Document::getText() const {
    String result;
    for (size_t i = 0; i < lines.size(); ++i) {
      result += lines[i];
      if (i < lines.size() - 1) {
        result += '\n';
      }
    }
    return result;
  }

  const String& Document::getLine(size_t line) const {
    if (line >= lines.size()) {
      throw std::out_of_range("Line number out of range");
    }
    return lines[line];
  }

  size_t Document::getLineCount() const {
    return lines.size();
  }
  
  void Document::patch(const TextRange& range, const String& new_text) {
    if (!isValidPosition(range.start) || !isValidPosition(range.end)) {
      throw std::out_of_range("Invalid range position");
    }
    // 单行变更
    if (range.start.line == range.end.line) {
      String& line = lines[range.start.line];
      line.replace(range.start.column,
                  range.end.column - range.start.column,
                  new_text);
    } else {
      // 跨行变更
      patchMultiLine(range, new_text);
    }
  }

  void Document::insert(const TextPosition& position, const String& text) {
    if (!isValidPosition(position)) {
      throw std::out_of_range("Invalid insert position");
    }
    patch(TextRange(position, position), text);
  }

  void Document::remove(const TextRange& range) {
    patch(range, "");
  }

  bool Document::isValidPosition(const TextPosition& pos) const {
    if (pos.line >= lines.size()) {
      return false;
    }
    return pos.column <= lines[pos.line].length();
  }

  size_t Document::positionToIndex(const TextPosition& pos) const {
    if (!isValidPosition(pos)) {
      throw std::out_of_range("Invalid text position");
    }
    size_t index = 0;
    for (size_t i = 0; i < pos.line; ++i) {
      index += lines[i].length() + 1; // +1 for newline
    }
    index += pos.column;
    return index;
  }

  TextPosition Document::indexToPosition(size_t index) const {
    TextPosition pos;
    size_t current = 0;
    for (pos.line = 0; pos.line < lines.size(); ++pos.line) {
      size_t lineLength = lines[pos.line].length();
      if (index <= current + lineLength) {
        pos.column = index - current;
        return pos;
      }
      current += lineLength + 1; // +1 for newline
    }
    throw std::out_of_range("Index out of range");
  }

  void Document::patchMultiLine(const TextRange& range, const String& new_text) {
    // 受变更影响的第一行的前缀
    String first_line_prefix = lines[range.start.line].substr(0, range.start.column);
    // 受变更影响的最后一行的后缀
    String last_line_suffix = lines[range.end.line].substr(range.end.column);

    std::vector<String> new_lines;
    String current_line;

    for (char ch : new_text) {
      if (ch == '\n') {
        new_lines.push_back(current_line);
        current_line.clear();
      } else {
        current_line += ch;
      }
    }
    new_lines.push_back(current_line);

    std::vector<String> updated_lines;

    // range之前的所有行
    for (size_t i = 0; i < range.start.line; ++i) {
      updated_lines.push_back(lines[i]);
    }

    // 变更的第一行与new_text的第一行合并
    if (!new_lines.empty()) {
      updated_lines.push_back(first_line_prefix + new_lines[0]);
      // new_text的中间行
      for (size_t i = 1; i < new_lines.size(); ++i) {
        updated_lines.push_back(new_lines[i]);
      }
    } else {
      updated_lines.push_back(first_line_prefix);
    }

    // 变更的最后一行与new_text的最后一行合并
    if (!updated_lines.empty() && !last_line_suffix.empty()) {
      updated_lines.back() += last_line_suffix;
    } else if (!last_line_suffix.empty()) {
      updated_lines.push_back(last_line_suffix);
    }

    // 剩下没变更的行
    for (size_t i = range.end.line + 1; i < lines.size(); ++i) {
      updated_lines.push_back(lines[i]);
    }

    lines = updated_lines;
  }

}
