/**
 * Demo 07: 动态绑定 —— 运行时决定注入哪个实现
 *
 * 核心概念:
 *   - di::bind<Interface>.to([](const auto& injector) { ... })
 *     通过 lambda 在运行时决定返回哪个具体实现
 *   - 可以根据配置文件、环境变量、命令行参数等条件动态选择
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>

namespace di = boost::di;

// ============================================================
// 接口和多个实现
// ============================================================
struct ITransport {
    virtual ~ITransport() = default;
    virtual std::string protocol() const = 0;
};

class HttpTransport : public ITransport {
public:
    std::string protocol() const override { return "HTTP"; }
};

class GrpcTransport : public ITransport {
public:
    std::string protocol() const override { return "gRPC"; }
};

class WebSocketTransport : public ITransport {
public:
    std::string protocol() const override { return "WebSocket"; }
};

class Client {
public:
    explicit Client(std::shared_ptr<ITransport> transport)
        : transport_(transport) {}

    void connect() const {
        std::cout << "  Client connecting via: "
                  << transport_->protocol() << std::endl;
    }

private:
    std::shared_ptr<ITransport> transport_;
};

int main() {
    std::cout << "=== Demo 07: 动态绑定 ===" << std::endl;

    // 模拟运行时配置
    std::string config_protocol = "grpc";  // 可以来自配置文件/命令行

    // 1) 基于条件的动态绑定
    std::cout << "\n[1] 基于运行时条件选择实现:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<ITransport>.to([&](const auto& injector)
                -> std::shared_ptr<ITransport> {
                if (config_protocol == "http") {
                    return injector.template create<std::shared_ptr<HttpTransport>>();
                } else if (config_protocol == "grpc") {
                    return injector.template create<std::shared_ptr<GrpcTransport>>();
                } else {
                    return injector.template create<std::shared_ptr<WebSocketTransport>>();
                }
            })
        );

        auto client = injector.create<Client>();
        client.connect();  // 输出: gRPC
    }

    // 2) 切换配置后重新创建
    std::cout << "\n[2] 切换配置后重新创建:" << std::endl;
    {
        bool use_secure = true;

        const auto injector = di::make_injector(
            di::bind<ITransport>.to([&](const auto& injector)
                -> std::shared_ptr<ITransport> {
                if (use_secure) {
                    return injector.template create<std::shared_ptr<GrpcTransport>>();
                }
                return injector.template create<std::shared_ptr<HttpTransport>>();
            })
        );

        auto client1 = injector.create<Client>();
        client1.connect();  // gRPC

        use_secure = false;
        auto client2 = injector.create<Client>();
        client2.connect();  // HTTP
    }

    // 3) 动态绑定与值绑定组合
    std::cout << "\n[3] 动态绑定 + 值组合:" << std::endl;
    {
        int env = 2;  // 1=dev, 2=staging, 3=prod

        const auto injector = di::make_injector(
            di::bind<ITransport>.to([&](const auto& injector)
                -> std::shared_ptr<ITransport> {
                switch (env) {
                    case 1: return injector.template create<std::shared_ptr<HttpTransport>>();
                    case 2: return injector.template create<std::shared_ptr<GrpcTransport>>();
                    default: return injector.template create<std::shared_ptr<WebSocketTransport>>();
                }
            }),
            di::bind<int>.to(env)
        );

        auto client = injector.create<Client>();
        client.connect();  // staging -> gRPC
    }

    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
