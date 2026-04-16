/**
 * Demo 03: 值绑定 —— 绑定常量、lambda、已有对象
 *
 * 核心概念:
 *   - di::bind<T>.to(value)         绑定到具体值
 *   - di::bind<T>.to(shared_ptr)    绑定到已有对象
 *   - di::bind<T>.to(lambda)        绑定到工厂 lambda
 *   - di::bind<>.to(value)          自动推导类型绑定
 *   - di::bind<T>.to(ref)           绑定到引用(外部管理生命周期)
 *   - [di::override]                覆盖之前的绑定
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>

namespace di = boost::di;

struct IService {
    virtual ~IService() = default;
    virtual int value() const = 0;
};

class ServiceImpl : public IService {
public:
    int value() const override { return 999; }
};

class App {
public:
    App(int port, double rate, std::shared_ptr<IService> svc)
        : port_(port), rate_(rate), svc_(svc) {}

    void print() const {
        std::cout << "  App: port=" << port_
                  << ", rate=" << rate_
                  << ", svc.value=" << svc_->value() << std::endl;
    }

private:
    int port_;
    double rate_;
    std::shared_ptr<IService> svc_;
};

int main() {
    std::cout << "=== Demo 03: 值绑定 ===" << std::endl;

    // 1) 基本值绑定
    std::cout << "\n[1] 绑定具体值:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<int>.to(8080),
            di::bind<double>.to(0.5),
            di::bind<IService>.to<ServiceImpl>()
        );
        injector.create<App>().print();
    }

    // 2) 绑定到已有 shared_ptr 对象
    std::cout << "\n[2] 绑定到已有 shared_ptr:" << std::endl;
    {
        auto existing_svc = std::make_shared<ServiceImpl>();
        const auto injector = di::make_injector(
            di::bind<int>.to(3000),
            di::bind<double>.to(1.0),
            di::bind<IService>.to(existing_svc)
        );
        auto app = injector.create<App>();
        app.print();
        // 验证是同一个对象
        auto svc = injector.create<std::shared_ptr<IService>>();
        assert(svc.get() == existing_svc.get());
        std::cout << "  确认: 注入的是同一个 shared_ptr 对象" << std::endl;
    }

    // 3) lambda 工厂绑定
    std::cout << "\n[3] lambda 工厂绑定:" << std::endl;
    {
        int call_count = 0;
        const auto injector = di::make_injector(
            di::bind<int>.to([&call_count] { return ++call_count * 1000; }),
            di::bind<double>.to(2.0),
            di::bind<IService>.to<ServiceImpl>()
        );
        // 每次 create 都会调用 lambda
        auto port1 = injector.create<int>();
        auto port2 = injector.create<int>();
        std::cout << "  第1次: " << port1 << ", 第2次: " << port2 << std::endl;
    }

    // 4) 绑定到引用(外部管理生命周期)
    std::cout << "\n[4] 绑定到外部变量引用:" << std::endl;
    {
        int external_port = 9090;
        const auto injector = di::make_injector(
            di::bind<int>.to(external_port),
            di::bind<double>.to(0.1),
            di::bind<IService>.to<ServiceImpl>()
        );
        // 可以获取引用
        auto& port_ref = injector.create<int&>();
        std::cout << "  port_ref = " << port_ref << std::endl;
        assert(&port_ref == &external_port);
        std::cout << "  确认: 获取的是同一个变量的引用" << std::endl;
    }

    // 5) override: 覆盖之前的绑定
    std::cout << "\n[5] 使用 [di::override] 覆盖绑定:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<int>.to(80),
            di::bind<int>.to(443)[di::override],   // 覆盖上面的 80
            di::bind<double>.to(5.0),
            di::bind<IService>.to<ServiceImpl>()
        );
        auto port = injector.create<int>();
        assert(port == 443);
        std::cout << "  port = " << port << " (被 override 为 443)" << std::endl;
    }

    // 6) 自动推导类型绑定
    std::cout << "\n[6] 自动推导类型 di::bind<>.to(value):" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<>.to(42),       // 自动推导为 int
            di::bind<>.to(3.14),     // 自动推导为 double
            di::bind<IService>.to<ServiceImpl>()
        );
        assert(42 == injector.create<int>());
        assert(3.14 == injector.create<double>());
        std::cout << "  int = " << injector.create<int>()
                  << ", double = " << injector.create<double>() << std::endl;
    }

    std::cout << "\n所有断言通过!" << std::endl;
    return 0;
}
