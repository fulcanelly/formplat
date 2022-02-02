#include <functional>
#include <map>
#include <memory>

template<class T, class D>
struct matcher {

    using supplier = std::function<D()>;

    std::function<bool(T, T)> predicate;
    std::map<T, supplier> data;
    std::shared_ptr<supplier> empty = nullptr;
    
    matcher(std::function<bool(T, T)> predicate): predicate(predicate) {}

    matcher& on_nokey(supplier supl) {
        empty = std::make_shared<supplier>(supl);
        return *this;
    }

    matcher& when(T data, supplier s) {
        this->data[data] = s;
        return *this;
    }

    D apply(T data_) {
        for (auto &pair_ : data) {
            if (predicate(pair_.first, data_)) {
              return pair_.second();
            } 
        }
        return (*empty)();
    }

};