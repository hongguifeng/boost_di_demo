/**
 * Demo 05: 命名注解(Named Annotations) —— 区分同类型的不同依赖
 *
 * 核心概念:
 *   - auto name = []{}; 定义一个命名标识
 *   - BOOST_DI_INJECT(T, (named = name) Type param) 标注构造函数参数
 *   - di::bind<T>.named(name).to(value) 绑定命名参数
 *   - 解决同一类型多个参数的歧义问题
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <string>
#include <memory>

namespace di = boost::di;

// ============================================================
// 1. 定义命名标识 (每个 lambda 对象类型唯一)
// ============================================================
auto Width = [] {};
auto Height = [] {};
auto Title = [] {};
auto ServerPort = [] {};
auto DbPort = [] {};

// ============================================================
// 2. 使用 BOOST_DI_INJECT 标注需要命名的参数
// ============================================================
class Window {
public:
    // BOOST_DI_INJECT 宏用于告诉 DI 框架使用哪个构造函数
    // (named = XXX) 用于区分同类型参数
    BOOST_DI_INJECT(Window,
                    (named = Width) int w,
                    (named = Height) int h,
                    (named = Title) std::string title)
        : width_(w), height_(h), title_(title) {}

    void print() const {
        std::cout << "  Window: " << title_
                  << " (" << width_ << "x" << height_ << ")" << std::endl;
    }

private:
    int width_;
    int height_;
    std::string title_;
};

// ============================================================
// 3. 更复杂的场景: 多个同类型命名参数
// ============================================================
class NetworkConfig {
public:
    BOOST_DI_INJECT(NetworkConfig,
                    (named = ServerPort) int server_port,
                    (named = DbPort) int db_port)
        : server_port_(server_port), db_port_(db_port) {}

    void print() const {
        std::cout << "  NetworkConfig: server=" << server_port_
                  << ", db=" << db_port_ << std::endl;
    }

private:
    int server_port_;
    int db_port_;
};

// ============================================================
// 4. 命名接口绑定
// ============================================================
struct ILogger {
    virtual ~ILogger() = default;
    virtual std::string name() const = 0;
};

class FileLogger : public ILogger {
public:
    std::string name() const override { return "FileLogger"; }
};

class ConsoleLogger : public ILogger {
public:
    std::string name() const override { return "ConsoleLogger"; }
};

auto AppLogger = [] {};
auto AuditLogger = [] {};

class Application {
public:
    BOOST_DI_INJECT(Application,
                    (named = AppLogger) std::shared_ptr<ILogger> app_log,
                    (named = AuditLogger) std::shared_ptr<ILogger> audit_log)
        : app_log_(app_log), audit_log_(audit_log) {}

    void print() const {
        std::cout << "  Application: app_logger=" << app_log_->name()
                  << ", audit_logger=" << audit_log_->name() << std::endl;
    }

private:
    std::shared_ptr<ILogger> app_log_;
    std::shared_ptr<ILogger> audit_log_;
};

int main() {
    std::cout << "=== Demo 05: 命名注解 ===" << std::endl;

    // 1) 基本命名参数
    std::cout << "\n[1] 基本命名参数:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<int>.named(Width).to(1920),
            di::bind<int>.named(Height).to(1080),
            di::bind<std::string>.named(Title).to(std::string("My Window"))
        );
        auto window = injector.create<Window>();
        window.print();
    }

    // 2) 多个同类型命名参数
    std::cout << "\n[2] 多个同类型命名参数:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<int>.named(ServerPort).to(8080),
            di::bind<int>.named(DbPort).to(5432)
        );
        auto config = injector.create<NetworkConfig>();
        config.print();
    }

    // 3) 命名接口绑定 —— 同一接口的不同实现用于不同用途
    std::cout << "\n[3] 命名接口绑定:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<ILogger>.named(AppLogger).to<ConsoleLogger>(),
            di::bind<ILogger>.named(AuditLogger).to<FileLogger>()
        );
        auto app = injector.create<Application>();
        app.print();
    }

    // 4) 混合使用: 命名 + 非命名
    std::cout << "\n[4] 命名与非命名参数混合:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<int>.named(Width).to(800),
            di::bind<int>.named(Height).to(600),
            di::bind<std::string>.named(Title).to(std::string("Mixed")),
            di::bind<int>.named(ServerPort).to(3000),
            di::bind<int>.named(DbPort).to(27017)
        );
        auto window = injector.create<Window>();
        window.print();
        auto config = injector.create<NetworkConfig>();
        config.print();
    }

    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
