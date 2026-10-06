#include <iostream>
#include <list>
#include <unordered_map>
#include <chrono>
#include <set>
#include <vector>
#include <fstream>

typedef std::list<std::pair<int, int> > List;         // двусвязный список пар (ключ, значение)
typedef std::unordered_map<int, List::iterator> Map;  // ключ -> узел списка

class Cache {
public:
    Cache(int capacity) : capacity_(capacity) {}
    virtual ~Cache() {}
    virtual int get(int key) = 0;
    virtual void put(int key, int value) = 0;
    virtual bool remove(int key) = 0;

    void print() const {
        for (List::const_iterator it = items_.begin(); it != items_.end(); ++it)
            std::cout << "[" << it->first << ":" << it->second << "] ";
        std::cout << "\n";
    }

protected:
    int capacity_;
    List items_;
    Map map_;
};

class LRUCache : public Cache {
public:
    LRUCache(int capacity) : Cache(capacity) {}

    // Значение по ключу или -1, если ключа нет
    int get(int key) override{
        Map::iterator it = map_.find(key);
        if (it == map_.end()) return -1;
        items_.splice(items_.begin(), items_, it->second);  // LRU // without: FIFO
        return it->second->second;
    }

    void put(int key, int value) override{
        Map::iterator it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if ((int)items_.size() == capacity_) {   // кэш полон -> удаляем самый старый
            map_.erase(items_.back().first);
            items_.pop_back();
        }
        items_.push_front(std::make_pair(key, value));
        map_[key] = items_.begin();
    }

    bool remove(int key) override {
        Map::iterator it = map_.find(key);
        if (it == map_.end()) return false; // Не нашли

        items_.erase(it->second); // Удаляем из двусвязного списка за O(1)
        map_.erase(it);           // Удаляем из хэш-таблицы за O(1)
        return true;
    }

};

class LFUCache : public Cache {
public:
    LFUCache(int capacity) : Cache(capacity) {}

    int get(int key) override {
        Map::iterator it = map_.find(key);
        if (it == map_.end()) return -1;

        frequencies_[key]++;
        update_position(it->second);

        return it->second->second;
    }

    void put(int key, int value) override {
        if (capacity_ <= 0) return;

        Map::iterator it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            frequencies_[key]++;
            update_position(it->second);
            return;
        }

        if ((int)items_.size() == capacity_) {
            int lfu_key = items_.back().first;
            map_.erase(lfu_key);
            frequencies_.erase(lfu_key);
            items_.pop_back();
        }

        items_.push_front(std::make_pair(key, value));
        map_[key] = items_.begin();
        frequencies_[key] = 1;
        update_position(items_.begin());
    }

    bool remove(int key) override {
        Map::iterator it = map_.find(key);
        if (it == map_.end()) return false;

        items_.erase(it->second);
        map_.erase(it);
        frequencies_.erase(key);
        return true;
    }

private:
    std::unordered_map<int, int> frequencies_;

    void update_position(List::iterator curr_it) {
        int curr_key = curr_it->first;
        int curr_freq = frequencies_[curr_key];

        for (List::iterator it = items_.begin(); it != items_.end(); ++it) {
            if (frequencies_[it->first] < curr_freq) {
                items_.splice(it, items_, curr_it);
                return;
            }
        }
    }
};

class FIFOCache : public Cache {
public:
    FIFOCache(int capacity) : Cache(capacity) {}

    // Значение по ключу или -1, если ключа нет
    int get(int key) override{
        Map::iterator it = map_.find(key);
        if (it == map_.end()) return -1;
        return it->second->second;
    }

    void put(int key, int value) override{
        Map::iterator it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if ((int)items_.size() == capacity_) {   // кэш полон -> удаляем самый старый
            map_.erase(items_.back().first);
            items_.pop_back();
        }
        items_.push_front(std::make_pair(key, value));
        map_[key] = items_.begin();
    }

