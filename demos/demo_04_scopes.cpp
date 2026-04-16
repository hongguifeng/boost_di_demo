/**
 * Demo 04: 作用域(Scopes) —— 控制对象的生命周期
 *
 * 核心概念:
 *   - di::unique     每次请求创建新对象
 *   - di::singleton  全局单例(injector 生命周期内共享)
 *   - di::deduce     默认行为, 根据请求类型自动推导:
 *       T, T&&, T*, unique_ptr<T> -> unique
 *       T&, shared_ptr<T>, weak_ptr<T> -> singleton
 */
#include <boost/di.hpp>
#include <cassert>
#include <iostream>
#include <memory>

namespace di = boost::di;

struct IService {
    virtual ~IService() = default;
    virtual int id() const = 0;
};

class ServiceImpl : public IService {
    static int next_id_;
    int id_;

public:
    ServiceImpl() : id_(++next_id_) {
        std::cout << "  [构造] ServiceImpl #" << id_ << std::endl;
    }
    int id() const override { return id_; }
};
int ServiceImpl::next_id_ = 0;

int main() {
    std::cout << "=== Demo 04: 作用域(Scopes) ===" << std::endl;

    // ============================================================
    // 1) di::unique — 每次创建新实例
    // ============================================================
    std::cout << "\n[1] di::unique 作用域:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<IService>.in(di::unique).to<ServiceImpl>()
        );

        auto s1 = injector.create<std::shared_ptr<IService>>();
        auto s2 = injector.create<std::shared_ptr<IService>>();
        std::cout << "  s1.id=" << s1->id() << ", s2.id=" << s2->id() << std::endl;
        assert(s1 != s2);  // 不同对象
        std::cout << "  确认: 每次创建不同实例" << std::endl;
    }

    // ============================================================
    // 2) di::singleton — 同一个 injector 内共享同一实例
    // ============================================================
    std::cout << "\n[2] di::singleton 作用域:" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<IService>.in(di::singleton).to<ServiceImpl>()
        );

        auto s1 = injector.create<std::shared_ptr<IService>>();
        auto s2 = injector.create<std::shared_ptr<IService>>();
        std::cout << "  s1.id=" << s1->id() << ", s2.id=" << s2->id() << std::endl;
        assert(s1 == s2);  // 同一个对象
        std::cout << "  确认: 共享同一实例 (singleton)" << std::endl;

        // singleton 也可以通过引用获取
        IService& ref1 = injector.create<IService&>();
        IService& ref2 = injector.create<IService&>();
        assert(&ref1 == &ref2);
        (void)ref1; (void)ref2;
        std::cout << "  确认: 引用也指向同一实例" << std::endl;
    }

    // ============================================================
    // 3) di::deduce — 默认行为, 根据请求类型自动推导
    // ============================================================
    std::cout << "\n[3] di::deduce 作用域 (默认):" << std::endl;
    {
        const auto injector = di::make_injector(
            di::bind<IService>.to<ServiceImpl>()  // 默认 deduce
        );

        // shared_ptr -> singleton
        auto sp1 = injector.create<std::shared_ptr<IService>>();
        auto sp2 = injector.create<std::shared_ptr<IService>>();
        assert(sp1 == sp2);
        std::cout << "  shared_ptr: 同一实例 (auto-singleton)" << std::endl;

        // unique_ptr -> unique (每次不同)
        auto up1 = injector.create<std::unique_ptr<IService>>();
        auto up2 = injector.create<std::unique_ptr<IService>>();
        assert(up1 != up2);
        std::cout << "  unique_ptr: 不同实例 (auto-unique)" << std::endl;
    }

    // ============================================================
    // 4) 混合使用: 一个复杂对象中不同依赖使用不同 scope
    // ============================================================
    std::cout << "\n[4] 混合 scope 示例:" << std::endl;
    {
        struct Worker {
            Worker(std::shared_ptr<IService> shared_svc,
                   std::unique_ptr<IService> unique_svc,
                   int config_val)
                : shared_svc_(shared_svc)
                , unique_svc_(std::move(unique_svc))
                , config_(config_val) {}

            std::shared_ptr<IService> shared_svc_;  // singleton (by deduce)
            std::unique_ptr<IService> unique_svc_;  // unique (by deduce)
            int config_;
        };

        const auto injector = di::make_injector(
            di::bind<IService>.to<ServiceImpl>(),
            di::bind<int>.to(100)
        );

        auto w1 = injector.create<Worker>();
        auto w2 = injector.create<Worker>();

        // shared_ptr 部分共享同一实例
        assert(w1.shared_svc_ == w2.shared_svc_);
        std::cout << "  shared_svc: 共享 (id="
                  << w1.shared_svc_->id() << ")" << std::endl;

        // unique_ptr 部分每次不同
        assert(w1.unique_svc_->id() != w2.unique_svc_->id());
        std::cout << "  unique_svc: w1.id=" << w1.unique_svc_->id()
                  << ", w2.id=" << w2.unique_svc_->id() << std::endl;
    }

    std::cout << "\n所有断言通过!" << std::endl;
    return 0;
}
