/**
 * Demo 01: 基本自动注入 —— Boost.DI 自动推导构造函数并注入依赖
 *
 * 核心概念:
 *   - di::make_injector() 创建注入器
 *   - injector.create<T>() 创建对象, DI 自动递归构造所有依赖
 *   - 自动选择参数最多的构造函数进行注入
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>

namespace di = boost::di;

// ============================================================
// 1. 最简单的情况: 构造函数参数为基本类型
// ============================================================
class Logger {
public:
    // DI 框架会自动找到参数最多的构造函数
    Logger(int log_level, double rate)
        : log_level_(log_level), rate_(rate) {}

    void print() const {
        std::cout << "  Logger: level=" << log_level_
                  << ", rate=" << rate_ << std::endl;
    }

private:
    int log_level_;
    double rate_;
};

// ============================================================
// 2. 嵌套依赖: DI 递归解析整棵对象树
// ============================================================
class Database {
public:
    explicit Database(int port) : port_(port) {}
    int port() const { return port_; }

private:
    int port_;
};

class Repository {
public:
    explicit Repository(const Database& db) : db_(db) {}
    int db_port() const { return db_.port(); }

private:
    Database db_;
};

class Service {
public:
    // DI 自动构造 Repository -> Database -> int 整棵依赖树
    Service(const Repository& repo, double timeout)
        : repo_(repo), timeout_(timeout) {}

    void print() const {
        std::cout << "  Service: db_port=" << repo_.db_port()
                  << ", timeout=" << timeout_ << std::endl;
    }

private:
    Repository repo_;
    double timeout_;
};

// ============================================================
// 3. 聚合类型(aggregate): DI 也支持 T{...} 方式初始化
// ============================================================
struct Config {
    int max_connections;
    double timeout;
};

int main() {
    std::cout << "=== Demo 01: 基本自动注入 ===" << std::endl;

    // 创建注入器并绑定值
    const auto injector = di::make_injector(
        di::bind<int>.to(42),
        di::bind<double>.to(3.14)
    );

    // 1) 基本类型注入
    std::cout << "\n[1] 基本类型注入:" << std::endl;
    auto logger = injector.create<Logger>();
    logger.print();

    // 2) 嵌套依赖注入 —— DI 自动递归构建整棵对象树
    std::cout << "\n[2] 嵌套依赖注入:" << std::endl;
    auto service = injector.create<Service>();
    service.print();

    // 3) 聚合类型注入
    std::cout << "\n[3] 聚合类型注入:" << std::endl;
    auto config = injector.create<Config>();
    std::cout << "  Config: max_connections=" << config.max_connections
              << ", timeout=" << config.timeout << std::endl;

    // 4) 支持多种创建方式
    std::cout << "\n[4] 不同智能指针创建方式:" << std::endl;
    auto logger_unique = injector.create<std::unique_ptr<Logger>>();
    logger_unique->print();
    auto logger_shared = injector.create<std::shared_ptr<Logger>>();
    logger_shared->print();

    std::cout << "\n所有断言通过!" << std::endl;
    return 0;
}
