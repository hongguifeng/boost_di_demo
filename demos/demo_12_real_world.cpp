/**
 * Demo 12: 综合实战 —— 模拟一个 Web 应用的完整 DI 配置
 *
 * 展示如何在真实项目中组合使用:
 *   - 接口绑定
 *   - 值绑定
 *   - 命名注解
 *   - 模块拆分
 *   - 单例/唯一作用域
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace di = boost::di;

// ============================================================
// 领域接口定义
// ============================================================
struct ILogger {
    virtual ~ILogger() = default;
    virtual void info(const std::string& msg) const = 0;
};

struct IDatabase {
    virtual ~IDatabase() = default;
    virtual void connect() = 0;
    virtual std::string query(const std::string& sql) const = 0;
};

struct ICache {
    virtual ~ICache() = default;
    virtual void set(const std::string& key, const std::string& val) = 0;
    virtual std::string get(const std::string& key) const = 0;
};

struct IAuthService {
    virtual ~IAuthService() = default;
    virtual bool authenticate(const std::string& token) const = 0;
};

// ============================================================
// 实现类
// ============================================================
class ConsoleLogger : public ILogger {
public:
    void info(const std::string& msg) const override {
        std::cout << "    [LOG] " << msg << std::endl;
    }
};

auto DbHost = [] {};
auto DbPort = [] {};

class PostgresDatabase : public IDatabase {
    std::shared_ptr<ILogger> logger_;
    std::string host_;
    int port_;
    bool connected_ = false;

public:
    BOOST_DI_INJECT(PostgresDatabase,
                    std::shared_ptr<ILogger> logger,
                    (named = DbHost) std::string host,
                    (named = DbPort) int port)
        : logger_(logger), host_(host), port_(port) {}

    void connect() override {
        connected_ = true;
        logger_->info("Connected to PostgreSQL at " + host_ + ":" +
                      std::to_string(port_));
    }

    std::string query(const std::string& sql) const override {
        logger_->info("Query: " + sql);
        return "result_data";
    }
};

class RedisCache : public ICache {
    std::shared_ptr<ILogger> logger_;

public:
    explicit RedisCache(std::shared_ptr<ILogger> logger) : logger_(logger) {}

    void set(const std::string& key, const std::string& val) override {
        logger_->info("Cache SET " + key + "=" + val);
    }

    std::string get(const std::string& key) const override {
        logger_->info("Cache GET " + key);
        return "cached_value";
    }
};

class JwtAuthService : public IAuthService {
    std::shared_ptr<ILogger> logger_;

public:
    explicit JwtAuthService(std::shared_ptr<ILogger> logger) : logger_(logger) {}

    bool authenticate(const std::string& token) const override {
        logger_->info("Authenticating token: " + token.substr(0, 8) + "...");
        return token.length() > 10;  // 简化校验
    }
};

// ============================================================
// 高层业务类
// ============================================================
auto ServerPort = [] {};

class UserRepository {
public:
    UserRepository(std::shared_ptr<IDatabase> db,
                   std::shared_ptr<ICache> cache,
                   std::shared_ptr<ILogger> logger)
        : db_(db), cache_(cache), logger_(logger) {}

    std::string find_user(const std::string& id) const {
        auto cached = cache_->get("user:" + id);
        if (!cached.empty()) return cached;
        return db_->query("SELECT * FROM users WHERE id=" + id);
    }

private:
    std::shared_ptr<IDatabase> db_;
    std::shared_ptr<ICache> cache_;
    std::shared_ptr<ILogger> logger_;
};

class RequestHandler {
public:
    RequestHandler(std::shared_ptr<IAuthService> auth,
                   std::shared_ptr<UserRepository> repo,
                   std::shared_ptr<ILogger> logger)
        : auth_(auth), repo_(repo), logger_(logger) {}

    void handle(const std::string& token, const std::string& user_id) const {
        logger_->info("=== 处理请求 ===");
        if (auth_->authenticate(token)) {
            auto user = repo_->find_user(user_id);
            logger_->info("处理完成, user=" + user);
        } else {
            logger_->info("认证失败!");
        }
    }

private:
    std::shared_ptr<IAuthService> auth_;
    std::shared_ptr<UserRepository> repo_;
    std::shared_ptr<ILogger> logger_;
};

class WebServer {
public:
    BOOST_DI_INJECT(WebServer,
                    std::shared_ptr<ILogger> logger,
                    std::shared_ptr<IDatabase> db,
                    std::shared_ptr<RequestHandler> handler,
                    (named = ServerPort) int port)
        : logger_(logger), db_(db), handler_(handler), port_(port) {}

    void start() {
        logger_->info("Starting server on port " + std::to_string(port_));
        db_->connect();
        logger_->info("Server ready!");

        // 模拟处理请求
        handler_->handle("valid_token_12345", "user_001");
        handler_->handle("bad", "user_002");
    }

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IDatabase> db_;
    std::shared_ptr<RequestHandler> handler_;
    int port_;
};

// ============================================================
// 模块定义
// ============================================================
auto infra_module = [] {
    return di::make_injector(
        di::bind<ILogger>.to<ConsoleLogger>(),
        di::bind<ICache>.to<RedisCache>()
    );
};

// 注意: di::bind<T>.to(lvalue) 存储的是引用!
// 参数化模块中, 局部变量/参数会在函数返回后销毁, 导致悬垂引用
// 解决方案: 使用 lambda 工厂按值捕获参数
auto data_module = [](std::string host, int port) {
    return di::make_injector(
        di::bind<IDatabase>.to<PostgresDatabase>(),
        di::bind<std::string>.named(DbHost).to([host] { return host; }),
        di::bind<int>.named(DbPort).to([port] { return port; })
    );
};

auto auth_module = [] {
    return di::make_injector(
        di::bind<IAuthService>.to<JwtAuthService>()
    );
};

int main() {
    std::cout << "=== Demo 12: 综合实战 (Web 应用) ===" << std::endl;
    std::cout << std::endl;

    // 组装所有模块
    const auto injector = di::make_injector(
        infra_module(),
        data_module("localhost", 5432),
        auth_module(),
        di::bind<int>.named(ServerPort).to(8080)
    );

    // 只需一行即可构建整个对象图!
    auto server = injector.create<WebServer>();
    server.start();

    std::cout << "\n=== 验证单例行为 ===" << std::endl;
    auto logger1 = injector.create<std::shared_ptr<ILogger>>();
    auto logger2 = injector.create<std::shared_ptr<ILogger>>();
    assert(logger1 == logger2);
    std::cout << "  Logger 是单例: " << (logger1 == logger2 ? "是" : "否")
              << std::endl;

    std::cout << "\n全部功能正常!" << std::endl;
    return 0;
}
