/**
 * Demo 10: 策略(Policies) —— 编译时/运行时限制和检查
 *
 * 核心概念:
 *   - di::config 提供自定义配置
 *   - di::make_policies(...) 创建策略
 *   - di::policies::constructible 限制哪些类型可以被创建
 *   - 自定义策略: 输出创建的类型信息
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <typeinfo>

namespace di = boost::di;

struct IService {
    virtual ~IService() = default;
    virtual void run() const = 0;
};

class ServiceImpl : public IService {
public:
    explicit ServiceImpl(int value) : value_(value) {}
    void run() const override {
        std::cout << "  ServiceImpl::run() value=" << value_ << std::endl;
    }

private:
    int value_;
};

class App {
public:
    App(std::unique_ptr<IService> svc, int port)
        : svc_(std::move(svc)), port_(port) {}

    void start() const {
        std::cout << "  App starting on port " << port_ << std::endl;
        svc_->run();
    }

private:
    std::unique_ptr<IService> svc_;
    int port_;
};

// ============================================================
// 自定义策略 1: 打印所有被创建的类型 (运行时诊断)
// ============================================================
class print_types_config : public di::config {
public:
    static auto policies(...) noexcept {
        return di::make_policies(
            [](auto type) {
                using T = typename decltype(type)::type;
                std::cout << "  [Policy] 创建类型: " << typeid(T).name()
                          << std::endl;
            }
        );
    }
};

// ============================================================
// 自定义策略 2: 打印详细信息
// ============================================================
class verbose_config : public di::config {
public:
    static auto policies(...) noexcept {
        return di::make_policies(
            [](auto arg) {
                using T = decltype(arg);
                using type = typename T::type;
                using expected = typename T::expected;
                using given = typename T::given;
                using scope = typename T::scope;
                auto ctor_size = T::arity::value;

                std::cout << "  [Verbose] type="
                          << typeid(type).name()
                          << " expected=" << typeid(expected).name()
                          << " given=" << typeid(given).name()
                          << " scope=" << typeid(scope).name()
                          << " ctor_params=" << ctor_size
                          << std::endl;
            }
        );
    }
};

// ============================================================
// 自定义策略 3: 只允许绑定过的类型被创建
// ============================================================
class must_be_bound_config : public di::config {
public:
    static auto policies(...) noexcept {
        using namespace di::policies;
        using namespace di::policies::operators;
        return di::make_policies(
            constructible(is_bound<di::_>{})
        );
    }
};

int main() {
    std::cout << "=== Demo 10: 策略(Policies) ===" << std::endl;

    // 1) 类型打印策略
    std::cout << "\n[1] 类型打印策略:" << std::endl;
    {
        const auto injector = di::make_injector<print_types_config>(
            di::bind<IService>.to<ServiceImpl>(),
            di::bind<int>.to(8080)
        );
        auto app = injector.create<App>();
        app.start();
    }

    // 2) 详细信息策略
    std::cout << "\n[2] 详细信息策略:" << std::endl;
    {
        const auto injector = di::make_injector<verbose_config>(
            di::bind<IService>.to<ServiceImpl>(),
            di::bind<int>.to(3000)
        );
        auto app = injector.create<App>();
        app.start();
    }

    // 3) 所有类型必须显式绑定
    std::cout << "\n[3] 必须绑定策略 (所有依赖必须显式注册):" << std::endl;
    {
        const auto injector = di::make_injector<must_be_bound_config>(
            di::bind<IService>.to<ServiceImpl>(),
            di::bind<int>.to(9090)  // 必须显式绑定, 否则编译错误
        );
        auto app = injector.create<App>();
        app.start();
        std::cout << "  如果去掉 bind<int> 会编译失败!" << std::endl;
    }

    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