    bool remove(int key) override {
        Map::iterator it = map_.find(key);
        if (it == map_.end()) return false; // Не нашли

        items_.erase(it->second); // Удаляем из двусвязного списка за O(1)
        map_.erase(it);           // Удаляем из хэш-таблицы за O(1)
        return true;
    }

};

class TwoQ_Cache : public Cache {
public:
    TwoQ_Cache(int capacity, int FIFO = -1, int LRU = -1) : Cache(capacity), FIFOPart(FIFO == -1 ? std::max(1, int(capacity * 0.2)) : FIFO), LRUPart(LRU == -1 ? std::max(1, int(capacity * 0.8)) : LRU) {}

    int get(int key) override{
        int value = FIFOPart.get(key);
        if (value != -1) {
            LRUPart.put(key, value);
            FIFOPart.remove(key);
            return value;
        }
        value = LRUPart.get(key);
        if (value != -1) {
            return value;
        }
        return -1;

    }

    void put(int key, int value) override{
        if (LRUPart.get(key) != -1) {
            LRUPart.put(key, value);
            return;
        }
        if (FIFOPart.get(key) != -1) {
            FIFOPart.put(key, value);
            return;
        }
        FIFOPart.put(key, value);
    }

    bool remove(int key) override {
        return (LRUPart.remove(key) || FIFOPart.remove(key)) == true ? true : false ;
    }

    private:
        FIFOCache FIFOPart;
        LRUCache LRUPart;
};

int fib_cache(int n, Cache& cache){
    if (n==1 or n==2){
        return 1;
    }
    int cached = cache.get(n);
    if(cached != -1){
        return cached;
    }
    int value = fib_cache(n-1, cache) + fib_cache(n-2, cache);
    cache.put(n, value);
    //cache.print();
    return value;
};

int fib(int n){
    if (n==1 or n==2){
        return 1;
    }
    return fib(n-1) + fib(n-2);
};

bool readData(const char* filename, std::vector<int>& data) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Не удалось открыть файл: " << filename << "\n";
        return false;
    }

    int n;
    if (!(in >> n) || n < 0) {
        std::cerr << "В начале файла должно быть количество данных\n";
        return false;
    }

    data.clear();
    for (int i = 0; i < n; i++) {
        int x;
        if (!(in >> x)) {
            std::cerr << "Ожидалось " << n << " чисел, а в файле только " << i << "\n";
            return false;
        }
        data.push_back(x);
    }
    return true;
}

int main(int argc, char** argv) {

    std::vector<int> data;
    readData(argv[1], data);
    std::set<int> s(data.begin(), data.end());
    std::cout << "Идеальный кэш: " << data.size() - s.size()<< "\n";

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::cout<<fib(40)<<" ";
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "without cache: Время: " << ms << " мс\n";

    for(int i = 1; i < 15; i++){
        std::cout << i << " ";



        FIFOCache cache_FIFO(i);
        start = std::chrono::steady_clock::now();
        std::cout<<fib_cache(20, cache_FIFO)<<" ";
        end = std::chrono::steady_clock::now();
        ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "FIFO: " << ms << " мс ";

        LRUCache cache_LRU(i);
        start = std::chrono::steady_clock::now();
        std::cout<<fib_cache(20, cache_LRU)<<" ";
        end = std::chrono::steady_clock::now();
        ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "LRU: " << ms << " мс ";

        TwoQ_Cache cache_2Q(i);
        start = std::chrono::steady_clock::now();
        std::cout<<fib_cache(20, cache_2Q)<<" ";
        end = std::chrono::steady_clock::now();
        ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "2Q: " << ms << " мс ";

        LFUCache cache_LFU(i);
        start = std::chrono::steady_clock::now();
        std::cout<<fib_cache(20, cache_LFU)<<" ";
        end = std::chrono::steady_clock::now();
        ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "LFU: " << ms << " мс\n";

    }
    return 0;
}
