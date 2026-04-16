# Boost.DI 完全指南 —— 原理与实践

## 目录

- [1. 概述](#1-概述)
- [2. 快速开始](#2-快速开始)
- [3. 核心原理](#3-核心原理)
- [4. 功能详解与 Demo](#4-功能详解与-demo)
  - [4.1 自动注入 (demo_01)](#41-自动注入-demo_01)
  - [4.2 接口绑定 (demo_02)](#42-接口绑定-demo_02)
  - [4.3 值绑定 (demo_03)](#43-值绑定-demo_03)
  - [4.4 作用域 Scopes (demo_04)](#44-作用域-scopes-demo_04)
  - [4.5 命名注解 Annotations (demo_05)](#45-命名注解-annotations-demo_05)
  - [4.6 模块 Modules (demo_06)](#46-模块-modules-demo_06)
  - [4.7 动态绑定 (demo_07)](#47-动态绑定-demo_07)
  - [4.8 多重绑定 (demo_08)](#48-多重绑定-demo_08)
  - [4.9 构造函数注入控制 (demo_09)](#49-构造函数注入控制-demo_09)
  - [4.10 策略 Policies (demo_10)](#410-策略-policies-demo_10)
  - [4.11 自定义 Provider (demo_11)](#411-自定义-provider-demo_11)
  - [4.12 综合实战 (demo_12)](#412-综合实战-demo_12)
- [5. 关键注意事项与陷阱](#5-关键注意事项与陷阱)
- [6. API 速查表](#6-api-速查表)

---

## 1. 概述

**Boost.DI** (boost-ext/di) 是一个 C++14 的 **单头文件** 依赖注入 (Dependency Injection) 库，特点：

| 特性 | 说明 |
|------|------|
| **零运行时开销** | 编译期完成所有类型解析，生成的代码等同手写 |
| **单头文件** | 只需 `#include <boost/di.hpp>` |
| **无外部依赖** | 不依赖 Boost 或任何库 |
| **非侵入式** | 大多数场景不需要修改已有类 |
| **编译期保证** | 类型不匹配在编译期报错 |
| **快速编译** | 比 Java Dagger2 编译更快 |

### 什么是依赖注入?

```
不用 DI:                              用 DI:
class App {                            class App {
    App() {                                App(shared_ptr<ILogger> log,
        logger_ = make_shared              unique_ptr<IDB> db)
            <FileLogger>();                    : logger_(log), db_(move(db))
        db_ = make_unique                  { }
            <MySQL>("localhost", 3306);    };
    }
};
// 对象自己创建依赖 (紧耦合)             // 依赖从外部传入 (松耦合)
```

DI 的核心思想："**不要自己找，让别人给**" (Don't call us, we'll call you)。

---

## 2. 快速开始

### 项目结构

```
boost_di/
├── CMakeLists.txt
├── include/boost/di.hpp    ← 唯一的库文件
├── demos/
│   ├── demo_01_basic_injection.cpp
│   ├── demo_02_interface_bindings.cpp
│   ├── ...
│   └── demo_12_real_world.cpp
└── README.md
```

### 构建与运行

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 运行单个 demo
./demo_01_basic_injection

# 运行所有 demo
for demo in demo_*; do echo "=== $demo ===" && ./$demo && echo; done
```

### 最小示例

```cpp
#include <boost/di.hpp>
namespace di = boost::di;

class Greeter {
public:
    Greeter(std::string name, int times)
        : name_(name), times_(times) {}
    void greet() { /* ... */ }
private:
    std::string name_;
    int times_;
};

int main() {
    const auto injector = di::make_injector(
        di::bind<std::string>.to(std::string("World")),
        di::bind<int>.to(3)
    );
    auto greeter = injector.create<Greeter>();  // 自动注入!
    greeter.greet();
}
```

---

## 3. 核心原理

### 3.1 架构概览

```
┌──────────────────────────────────────────────────────┐
│                    make_injector                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
│  │ Bindings │  │  Scopes  │  │ Policies │           │
│  │ 类型映射  │  │ 生命周期  │  │ 约束检查  │           │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘           │
│       │              │              │                 │
│  ┌────▼──────────────▼──────────────▼─────┐          │
│  │              Core (Injector)            │          │
│  │  - ctor_traits: 推导构造函数参数        │          │
│  │  - binder: 解析依赖到具体类型           │          │
│  │  - wrapper: 转换为请求的类型            │          │
│  └────────────────┬───────────────────────┘          │
│                   │                                   │
│  ┌────────────────▼───────────────────────┐          │
│  │            Provider                     │          │
│  │  - stack_over_heap (默认)               │          │
│  │  - heap                                 │          │
│  └─────────────────────────────────────────┘          │
└──────────────────────────────────────────────────────┘
```

### 3.2 `create<T>()` 的工作流程

当调用 `injector.create<T>()` 时：

1. **策略检查** → 执行 `TConfig::policies<T>()` 验证 T 是否允许被创建
2. **构造函数推导** → `ctor_traits<T>` 找到最优构造函数
   - 优先使用 `BOOST_DI_INJECT` 标记的构造函数
   - 否则选择参数最多的构造函数
   - 支持 `T(args...)` 和 `T{args...}` 两种方式
3. **依赖解析** → `binder` 查找每个参数对应的绑定
4. **递归创建** → 对每个参数递归执行 `create()`
5. **Provider 创建** → 使用 Provider 在栈/堆上创建对象
6. **Wrapper 转换** → 转换为请求的目标类型（如 `shared_ptr<T>`）

### 3.3 构造函数推导规则

```
优先级 (从高到低):
1. BOOST_DI_INJECT(T, args...)          ← 显式标记
2. BOOST_DI_INJECT_TRAITS(args...)      ← 类型声明
3. using boost_di_inject__ = di::inject<args...>  ← 类型别名
4. di::ctor_traits<T> 特化              ← 外部特化
5. 自动推导: 选择参数最多的构造函数     ← 默认行为
   (上限 BOOST_DI_CFG_CTOR_LIMIT_SIZE = 10)
```

### 3.4 Scope 自动推导规则 (deduce)

| 请求类型 | 推导 Scope | 说明 |
|----------|-----------|------|
| `T` | unique | 每次新建 |
| `T&&` | unique | 移动语义 |
| `T*` | unique | 所有权转移 |
| `std::unique_ptr<T>` | unique | 独占所有权 |
| `T&` | singleton | 引用共享 |
| `const T&` | unique(临时) / singleton | 取决于绑定 |
| `std::shared_ptr<T>` | singleton | 共享所有权 |
| `std::weak_ptr<T>` | singleton | 弱引用 |

---

## 4. 功能详解与 Demo

### 4.1 自动注入 (demo_01)

**文件**: `demos/demo_01_basic_injection.cpp`

DI 框架自动分析构造函数参数，递归构建整个对象依赖树。

```cpp
// DI 自动推导构造函数参数
class Service {
public:
    Service(const Repository& repo, double timeout)  // 自动注入
        : repo_(repo), timeout_(timeout) {}
};

const auto injector = di::make_injector(
    di::bind<int>.to(42),
    di::bind<double>.to(3.14)
);

// 自动创建整棵依赖树: Service -> Repository -> Database -> int
auto service = injector.create<Service>();

// 支持多种智能指针
auto ptr = injector.create<std::unique_ptr<Service>>();
auto sptr = injector.create<std::shared_ptr<Service>>();
```

**关键点**:
- 不需要修改任何类的代码
- DI 递归解析所有嵌套依赖
- 支持聚合类型 `struct { int a; double b; }`

---

### 4.2 接口绑定 (demo_02)

**文件**: `demos/demo_02_interface_bindings.cpp`

将抽象接口映射到具体实现，实现松耦合。

```cpp
// 接口 -> 实现
di::bind<ILogger>.to<ConsoleLogger>()      // ILogger 用 ConsoleLogger 实现
di::bind<IDatabase>.to<MySQLDatabase>()    // IDatabase 用 MySQLDatabase 实现

// 切换实现只需改绑定，业务代码完全不变
di::bind<ILogger>.to<FileLogger>()         // 切换到 FileLogger

// 多个接口绑定到同一类
di::bind<IReader, IWriter>.to<ReadWriter>()
// IReader 和 IWriter 共享同一个 ReadWriter 实例 (singleton)
```

---

### 4.3 值绑定 (demo_03)

**文件**: `demos/demo_03_value_bindings.cpp`

```cpp
// 1) 绑定常量值
di::bind<int>.to(8080)

// 2) 绑定已有 shared_ptr 对象
auto svc = std::make_shared<ServiceImpl>();
di::bind<IService>.to(svc)

// 3) Lambda 工厂 (每次调用)
di::bind<int>.to([] { return generate_port(); })

// 4) 绑定到外部引用 (⚠️ 调用者负责生命周期!)
int port = 9090;
di::bind<int>.to(port)
// injector.create<int&>() 返回的引用指向 port 变量

// 5) override: 覆盖之前的绑定
di::bind<int>.to(80),
di::bind<int>.to(443)[di::override]  // 最终值为 443

// 6) 自动类型推导
di::bind<>.to(42)       // 推导为 int
di::bind<>.to(3.14)     // 推导为 double
```

**⚠️ 重要陷阱**: `di::bind<T>.to(lvalue)` 存储的是**引用**，不是拷贝!
详见 [第5节 注意事项](#5-关键注意事项与陷阱)。

---

### 4.4 作用域 Scopes (demo_04)

**文件**: `demos/demo_04_scopes.cpp`

```cpp
// unique: 每次创建新对象
di::bind<IService>.in(di::unique).to<ServiceImpl>()

// singleton: injector 生命周期内共享同一实例
di::bind<IService>.in(di::singleton).to<ServiceImpl>()

// deduce (默认): 根据请求类型自动推导
di::bind<IService>.to<ServiceImpl>()  // 不指定 scope
// → shared_ptr<IService> 自动用 singleton
// → unique_ptr<IService> 自动用 unique
```

**Scope 生命周期图**:

```
unique:       create()  ──→  新对象1
              create()  ──→  新对象2  (每次不同)

singleton:    create()  ──→  对象A ──┐
              create()  ──→  对象A ──┘ (始终同一个)

deduce:       create<shared_ptr>()  ──→  singleton 行为
              create<unique_ptr>()  ──→  unique 行为
```

---

### 4.5 命名注解 Annotations (demo_05)

**文件**: `demos/demo_05_annotations.cpp`

解决同一类型多个参数的歧义问题。

```cpp
// 1) 定义命名标识 (每个 lambda 类型唯一)
auto Width = [] {};
auto Height = [] {};

// 2) 在构造函数中标注
class Window {
public:
    BOOST_DI_INJECT(Window,
        (named = Width) int w,
        (named = Height) int h)
        : width_(w), height_(h) {}
};

// 3) 绑定时指定名称
di::bind<int>.named(Width).to(1920),
di::bind<int>.named(Height).to(1080)

// 4) 命名接口绑定 —— 同一接口不同实现
auto AppLog = [] {};
auto AuditLog = [] {};
di::bind<ILogger>.named(AppLog).to<ConsoleLogger>(),
di::bind<ILogger>.named(AuditLog).to<FileLogger>()
```

**注意**: `BOOST_DI_INJECT` 只用于**声明**参数类型和注解，构造函数的实现代码不需要注解。

---

### 4.6 模块 Modules (demo_06)

**文件**: `demos/demo_06_modules.cpp`

将绑定拆分到独立模块，再组合到主 injector。

```cpp
// 方式 1: Lambda 模块 (暴露所有类型)
auto logging_module = [] {
    return di::make_injector(
        di::bind<ILogger>.to<ConsoleLogger>()
    );
};

// 方式 2: 类型限定模块 (只暴露指定类型)
di::injector<std::shared_ptr<ILogger>> make_logger() {
    return di::make_injector(
        di::bind<ILogger>.to<ConsoleLogger>()
    );
}

// 方式 3: 参数化模块 (⚠️ 注意生命周期!)
auto config_module = [](int threads) {
    return di::make_injector(
        di::bind<int>.to([threads] { return threads; })  // 用 lambda 捕获!
    );
};

// 组合模块
const auto injector = di::make_injector(
    logging_module(),
    storage_module(),
    config_module(8)
);
```

---

### 4.7 动态绑定 (demo_07)

**文件**: `demos/demo_07_dynamic_bindings.cpp`

运行时决定使用哪个实现。

```cpp
std::string protocol = "grpc";  // 可来自配置文件

di::bind<ITransport>.to([&](const auto& injector)
    -> std::shared_ptr<ITransport> {
    if (protocol == "http")
        return injector.template create<std::shared_ptr<HttpTransport>>();
    else if (protocol == "grpc")
        return injector.template create<std::shared_ptr<GrpcTransport>>();
    else
        return injector.template create<std::shared_ptr<WebSocketTransport>>();
})
```

**要点**: Lambda 参数中的 `injector` 可用于递归创建子依赖。注意使用 `template` 关键字。

---

### 4.8 多重绑定 (demo_08)

**文件**: `demos/demo_08_multiple_bindings.cpp`

注入集合 (vector/set)。

```cpp
// 接口的多个实现 → vector<unique_ptr<IPlugin>>
di::bind<IPlugin*[]>.to<AuthPlugin, LogPlugin, CachePlugin>()
// → injector.create<std::vector<std::unique_ptr<IPlugin>>>() 得到 3 个插件

// 值列表 → vector<int>
auto ports = {8080, 8081, 8082};
di::bind<int[]>.to(ports)
// → injector.create<std::vector<int>>() 得到 {8080, 8081, 8082}
```

---

### 4.9 构造函数注入控制 (demo_09)

**文件**: `demos/demo_09_injection_control.cpp`

| 方式 | 适用场景 | 参数限制 |
|------|---------|---------|
| `BOOST_DI_INJECT(T, args...)` | 类内标注，消除构造函数歧义 | ≤ 10 |
| `BOOST_DI_INJECT_TRAITS(args...)` | 声明与定义分离 | ≤ 10 |
| `using boost_di_inject__ = di::inject<args...>` | 泛型类 | 无限制 |
| `di::ctor_traits<T>` 特化 | **第三方类** (不能修改源码) | ≤ 10 |

```cpp
// 第三方类无法修改 → 用 ctor_traits 外部指定
namespace boost { inline namespace ext { namespace di {
template <> struct ctor_traits<ThirdPartyClass> {
    BOOST_DI_INJECT_TRAITS(int, double);
};
}}}
```

---

### 4.10 策略 Policies (demo_10)

**文件**: `demos/demo_10_policies.cpp`

编译时约束 + 运行时诊断。

```cpp
// 运行时: 打印所有创建的类型
class print_config : public di::config {
    static auto policies(...) noexcept {
        return di::make_policies([](auto type) {
            using T = typename decltype(type)::type;
            std::cout << typeid(T).name() << std::endl;
        });
    }
};

// 编译时: 所有依赖必须显式绑定
class strict_config : public di::config {
    static auto policies(...) noexcept {
        using namespace di::policies;
        using namespace di::policies::operators;
        return di::make_policies(
            constructible(is_bound<di::_>{})  // 未绑定的类型 → 编译错误
        );
    }
};

auto injector = di::make_injector<strict_config>(...);
```

---

### 4.11 自定义 Provider (demo_11)

**文件**: `demos/demo_11_providers.cpp`

控制对象的内存分配方式。

```cpp
// 默认: stack_over_heap (尽量栈上, 需要时堆上)
// 内置: di::providers::heap (全部堆上)

// 自定义: nothrow new
class nothrow_provider {
    template <class...> struct is_creatable {
        static constexpr auto value = true;
    };
    template <class T, class TInit, class TMemory, class... TArgs>
    auto get(const TInit&, const TMemory&, TArgs&&... args) const noexcept {
        return new (std::nothrow) T{std::forward<TArgs>(args)...};
    }
};

class my_config : public di::config {
    static auto provider(...) noexcept { return nothrow_provider{}; }
};

auto injector = di::make_injector<my_config>(...);
```

---

### 4.12 综合实战 (demo_12)

**文件**: `demos/demo_12_real_world.cpp`

模拟一个 Web 应用，组合使用所有核心特性：

```
WebServer
├── ILogger (singleton) ─── ConsoleLogger
├── IDatabase (singleton) ─── PostgresDatabase
│   ├── ILogger (同上单例)
│   ├── DbHost: "localhost" (named)
│   └── DbPort: 5432 (named)
├── RequestHandler
│   ├── IAuthService ─── JwtAuthService
│   │   └── ILogger (同上单例)
│   ├── UserRepository
│   │   ├── IDatabase (同上单例)
│   │   ├── ICache ─── RedisCache
│   │   └── ILogger (同上单例)
│   └── ILogger (同上单例)
└── ServerPort: 8080 (named)
```

```cpp
// 按职责拆分为模块
const auto injector = di::make_injector(
    infra_module(),               // ILogger, ICache
    data_module("localhost", 5432), // IDatabase
    auth_module(),                // IAuthService
    di::bind<int>.named(ServerPort).to(8080)
);

// 一行代码构建整个对象图!
auto server = injector.create<WebServer>();
server.start();
```

---

## 5. 关键注意事项与陷阱

### ⚠️ 陷阱 1: `di::bind<T>.to(lvalue)` 存储引用，不是拷贝

```cpp
// ❌ 危险! port 是局部变量, 离开作用域后悬垂引用
auto make_module(int port) {
    return di::make_injector(
        di::bind<int>.to(port)  // 存储了对 port 的引用!
    );
}

// ✅ 安全: 用 lambda 按值捕获
auto make_module(int port) {
    return di::make_injector(
        di::bind<int>.to([port] { return port; })
    );
}

// ✅ 也安全: 确保变量生命周期 >= injector
int port = 8080;  // 在 injector 之前声明
auto injector = di::make_injector(di::bind<int>.to(port));
// port 和 injector 在同一作用域, OK
```

### ⚠️ 陷阱 2: 同类型绑定冲突

```cpp
// ❌ 编译错误: int 绑定了两次
di::bind<int>.to(42),
di::bind<int>.to(100)

// ✅ 用 override
di::bind<int>.to(42),
di::bind<int>.to(100)[di::override]

// ✅ 用 named 区分
di::bind<int>.named(Port).to(42),
di::bind<int>.named(Timeout).to(100)
```

### ⚠️ 陷阱 3: 模板类的隐式转换构造函数

```cpp
// ❌ DI 不支持 template<class I> T(I) 形式的转换构造函数
class Bad {
    template<class T> Bad(T) {}  // 无法自动注入
};

// ✅ 使用 BOOST_DI_INJECT 显式标注
class Good {
    BOOST_DI_INJECT(Good, int a, double b) {}
};
```

### ⚠️ 陷阱 4: 构造函数参数上限

默认上限为 10 个参数。需要更多时：
```cpp
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 20  // 在 #include 之前定义
#include <boost/di.hpp>
```

### 最佳实践

1. **依赖接口，不依赖实现** —— 构造函数参数用接口类型
2. **不要传递 injector** —— avoid Service Locator 反模式
3. **组合根** —— 在程序入口处创建唯一 injector 并创建对象图
4. **模块化** —— 按职责拆分 bindings 到不同模块
5. **singleton 谨慎使用** —— 优先使用 deduce 让框架自动决定

---

## 6. API 速查表

### 创建 Injector

```cpp
auto injector = di::make_injector(bindings...);
auto injector = di::make_injector<MyConfig>(bindings...);
```

### Bindings

| 语法 | 说明 |
|------|------|
| `di::bind<Interface>.to<Impl>()` | 接口 → 实现 |
| `di::bind<I1, I2>.to<Impl>()` | 多接口 → 同一实现 |
| `di::bind<T>.to(value)` | 绑定到值 (**引用语义!**) |
| `di::bind<T>.to([]{ return v; })` | Lambda 工厂 (**值语义**) |
| `di::bind<T>.to(shared_ptr)` | 绑定到已有对象 |
| `di::bind<>.to(value)` | 自动类型推导 |
| `di::bind<T>.to(lambda)(const auto& injector)` | 动态绑定 |
| `di::bind<T*[]>.to<A, B, C>()` | 多重绑定 (→ vector) |
| `di::bind<int[]>.to({1,2,3})` | 值列表 (→ vector) |

### Binding 修饰符

| 修饰符 | 说明 |
|--------|------|
| `.in(di::unique)` | 每次新建 |
| `.in(di::singleton)` | 单例 |
| `.named(name)` | 命名绑定 |
| `[di::override]` | 覆盖已有绑定 |

### 创建对象

| 语法 | 说明 |
|------|------|
| `injector.create<T>()` | 栈上创建 |
| `injector.create<T&>()` | 获取引用 (singleton) |
| `injector.create<std::unique_ptr<T>>()` | 独占指针 |
| `injector.create<std::shared_ptr<T>>()` | 共享指针 (singleton) |
| `injector.create<T*>()` | 原始指针 (**需手动 delete!**) |

### 构造函数控制

```cpp
BOOST_DI_INJECT(ClassName, args...)               // 类内标注
BOOST_DI_INJECT_TRAITS(types...)                   // 类型列表
using boost_di_inject__ = di::inject<types...>;    // 类型别名
namespace boost::ext::di { template<> struct ctor_traits<T> { ... }; }  // 外部特化
```

### 编译选项

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `BOOST_DI_CFG` | `di::config` | 全局默认配置 |
| `BOOST_DI_CFG_CTOR_LIMIT_SIZE` | 10 | 最大构造函数参数数 |
| `BOOST_DI_CFG_DIAGNOSTICS_LEVEL` | 0 | 错误信息详细程度 (0-2) |
