#ifndef SEAOTTER_CANARYCOUNTER_H
#define SEAOTTER_CANARYCOUNTER_H

#include <algorithm>
#include <functional>
#include <list>
#include <mutex>
#include <vector>

template <typename K, typename V, typename Hash>
class CanaryCounter {
public:
    using bucketValue = std::pair<K, V>;
    using bucketIter = typename std::list<bucketValue>::iterator;
    using updateFunc = std::function<void(V &, const V &)>;

    explicit CanaryCounter(unsigned buckets_num, const Hash &hash = Hash())
        : _buckets(buckets_num)
        , _hash(hash) {}

    CanaryCounter(const CanaryCounter &) = delete;

    CanaryCounter &operator=(const CanaryCounter &) = delete;

public:
    void AddOrUpdate(const K &key, const V &value, updateFunc func) {
        const auto idx = _hash(key) % _buckets.size();
        _buckets[idx].AddOrUpdate(key, value, func);
    }

    std::list<bucketValue> GetAndClear() {
        std::list<bucketValue> res;
        for (unsigned idx = 0; idx < _buckets.size(); ++idx) {
            auto data = _buckets[idx].GetAndClear();
            res.splice(res.end(), data, data.begin(), data.end());
        }
        return res;
    }

private:
    class bucket {
    public:
        void AddOrUpdate(const K &key, const V &value, updateFunc func) {
            std::lock_guard<std::mutex> lock(_mtx);
            const bucketIter iter =
                std::find_if(_data.begin(), _data.end(), [&](const bucketValue &item) { return item.first == key; });
            if (iter == _data.end()) {
                _data.emplace_back(key, value);
            }
            func(iter->second, value);
        }

        std::list<bucketValue> GetAndClear() {
            std::lock_guard<std::mutex> lock(_mtx);
            std::list<bucketValue> res = std::move(_data);
            return res;
        }

    private:
        std::list<bucketValue> _data;
        mutable std::mutex _mtx;
    };

private:
    std::vector<bucket> _buckets;
    Hash _hash;
};

#endif // SEAOTTER_CANARYCOUNTER_H
