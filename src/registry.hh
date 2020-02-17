#pragma once

#include <unordered_map>
#include <vector>

template<typename K, typename V>
struct registry {
    std::unordered_map<decltype(K::value), V> data;

    void insert(K& k, V& v) {
        data[k.value] = std::move(v);
    }
    
    V& get(K& k) {
        return data[k.value];
    }
};

template<typename K, typename V>
struct bi_registry {
    std::unordered_map<V, K> map;
    std::vector<V> list;

    K insert(V v) {
        auto f = map.find(v);
        if (f != map.end()) {
            return f->second;
        } else {
            K id {map.size()};
            map.insert({v, id});
            list.push_back(v);
            return id;
        }
    }

    V& get(K k) {
        return list[k.value];
    }
};
