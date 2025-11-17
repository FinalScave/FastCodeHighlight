#ifndef FAST_HIGHLIGHT_MACRO_H
#define FAST_HIGHLIGHT_MACRO_H

#include <memory>
#include <string>

#define NS_FASTHIGHLIGHT fasthighlight
#define PTR std::shared_ptr
#define UPTR std::unique_ptr
#define WPTR std::weak_ptr
#define MAKE_PTR std::make_shared
#define MAKE_UPTR std::make_unique
#define MAKE_WPTR std::make_weak
using String = std::string;

#endif //FAST_HIGHLIGHT_MACRO_H