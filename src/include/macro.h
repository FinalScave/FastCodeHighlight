#ifndef FAST_HIGHLIGHT_MACRO_H
#define FAST_HIGHLIGHT_MACRO_H

#include <memory>
#include <string>

#define NS_FASTHIGHLIGHT fasthighlight
#define MAKE_PTR std::make_shared
#define MAKE_UPTR std::make_unique
#define MAKE_WPTR std::make_weak

template<typename T>
using Ptr = std::shared_ptr<T>;

template<typename T>
using UPtr = std::unique_ptr<T>;

template<typename T>
using WPtr = std::weak_ptr<T>;

using String = std::string;

#endif //FAST_HIGHLIGHT_MACRO_H