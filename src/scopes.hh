#pragma once

#include <tuple>
#include <utility>
#include <algorithm>
#include <optional>
#include <vector>
#include <functional>

template<typename ID, typename T>
class scopes {
private:
    std::vector<std::pair<ID, T>> data;
    std::vector<size_t> sizes;
public:
    scopes() {
        push_scope();
    }
    void push_item(ID id, T&& t) {
        sizes.back()++;
        data.push_back(std::make_pair(id, std::move(t)));
    }
    std::optional<std::reference_wrapper<T>> find_item_current_scope(ID id) {
        auto l = std::find_if(data.rbegin(), data.rbegin() + sizes.back(),
            [id](std::pair<ID, T> &x) -> bool {
                return x.first == id;
            }
        );
        if (l != data.rbegin() + sizes.back()) {
            return {l->second};
        } else {
            return std::nullopt;
        }
    }
    std::optional<std::reference_wrapper<T>> find_item(ID id) {
        auto l = std::find_if(data.rbegin(), data.rend(),
            [id](std::pair<ID, T> &x) -> bool {
                return x.first == id;
            }
        );
        if (l != data.rend()) {
            return {l->second};
        } else {
            return std::nullopt;
        }
    }
    void push_scope() {
        sizes.push_back({0});
    }
    void pop_scope() {
        size_t size = sizes.back();
        data.resize(data.size() - size);
        sizes.pop_back();
    }
};
