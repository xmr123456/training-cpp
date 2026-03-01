#include "../exercise.h"
#include <memory>

// READ: `std::shared_ptr` <https://zh.cppreference.com/w/cpp/memory/shared_ptr>
// READ: `std::weak_ptr` <https://zh.cppreference.com/w/cpp/memory/weak_ptr>

// TODO: 将下列 `?` 替换为正确的值
void TestSharedPtr() {
    auto shared = std::make_shared<int>(10);
    std::shared_ptr<int> ptrs[]{shared, shared, shared};

    std::weak_ptr<int> observer = shared;
    ASSERT(observer.use_count() == 4, "");

    ptrs[0].reset();
    ASSERT(observer.use_count() == 3, "");

    ptrs[1] = nullptr;
    ASSERT(observer.use_count() == 2, "");

    ptrs[2] = std::make_shared<int>(*shared);
    ASSERT(observer.use_count() == 1, "");

    ptrs[0] = shared;
    ptrs[1] = shared;
    ptrs[2] = std::move(shared);
    ASSERT(observer.use_count() == 3, "");

    std::ignore = std::move(ptrs[0]);
    ptrs[1] = std::move(ptrs[1]);
    ptrs[1] = std::move(ptrs[2]);
    ASSERT(observer.use_count() == 2, "");

    shared = observer.lock();
    ASSERT(observer.use_count() == 3, "");

    shared = nullptr;
    for (auto &ptr : ptrs) ptr = nullptr;
    ASSERT(observer.use_count() == 0, "");

    shared = observer.lock();
    ASSERT(observer.use_count() == 0, "");
}

// NOTICE: shared_ptr 的循环引用问题
class A;
class B;
struct A {
    std::shared_ptr<B> ptr;
    ~A() { std::cout << "A destroyed" << std::endl; }
};
struct B {
    std::weak_ptr<A> ptr;
    ~B() { std::cout << "B destroyed" << std::endl; }
};

void TestSharedPtrCycle() {
    // TODO： 写一个测试用例，展示 shared_ptr 的循环引用问题
}

template<typename T>
class MySharedPtr {
    // TODO: 实现 shared_ptr 的基本功能
    // NOTICE: 面试时经常会问到 shared_ptr 的实现原理，建议自己独立实现一个简单的 shared_ptr。
    // 例如，构造函数、析构函数、移动构造函数、移动赋值运算符等。
	T* m_ptr; // 管理的资源指针
    size_t* m_ref_count; // 引用计数指针（动态分配，所有拷贝共享）

    // 释放资源（内部辅助函数）
    void release() {
        if (m_ref_count) {
            (*m_ref_count)--; // 引用计数-1
            if (*m_ref_count == 0) { // 计数为0，释放资源和计数
                delete m_ptr;
                delete m_ref_count;
            }
        }
        m_ptr = nullptr;
        m_ref_count = nullptr;
    }

public:
    // 构造函数：管理原始指针
    explicit MySharedPtr(T* ptr = nullptr) : m_ptr(ptr), m_ref_count(new size_t(1)) {
        if (ptr == nullptr) {
            *m_ref_count = 0; // 空指针计数为0
        }
    }

    // 拷贝构造：共享资源，计数+1
    MySharedPtr(const MySharedPtr& other) {
        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;
        if (m_ref_count) {
            (*m_ref_count)++;
        }
    }

    // 移动构造：转移所有权，原指针置空
    MySharedPtr(MySharedPtr&& other) noexcept {
        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;
        other.m_ptr = nullptr;
        other.m_ref_count = nullptr;
    }

    // 拷贝赋值
    MySharedPtr& operator=(const MySharedPtr& other) {
        if (this != &other) {
            release(); // 释放当前资源
            // 共享新资源
            m_ptr = other.m_ptr;
            m_ref_count = other.m_ref_count;
            if (m_ref_count) {
                (*m_ref_count)++;
            }
        }
        return *this;
    }

    // 移动赋值
    MySharedPtr& operator=(MySharedPtr&& other) noexcept {
        if (this != &other) {
            release(); // 释放当前资源
            // 转移所有权
            m_ptr = other.m_ptr;
            m_ref_count = other.m_ref_count;
            other.m_ptr = nullptr;
            other.m_ref_count = nullptr;
        }
        return *this;
    }

    // 析构函数
    ~MySharedPtr() {
        release();
    }

    // 获取引用计数
    size_t use_count() const {
        return m_ref_count ? *m_ref_count : 0;
    }

    // 解引用
    T& operator*() const {
        return *m_ptr;
    }

    // 箭头运算符
    T* operator->() const {
        return m_ptr;
    }

    // 重置资源
    void reset(T* ptr = nullptr) {
        release();
        m_ptr = ptr;
        m_ref_count = new size_t(1);
        if (ptr == nullptr) {
            *m_ref_count = 0;
        }
    }
};

int main(int argc, char **argv) {
    TestSharedPtr();
    TestSharedPtrCycle();

    // NOTICE: 在这里使用自己实现的 shared_ptr
    MySharedPtr<int> ptr = MySharedPtr<int>(new int(10));
    MySharedPtr<int> ptr2 = ptr;
    MySharedPtr<int> ptr3 = std::move(ptr);
    ptr = ptr2;
    ptr = std::move(ptr3);

    // TODO: 将下列 `?` 替换为正确的值
    ASSERT(ptr.use_count() == 2, "");
    ASSERT(ptr2.use_count() == 2, "");
    ASSERT(ptr3.use_count() == 0, "");
    return 0;
}
