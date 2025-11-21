#include <stdexcept>
#include <sstream>
#include "foundation.h"
#include "util.h"

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

  bool TextRange::contains(const TextPosition& pos) const {
    return !(pos < start) && (pos < end || pos == end);
  }

  // ===================================== Document ============================================
  Document::Document(const String& uri, const String& initial_text): uri_(uri) {
    setText(initial_text);
  }

  Document::Document(String&& uri, const String& initial_text): uri_(std::move(uri)) {
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

  size_t Document::getBytePosition(const TextPosition& pos) const {
    if (pos.line >= lines.size()) {
      return 0;
    }
    const String& line = lines[pos.line];
    return Utf8Util::charPosToBytePos(line, pos.column);
  }

  TextPosition Document::getCharPosition(size_t line_index, size_t byte_pos) const {
    if (line_index >= lines.size()) {
      return TextPosition(line_index, 0);
    }
    const auto& line = lines[line_index];
    size_t char_pos = Utf8Util::bytePosToCharPos(line, byte_pos);
    return TextPosition(line_index, char_pos);
  }

  size_t Document::totalChars() const {
    size_t count = 0;
    for (const auto& line : lines) {
      count += Utf8Util::countChars(line);
    }
    return count;
  }

  size_t Document::getLineCount() const {
    return lines.size();
  }
  
  void Document::patch(const TextRange& range, const String& new_text) {
    if (range.start.line >= lines.size()) {
      // 追加到末尾
      appendText(new_text);
      return;
    }
    // 将patch的文本按行分割
    std::vector<String> new_lines;
    splitTextIntoLines(new_text, new_lines);

    if (range.start.line == range.end.line) {
      // 单行patch
      patchSingleLine(range, new_lines);
    } else {
      // 多行patch
      patchMultipleLines(range, new_lines);
    }
  }

  void Document::appendText(const String& text) {
    std::vector<String> new_lines;
    splitTextIntoLines(text, new_lines);

    if (lines.empty()) {
      lines = std::move(new_lines);
    } else {
      // 第一行追加到现有的最后一行
      lines.back() += new_lines.empty() ? "" : new_lines[0];
      // 后续继续追加
      for (size_t i = 1; i < new_lines.size(); ++i) {
        lines.push_back(new_lines[i]);
      }
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

  size_t Document::positionToCharIndex(const TextPosition& pos) const {
    if (!isValidPosition(pos)) {
      throw std::out_of_range("Invalid text position");
    }
    size_t index = 0;
    for (size_t i = 0; i < pos.line; ++i) {
      index += Utf8Util::countChars(lines[i]) + 1; // +1 for newline
    }
    index += pos.column;
    return index;
  }

  TextPosition Document::charIndexToPosition(size_t char_index) const {
    TextPosition pos;
    size_t current = 0;
    for (pos.line = 0; pos.line < lines.size(); ++pos.line) {
      size_t lineLength = Utf8Util::countChars(lines[pos.line]);
      if (char_index <= current + lineLength) {
        pos.column = char_index - current;
        return pos;
      }
      current += lineLength + 1; // +1 for newline
    }
    throw std::out_of_range("Index out of range");
  }

  void Document::splitTextIntoLines(const String& text, std::vector<String>& result) {
    result.clear();
    if (text.empty()) {
      return;
    }
    std::stringstream ss(text);
    String line;
    while (std::getline(ss, line)) {
      result.push_back(line);
    }
    // 如果最后以换行符结束，添加一个空行
    if (!text.empty() && text.back() == '\n') {
      result.push_back("");
    }
  }

  void Document::patchSingleLine(const TextRange& range, const std::vector<String>& new_lines) {
    String& line = lines[range.start.line];
    // 转换为字节位置进行操作
    size_t start_byte = Utf8Util::charPosToBytePos(line, range.start.column);
    size_t end_byte = Utf8Util::charPosToBytePos(line, range.end.column);

    // 替换为""
    if (new_lines.empty()) {
      line = line.substr(0, start_byte) + line.substr(end_byte);
      return;
    }

    if (new_lines.size() == 1) {
      // 替换为单行
      line = line.substr(0, start_byte) + new_lines[0] + line.substr(end_byte);
    } else {
      // 替换为多行
      String rest_of_line = line.substr(end_byte);
      line = line.substr(0, start_byte) + new_lines[0];

      // 插入新行
      for (size_t i = 1; i < new_lines.size(); ++i) {
        lines.insert(lines.begin() + range.start.line + i, new_lines[i]);
      }

      // 原行剩余部分
      if (!rest_of_line.empty()) {
        lines[range.start.line + new_lines.size() - 1] += rest_of_line;
      }
    }
  }

  void Document::patchMultipleLines(const TextRange& range, const std::vector<String>& new_lines) {
    // 第一行
    String& first_line = lines[range.start.line];
    size_t start_byte = Utf8Util::charPosToBytePos(first_line, range.start.column);

    // 最后一行
    String& last_line = lines[range.end.line];
    size_t end_byte = Utf8Util::charPosToBytePos(last_line, range.end.column);
    String rest_of_last_line = last_line.substr(end_byte);

    // 替换为""
    if (new_lines.empty()) {
      first_line = first_line.substr(0, start_byte) + rest_of_last_line;
      // 删除中间行
      if (range.end.line > range.start.line) {
        size_t start_delete = range.start.line + 1;
        size_t end_delete = range.end.line + 1; // +1, erase 是 [first, last)
        if (end_delete > lines.size()) {
          end_delete = lines.size();
        }
        if (start_delete < end_delete) {
          lines.erase(lines.begin() + start_delete, lines.begin() + end_delete);
        }
      }
      return;
    }

    // 在插入处拼接第一行
    first_line = first_line.substr(0, start_byte) + new_lines[0];

    // 删除中间行
    size_t delete_count = range.end.line - range.start.line;
    if (delete_count > 0) {
      size_t delete_start = range.start.line + 1;
      size_t delete_end = delete_start + delete_count;
      if (delete_end > lines.size()) {
        delete_end = lines.size();
      }
      if (delete_start < delete_end) {
        lines.erase(lines.begin() + delete_start, lines.begin() + delete_end);
      }
    }

    // 插入新行
    for (size_t i = 1; i < new_lines.size(); ++i) {
      lines.insert(lines.begin() + range.start.line + i, new_lines[i]);
    }

    // 最后一行剩余内容
    if (!rest_of_last_line.empty()) {
      size_t last_new_line_index = range.start.line + new_lines.size() - 1;
      if (last_new_line_index < lines.size()) {
        lines[last_new_line_index] += rest_of_last_line;
      } else {
        lines.push_back(rest_of_last_line);
      }
    }
  }

}
