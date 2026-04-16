/**
 * Demo 06: 模块(Modules) —— 将绑定配置拆分成独立模块
 *
 * 核心概念:
 *   - auto module = di::make_injector(bindings...)  创建模块
 *   - 模块可以组合到主 injector 中
 *   - di::injector<Ts...>  限定模块暴露的类型列表
 *   - BOOST_DI_EXPOSE  暴露命名参数
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>

namespace di = boost::di;

// ============================================================
// 接口定义
// ============================================================
struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(const std::string& msg) const = 0;
};

struct IDatabase {
    virtual ~IDatabase() = default;
    virtual std::string name() const = 0;
};

struct ICache {
    virtual ~ICache() = default;
    virtual int size() const = 0;
};

// 实现类
class ConsoleLogger : public ILogger {
public:
    void log(const std::string& msg) const override {
        std::cout << "  [Log] " << msg << std::endl;
    }
};

class PostgresDB : public IDatabase {
public:
    std::string name() const override { return "PostgreSQL"; }
};

class RedisCache : public ICache {
public:
    int size() const override { return 256; }
};

// 业务类
class Application {
public:
    Application(std::shared_ptr<ILogger> logger,
                std::unique_ptr<IDatabase> db,
                std::shared_ptr<ICache> cache,
                int max_threads)
        : logger_(logger)
        , db_(std::move(db))
        , cache_(cache)
        , max_threads_(max_threads) {}

    void run() const {
        logger_->log("Starting application...");
        logger_->log("DB: " + db_->name());
        logger_->log("Cache size: " + std::to_string(cache_->size()));
        logger_->log("Max threads: " + std::to_string(max_threads_));
    }

private:
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<IDatabase> db_;
    std::shared_ptr<ICache> cache_;
    int max_threads_;
};

// ============================================================
// 方式一: 使用 auto (lambda) 返回模块, 暴露所有类型
// ============================================================
auto logging_module = [] {
    return di::make_injector(
        di::bind<ILogger>.to<ConsoleLogger>()
    );
};

auto storage_module = [] {
    return di::make_injector(
        di::bind<IDatabase>.to<PostgresDB>(),
        di::bind<ICache>.to<RedisCache>()
    );
};

auto config_module = [] {
    return di::make_injector(
        di::bind<int>.to(8)
    );
};

// ============================================================
// 方式二: 使用 di::injector<Ts...> 限定暴露类型
// ============================================================
di::injector<std::shared_ptr<ILogger>> make_logger_module() {
    return di::make_injector(
        di::bind<ILogger>.to<ConsoleLogger>()
    );
}

di::injector<std::unique_ptr<IDatabase>, std::shared_ptr<ICache>>
make_storage_module() {
    return di::make_injector(
        di::bind<IDatabase>.to<PostgresDB>(),
        di::bind<ICache>.to<RedisCache>()
    );
}

int main() {
    std::cout << "=== Demo 06: 模块 ===" << std::endl;

    // 1) auto 模块组合
    std::cout << "\n[1] Lambda 模块组合:" << std::endl;
    {
        const auto injector = di::make_injector(
            logging_module(),
            storage_module(),
            config_module()
        );
        auto app = injector.create<Application>();
        app.run();
    }

    // 2) 类型限定模块
    std::cout << "\n[2] 类型限定模块 (di::injector<Ts...>):" << std::endl;
    {
        const auto injector = di::make_injector(
            make_logger_module(),
            make_storage_module(),
            di::bind<int>.to(16)
        );
        auto app = injector.create<Application>();
        app.run();
    }

    // 3) 模块覆盖 —— 子 injector 中用 override 覆盖模块里的绑定
    std::cout << "\n[3] 模块覆盖:" << std::endl;
    {
        const auto injector = di::make_injector(
            logging_module(),
            storage_module(),
            di::bind<int>.to(4),          // 基础配置
            di::bind<int>.to(32)[di::override]  // 覆盖
        );
        auto threads = injector.create<int>();
        std::cout << "  max_threads = " << threads << " (已被 override)" << std::endl;
        assert(threads == 32);
    }

    // 4) 参数化模块 —— 模块接受运行时参数
    //    注意: di::bind<T>.to(lvalue) 存储引用, 所以必须确保值的生命周期
    //    使用 lambda 工厂可以安全地按值捕获参数
    std::cout << "\n[4] 参数化模块:" << std::endl;
    {
        auto configurable_module = [](int threads) {
            return di::make_injector(
                di::bind<int>.to([threads] { return threads; })
            );
        };

        const auto injector = di::make_injector(
            logging_module(),
            storage_module(),
            configurable_module(64)
        );
        auto app = injector.create<Application>();
        app.run();
    }

    std::cout << "\n所有测试通过!" << std::endl;
    return 0;
}
