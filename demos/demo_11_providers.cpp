/**
 * Demo 11: 自定义 Provider —— 控制对象的创建方式
 *
 * 核心概念:
 *   - di::providers::stack_over_heap (默认): 尽量在栈上创建
 *   - di::providers::heap: 全部在堆上创建
 *   - 自定义 provider: 控制 new 行为(如 placement new, nothrow 等)
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>

namespace di = boost::di;

class MyClass {
public:
    explicit MyClass(int value) : value_(value) {
        std::cout << "  MyClass 构造: value=" << value_ << std::endl;
    }
    int value() const { return value_; }

private:
    int value_;
};

// ============================================================
// 自定义 Provider: noexcept heap allocation
// ============================================================
class nothrow_heap_provider {
public:
    template <class...>
    struct is_creatable {
        static constexpr auto value = true;
    };

    template <class T, class TInit, class TMemory, class... TArgs>
    auto get(const TInit&, const TMemory&, TArgs&&... args) const noexcept {
        std::cout << "  [nothrow_provider] 创建 " << typeid(T).name()
                  << std::endl;
        return new (std::nothrow) T{std::forward<TArgs>(args)...};
    }
};

// 配置类: 使用自定义 provider
class nothrow_config : public di::config {
public:
    static auto provider(...) noexcept {
        return nothrow_heap_provider{};
    }
};

// ============================================================
// 自定义 Provider: 带计数的分配器
// ============================================================
class counting_provider {
public:
    template <class...>
    struct is_creatable {
        static constexpr auto value = true;
    };

    template <class T, class TInit, class TMemory, class... TArgs>
    auto get(const TInit&, const TMemory&, TArgs&&... args) const noexcept {
        ++allocation_count;
        std::cout << "  [counting_provider] 第 " << allocation_count
                  << " 次分配: " << typeid(T).name() << std::endl;
        return new (std::nothrow) T{std::forward<TArgs>(args)...};
    }

    static int allocation_count;
};
int counting_provider::allocation_count = 0;

class counting_config : public di::config {
public:
    static auto provider(...) noexcept {
        return counting_provider{};
    }
};

// ============================================================
// 使用默认 stack_over_heap provider
// ============================================================
class default_config : public di::config {
public:
    static auto provider(...) noexcept {
        return di::providers::stack_over_heap{};
    }
};

int main() {
    std::cout << "=== Demo 11: 自定义 Provider ===" << std::endl;

    // 1) 默认 provider (stack_over_heap)
    std::cout << "\n[1] 默认 provider:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<int>.to(42)
        );
        auto val = injector.create<int>();      // 栈上
        std::cout << "  栈上创建 int: " << val << std::endl;

        auto ptr = injector.create<std::unique_ptr<MyClass>>();  // 堆上
        std::cout << "  堆上创建 MyClass: " << ptr->value() << std::endl;
    }

    // 2) nothrow provider
    std::cout << "\n[2] Nothrow heap provider:" << std::endl;
    {
        const auto injector = di::make_injector<nothrow_config>(
            di::bind<int>.to(100)
        );
        auto ptr = std::unique_ptr<MyClass>(injector.create<MyClass*>());
        std::cout << "  value = " << ptr->value() << std::endl;
    }

    // 3) 带计数的 provider
    std::cout << "\n[3] Counting provider:" << std::endl;
    {
        counting_provider::allocation_count = 0;
        const auto injector = di::make_injector<counting_config>(
            di::bind<int>.to(200)
        );
        auto p1 = std::unique_ptr<MyClass>(injector.create<MyClass*>());
        auto p2 = std::unique_ptr<MyClass>(injector.create<MyClass*>());
        std::cout << "  总分配次数: " << counting_provider::allocation_count
                  << std::endl;
    }

    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
