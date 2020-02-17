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
