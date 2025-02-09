#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>
#include <algorithm> // For std::sort

// Order structure to store details of an order.
struct Order {
    int lotSize;
    int price;

    // Default constructor with default values for lotSize and price.
    Order() : lotSize(10), price(2) {}

    // Parameterized constructor to initialize lotSize and price.
    Order(int lotSize, int price) : lotSize(lotSize), price(price) {}
};

class ConcurrentHashMap {
public:
    // Inserts a new order for a symbol. If the price already exists, it aggregates the lotSize.
    void insert(const std::string& symbol, const Order& order) {
        std::unique_lock<std::shared_mutex> lock(getMutex(symbol)); // Lock the specific symbol's mutex

        auto& orders = map_[symbol];
        auto it = orders.find(order.price);
        if (it != orders.end()) {
            it->second.lotSize += order.lotSize; // Aggregating lotSize if price exists.
        } else {
            orders[order.price] = order; // Insert new order if price doesn't exist.
        }
    }

    // Removes all orders for the given symbol. Logs an error if symbol does not exist.
    void remove(const std::string& symbol) {
        std::unique_lock<std::shared_mutex> lock(getMutex(symbol)); // Lock the specific symbol's mutex

        if (map_.erase(symbol) == 0) {
            std::cerr << "Error: Symbol " << symbol << " not found for removal." << std::endl;
        } else {
            std::cout << "Removed orders for symbol " << symbol << std::endl;
        }
    }

    // Displays all orders in the map.
    void display() const {
        std::shared_lock<std::shared_mutex> lock(displayMutex_); // Lock to ensure thread-safe display

        for (const auto& pair : map_) {
            std::cout << pair.first << ": ";
            for (const auto& order : pair.second) {
                std::cout << "{lotSize: " << order.second.lotSize << ", price: " << order.second.price << "} ";
            }
            std::cout << std::endl;
        }
    }

private:
    mutable std::unordered_map<std::string, std::unordered_map<int, Order>> map_; // map for symbol -> price -> order
    mutable std::unordered_map<std::string, std::shared_mutex> mutexMap_; // Mutex map for each symbol
    mutable std::shared_mutex displayMutex_; // Shared mutex for displaying orders

    // Returns a reference to the mutex associated with a particular symbol.
    std::shared_mutex& getMutex(const std::string& symbol) const {
        return mutexMap_[symbol]; // Ensure each symbol has its own mutex
    }
};

int main() {
    ConcurrentHashMap concurrentMap;

    // Sample symbols
    std::vector<std::string> symbols = {
        "NESTLEIND", "HDFCBANK", "RELIANCE", "TCS", "INFY",
        "SBIN", "ICICIBANK", "LT", "BAJFINANCE", "HINDUNILVR"
    };

    // Insert initial orders for all symbols.
    for (const auto& symbol : symbols) {
        concurrentMap.insert(symbol, Order(10, 2));
    }

    // Test adding to an existing order and adding new order
    concurrentMap.insert("NESTLEIND", Order(20, 2)); // This should aggregate the lotSize
    concurrentMap.insert("HDFCBANK", Order(15, 4));  // This should add a new order with price 4

    // Display current orders after insertions
    concurrentMap.display();

    // Test removal of orders
    concurrentMap.remove("NESTLEIND"); // Should remove all orders for NESTLEIND

    // Display after removal to confirm the symbol was removed
    concurrentMap.display();

    // Attempting to remove a non-existent symbol (Error will be logged)
    concurrentMap.remove("NONEXISTENT");

    return 0;
}
