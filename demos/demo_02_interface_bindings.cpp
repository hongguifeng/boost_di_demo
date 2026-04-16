/**
 * Demo 02: 接口绑定 —— 将抽象接口绑定到具体实现
 *
 * 核心概念:
 *   - di::bind<Interface>.to<Implementation>()  接口到实现的绑定
 *   - 通过 shared_ptr / unique_ptr 持有多态对象
 *   - 多个接口绑定到同一个实现类
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>

namespace di = boost::di;

// ============================================================
// 定义接口和实现
// ============================================================
struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(const std::string& msg) const = 0;
};

class ConsoleLogger : public ILogger {
public:
    void log(const std::string& msg) const override {
        std::cout << "  [ConsoleLogger] " << msg << std::endl;
    }
};

class FileLogger : public ILogger {
public:
    void log(const std::string& msg) const override {
        std::cout << "  [FileLogger] " << msg << std::endl;
    }
};

struct IDatabase {
    virtual ~IDatabase() = default;
    virtual std::string query() const = 0;
};

class MySQLDatabase : public IDatabase {
public:
    std::string query() const override { return "MySQL result"; }
};

// ============================================================
// 业务类: 依赖接口而非具体实现
// ============================================================
class UserService {
public:
    UserService(std::shared_ptr<ILogger> logger,
                std::unique_ptr<IDatabase> db)
        : logger_(logger), db_(std::move(db)) {}

    void get_user() const {
        logger_->log("Querying user...");
        auto result = db_->query();
        logger_->log("Got: " + result);
    }

private:
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<IDatabase> db_;
};

// ============================================================
// 多个接口由同一个类实现
// ============================================================
struct IReader {
    virtual ~IReader() = default;
    virtual int read() const = 0;
};

struct IWriter {
    virtual ~IWriter() = default;
    virtual void write(int v) const = 0;
};

class ReadWriter : public IReader, public IWriter {
public:
    int read() const override { return 42; }
    void write(int v) const override {
        std::cout << "  [ReadWriter] write: " << v << std::endl;
    }
};

int main() {
    std::cout << "=== Demo 02: 接口绑定 ===" << std::endl;

    // 1) 基本接口绑定
    std::cout << "\n[1] 接口 -> 实现绑定:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<ILogger>.to<ConsoleLogger>(),
            di::bind<IDatabase>.to<MySQLDatabase>()
        );

        auto service = injector.create<UserService>();
        service.get_user();
    }

    // 2) 切换实现 —— 只需修改绑定, 业务代码完全不变
    std::cout << "\n[2] 切换实现 (ConsoleLogger -> FileLogger):" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<ILogger>.to<FileLogger>(),     // 仅此处改变
            di::bind<IDatabase>.to<MySQLDatabase>()
        );

        auto service = injector.create<UserService>();
        service.get_user();
    }

    // 3) 多接口绑定到同一实现
    std::cout << "\n[3] 多个接口绑定到同一个实现:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<IReader, IWriter>.to<ReadWriter>()
        );

        auto reader = injector.create<std::shared_ptr<IReader>>();
        auto writer = injector.create<std::shared_ptr<IWriter>>();
        std::cout << "  read() = " << reader->read() << std::endl;
        writer->write(100);

        // 两个接口实际上共享同一个对象(singleton scope)
        assert(reader.get() == dynamic_cast<IReader*>(
            dynamic_cast<ReadWriter*>(writer.get())));
        std::cout << "  同一个对象实例! (shared_ptr singleton)" << std::endl;
    }

    std::cout << "\n所有断言通过!" << std::endl;
    return 0;
}
