#ifndef FAST_HIGHLIGHT_UTIL_H
#define FAST_HIGHLIGHT_UTIL_H

#include "macro.h"

namespace NS_FASTHIGHLIGHT {
  /// UTF8字符串处理工具
  class Utf8Util {
  public:
    /// 计算UTF-8字符串中的字符数
    /// @param str UTF8文本
    static size_t countChars(const String& str);

    /// 将字符位置转换为字节位置
    /// @param str UTF8文本
    /// @param char_pos 字符位置
    static size_t charPosToBytePos(const String& str, size_t char_pos);

    /// 将字节位置转换为字符位置
    /// @param str UTF8文本
    /// @param byte_pos 字节位置
    static size_t bytePosToCharPos(const String& str, size_t byte_pos);

    /// 获取UTF-8子字符串（按字符计数）
    /// @param str UTF8文本
    /// @param start_char 字符起始位置
    /// @param char_count 字符数量
    /// @return 截取的子串
    static String utf8Substr(const String& str, size_t start_char, size_t char_count);

    /// 检查UTF-8字符串是否有效
    /// @param str UTF8文本
    static bool isValidUTF8(const String& str);
  };
}

#endif //FAST_HIGHLIGHT_UTIL_H