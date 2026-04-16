/**
 * Demo 08: 多重绑定(Multiple Bindings) —— 注入集合
 *
 * 核心概念:
 *   - di::bind<Interface*[]>.to<Impl1, Impl2, ...>()
 *     将多个实现绑定到同一接口, 注入为 vector/set 等容器
 *   - di::bind<int[]>.to({1, 2, 3})
 *     绑定值列表
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

namespace di = boost::di;

// ============================================================
// 接口和多个实现
// ============================================================
struct IPlugin {
    virtual ~IPlugin() = default;
    virtual std::string name() const = 0;
    virtual void execute() const = 0;
};

class AuthPlugin : public IPlugin {
public:
    std::string name() const override { return "AuthPlugin"; }
    void execute() const override {
        std::cout << "    -> 执行认证检查" << std::endl;
    }
};

class LogPlugin : public IPlugin {
public:
    std::string name() const override { return "LogPlugin"; }
    void execute() const override {
        std::cout << "    -> 记录操作日志" << std::endl;
    }
};

class CachePlugin : public IPlugin {
public:
    std::string name() const override { return "CachePlugin"; }
    void execute() const override {
        std::cout << "    -> 更新缓存" << std::endl;
    }
};

// 插件管理器: 注入所有插件的集合
class PluginManager {
public:
    explicit PluginManager(
        std::vector<std::unique_ptr<IPlugin>> plugins)
        : plugins_(std::move(plugins)) {}

    void run_all() const {
        std::cout << "  PluginManager: 共 " << plugins_.size()
                  << " 个插件" << std::endl;
        for (const auto& p : plugins_) {
            std::cout << "  [" << p->name() << "]" << std::endl;
            p->execute();
        }
    }

private:
    std::vector<std::unique_ptr<IPlugin>> plugins_;
};

int main() {
    std::cout << "=== Demo 08: 多重绑定 ===" << std::endl;

    // 1) 接口的多重绑定 → vector
    std::cout << "\n[1] 接口多重绑定 (注入 vector<unique_ptr<IPlugin>>):" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<IPlugin*[]>.to<AuthPlugin, LogPlugin, CachePlugin>()
        );

        auto manager = injector.create<PluginManager>();
        manager.run_all();
    }

    // 2) 值的多重绑定 → vector<int>
    std::cout << "\n[2] 值多重绑定 (注入 vector<int>):" << std::endl;
    {
        auto ports = {8080, 8081, 8082, 9090};
        const auto injector = di::make_injector(
            di::bind<int[]>.to(ports)
        );

        auto port_list = injector.create<std::vector<int>>();
        std::cout << "  端口列表: ";
        for (int p : port_list) {
            std::cout << p << " ";
        }
        std::cout << std::endl;
        assert(port_list.size() == 4);
    }

    // 3) 直接创建 vector
    std::cout << "\n[3] 直接创建接口的 vector:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<IPlugin*[]>.to<AuthPlugin, LogPlugin>()
        );

        auto plugins = injector.create<std::vector<std::unique_ptr<IPlugin>>>();
        std::cout << "  直接获取 " << plugins.size() << " 个插件:" << std::endl;
        for (const auto& p : plugins) {
            std::cout << "    - " << p->name() << std::endl;
        }
    }

    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
