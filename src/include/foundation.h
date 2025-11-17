#ifndef FASTCODEHIGHLIGHT_FOUNDATION_H
#define FASTCODEHIGHLIGHT_FOUNDATION_H

#include <vector>
#include "macro.h"

namespace NS_FASTHIGHLIGHT {
  /// 文本位置描述
  struct TextPosition {
    /// 文字所处行
    size_t line;
    /// 文字所处列
    size_t column;

    explicit TextPosition(size_t line = 0, size_t column = 0);

    bool operator<(const TextPosition& other) const;
    bool operator==(const TextPosition& other) const;
  };

  /// 文字的范围区间描述
  struct TextRange {
    TextPosition start;
    TextPosition end;
    TextRange(const TextPosition& start, const TextPosition& end);
  };

  /// 支持增量更新的文本
  class Document {
  public:
    explicit Document(const String& initial_text = "");

    /// 设置完整的文本内容，设置后会按行分割
    /// @param text 文本内容
    void setText(const String& text);

    /// 获取完整文本
    String getText() const;

    /// 获取指定行的文本
    const String& getLine(size_t line) const;

    /// 获取总行数
    size_t getLineCount() const;

    /// 根据指定的行列范围进行增量更新
    /// @param range 更新的范围区间
    /// @param new_text 更新后的文本
    void patch(const TextRange& range, const String& new_text);

    /// 在指定位置插入文本
    /// @param position 要插入的位置
    /// @param text 要插入的文本
    void insert(const TextPosition& position, const String& text);

    /// 删除指定范围的文本
    /// @param range 范围
    void remove(const TextRange& range);
  private:
    std::vector<String> lines;
    bool isValidPosition(const TextPosition& pos) const;
    size_t positionToIndex(const TextPosition& pos) const;
    TextPosition indexToPosition(size_t index) const;
    void patchMultiLine(const TextRange& range, const String& new_text);
  };
}

#endif //FASTCODEHIGHLIGHT_FOUNDATION_H
