/**
 * Demo 09: 构造函数注入控制
 *   - BOOST_DI_INJECT: 选择并标注构造函数
 *   - BOOST_DI_INJECT_TRAITS: 单独声明构造参数类型
 *   - di::inject<>: 无限制参数列表
 *   - di::ctor_traits<>: 为第三方类指定构造方式
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <string>

namespace di = boost::di;

// ============================================================
// 1. BOOST_DI_INJECT: 指定使用哪个构造函数
// ============================================================
class MultiCtorClass {
public:
    MultiCtorClass() : a_(0), d_(0.0) {
        std::cout << "  [默认构造函数]" << std::endl;
    }
    MultiCtorClass(int a) : a_(a), d_(0.0) {
        std::cout << "  [单参数构造函数] a=" << a << std::endl;
    }
    // DI 会选择这个构造函数(通过 BOOST_DI_INJECT 标记)
    BOOST_DI_INJECT(MultiCtorClass, int a, double d)
        : a_(a), d_(d) {
        std::cout << "  [BOOST_DI_INJECT 标记的构造函数] a=" << a
                  << ", d=" << d << std::endl;
    }

    int a() const { return a_; }
    double d() const { return d_; }

private:
    int a_;
    double d_;
};

// ============================================================
// 2. BOOST_DI_INJECT_TRAITS: 声明与定义分离
// ============================================================
class SplitClass {
public:
    // 声明: 告诉 DI 使用 (int, double) 参数列表
    BOOST_DI_INJECT_TRAITS(int, double);

    // 注意: 参数顺序可以与 traits 不同 (但类型必须匹配)
    SplitClass(int a, double d) : a_(a), d_(d) {}

    void print() const {
        std::cout << "  SplitClass: a=" << a_ << ", d=" << d_ << std::endl;
    }

private:
    int a_;
    double d_;
};

// ============================================================
// 3. BOOST_DI_INJECT_TRAITS 处理默认参数
// ============================================================
class WithDefaults {
public:
    // 只让 DI 注入第一个参数, 第二个使用默认值
    BOOST_DI_INJECT_TRAITS(int);

    explicit WithDefaults(int a, double d = 99.9) : a_(a), d_(d) {}

    void print() const {
        std::cout << "  WithDefaults: a=" << a_ << ", d=" << d_ << std::endl;
    }

private:
    int a_;
    double d_;
};

// ============================================================
// 4. di::inject<>: 替代 BOOST_DI_INJECT_TRAITS, 无参数数量限制
// ============================================================
class ManyParams {
public:
    using boost_di_inject__ = di::inject<int, double, int, double>;

    ManyParams(int a, double b, int c, double d)
        : a_(a), b_(b), c_(c), d_(d) {}

    void print() const {
        std::cout << "  ManyParams: a=" << a_ << ", b=" << b_
                  << ", c=" << c_ << ", d=" << d_ << std::endl;
    }

private:
    int a_;
    double b_;
    int c_;
    double d_;
};

// ============================================================
// 5. di::ctor_traits<>: 为第三方类指定构造方式
//    假设 ThirdPartyClass 来自外部库, 无法修改
// ============================================================
class ThirdPartyClass {
public:
    ThirdPartyClass(double d, int a) : a_(a), d_(d) {}
    ThirdPartyClass(int a, double d) : a_(a), d_(d) {}  // 歧义!

    void print() const {
        std::cout << "  ThirdPartyClass: a=" << a_ << ", d=" << d_ << std::endl;
    }

    int a_;
    double d_;
};

// 在 boost::di 命名空间中特化 ctor_traits
namespace boost {
inline namespace ext {
namespace di {
template <>
struct ctor_traits<ThirdPartyClass> {
    BOOST_DI_INJECT_TRAITS(int, double);  // 指定使用 (int, double) 构造函数
};
}  // namespace di
}  // namespace ext
}  // namespace boost

int main() {
    std::cout << "=== Demo 09: 构造函数注入控制 ===" << std::endl;

    const auto injector = di::make_injector(
        di::bind<int>.to(42),
        di::bind<double>.to(3.14)
    );

    // 1) BOOST_DI_INJECT
    std::cout << "\n[1] BOOST_DI_INJECT - 选择特定构造函数:" << std::endl;
    {
        auto obj1 = injector.create<MultiCtorClass>();
        assert(obj1.a() == 42);
        assert(obj1.d() == 3.14);
    }

    // 2) BOOST_DI_INJECT_TRAITS
    std::cout << "\n[2] BOOST_DI_INJECT_TRAITS - 声明与定义分离:" << std::endl;
    {
        auto obj = injector.create<SplitClass>();
        obj.print();
    }

    // 3) 默认参数处理
    std::cout << "\n[3] 默认参数 - DI 只注入部分参数:" << std::endl;
    {
        auto obj = injector.create<WithDefaults>();
        obj.print();  // a=42, d=99.9 (默认值)
    }

    // 4) di::inject<>
    std::cout << "\n[4] di::inject - 无参数数量限制:" << std::endl;
    {
        auto obj = injector.create<ManyParams>();
        obj.print();
    }

    // 5) ctor_traits 为第三方类指定构造方式
    std::cout << "\n[5] di::ctor_traits - 第三方类构造:" << std::endl;
    {
        auto obj = injector.create<ThirdPartyClass>();
        obj.print();
        assert(obj.a_ == 42);
        assert(obj.d_ == 3.14);
    }

    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
