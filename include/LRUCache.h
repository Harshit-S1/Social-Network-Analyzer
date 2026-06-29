#ifndef LRU_CACHE_H
#define LRU_CACHE_H
#include <unordered_map>
#include <list>
using namespace std;

// LRU (Least Recently Used) cache to keep our memory footprint small
// Automatically kicks out the oldest, least-used data when it gets full
template <typename K, typename V>
class LRUCache {
    size_t capacity; // number of items we can hold
    
    // the actual data queue, the newly used stuff goes at the front, old stuff falls to the back
    list<pair<K, V>> items; 
    
    // fast lookup table so we don't have to scan the list, maps the key to its exact spot in the list
    unordered_map<K, typename list<pair<K, V>>::iterator> cacheMap;

public:
    LRUCache(size_t cap) : capacity(cap) {}

    // trying to grab an item from the cache
    bool get(const K& key, V& value) {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) return false; // cache miss
        
        // cache hit, moving this item to the very front of the line so it doesn't get evicted
        items.splice(items.begin(), items, it->second); 
        value = it->second->second;
        return true;
    }

    // adding a new item or updating an existing one
    void put(const K& key, const V& value) {
        auto it = cacheMap.find(key);
        
        // If its already in the cache, just have to update the value and push it to the front
        if (it != cacheMap.end()) {
            it->second->second = value; 
            items.splice(items.begin(), items, it->second); 
            return;
        }

        // new item, throw it at the front of the list and record its location
        items.push_front({key, value});
        cacheMap[key] = items.begin();

        // If out of space, push out the oldest item (the one at the very back of the list)
        if (cacheMap.size() > capacity) {
            cacheMap.erase(items.back().first);
            items.pop_back();
        }
    }

    // wipes the entire cache clean
    void clear() {
        items.clear();
        cacheMap.clear();
    }
};

#endif 