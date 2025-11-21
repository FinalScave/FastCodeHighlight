#include <utf8/utf8.h>
#include "util.h"

namespace NS_FASTHIGHLIGHT {
  // ===================================== Utf8Util ============================================
  size_t Utf8Util::countChars(const String& str) {
    return utf8::distance(str.begin(), str.end());
  }
  
  size_t Utf8Util::charPosToBytePos(const String& str, size_t char_pos) {
    if (char_pos == 0) return 0;

    auto it = str.begin();
    for (size_t i = 0; i < char_pos && it != str.end(); ++i) {
      utf8::next(it, str.end());
    }
    return it - str.begin();
  }
  
  size_t Utf8Util::bytePosToCharPos(const String& str, size_t byte_pos) {
    if (byte_pos == 0) return 0;

    size_t char_count = 0;
    auto it = str.begin();
    while (it != str.end() && (it - str.begin()) < static_cast<ptrdiff_t>(byte_pos)) {
      utf8::next(it, str.end());
      char_count++;
    }
    return char_count;
  }
  
  String Utf8Util::utf8Substr(const String& str, size_t start_char, size_t char_count) {
    auto start_it = str.begin();
    auto end_it = str.begin();

    // 移动到起始字符
    for (size_t i = 0; i < start_char && start_it != str.end(); ++i) {
      utf8::next(start_it, str.end());
    }

    // 移动到结束字符
    end_it = start_it;
    for (size_t i = 0; i < char_count && end_it != str.end(); ++i) {
      utf8::next(end_it, str.end());
    }

    return {start_it, end_it};
  }
  
  bool Utf8Util::isValidUTF8(const String& str) {
    return utf8::is_valid(str.begin(), str.end());
  }
}