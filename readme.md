
# ⚡ Limit Order Book Engine (HFT-Optimized)

A **high-performance, low-latency Limit Order Book (LOB) engine** designed for **high-frequency trading (HFT) systems**.
This project focuses on **efficient order matching, cache-friendly data structures, and real-time processing**.

---

## 🚀 Overview

This engine simulates a **price-time priority matching system** used in modern financial exchanges.
It is optimized for **throughput, latency, and deterministic behavior**, making it suitable for **performance-critical environments**.

---

## 🧠 Core Features

* ⚡ Ultra-fast order matching engine
* 📊 Price-Time Priority (FIFO within price levels)
* 🔄 Real-time order processing
* 📉 Efficient bid/ask book maintenance
* 🧵 Lock-free / low-latency design components (SPSC queue)
* 📈 Metrics tracking for performance analysis
* 🧪 Unit-tested matching logic

---

## 🏗 Architecture

```
                +----------------------+
                |  MarketDataFeed      |
                +----------+-----------+
                           |
                           v
                +----------------------+
                |      Engine          |
                +----------+-----------+
                           |
        +------------------+------------------+
        |                                     |
        v                                     v
+-------------------+              +-------------------+
|   OrderBook       |              |     Metrics       |
+-------------------+              +-------------------+
        |
        v
+-------------------+
|   PriceLevel      |
+-------------------+
```

---

## 📂 Project Structure

```
limit_order_book/
├── fixed_orderbook/
│   ├── include/        # Core headers (Engine, OrderBook, etc.)
│   ├── src/            # Implementation
│   ├── tests/          # Unit tests
│   ├── build/          # Build artifacts
│   ├── CMakeLists.txt  # Build config
│   ├── build.sh        # Build script
│   └── main.cpp        # Entry point
```

---

## ⚙️ Tech Stack

* **Language:** C++ (Performance-oriented)
* **Build System:** CMake
* **Design Focus:** Low latency, cache efficiency
* **Concurrency:** SPSC Queue (lock-free pattern)

---

## 🧪 How to Build & Run

### 🔨 Build

```bash
cd fixed_orderbook
chmod +x build.sh
./build.sh
```

### ▶️ Run

```bash
./orderbook
```

---

## 🧪 Run Tests

```bash
cd tests
./run_tests
```

---

## 📊 Example Output

```
BIDS            | ASKS
-------------------------------
100 @ 5         | 101 @ 3
99  @ 2         | 102 @ 4
```

---

## ⚡ Performance Goals

* Target throughput: **5,000+ ops/sec**
* Low latency matching
* O(log n) or better operations for order handling

---

## 🧠 Key Concepts Implemented

* Limit Order Book (LOB)
* Matching Engine Design
* Price-Time Priority
* Cache-efficient data structures
* Lock-free queues (SPSC)
* System-level performance optimization

---

## 🔮 Future Improvements

* 🚀 Multi-threaded matching engine
* 🌐 WebSocket live market feed
* 💾 Persistence layer (Redis / Disk logging)
* 📊 Real-time visualization dashboard
* 📉 Advanced order types (IOC, FOK, Stop orders)

---

## 🤝 Contribution

Contributions are welcome!
Feel free to open issues or submit pull requests.

---

## 📜 License

This project is open-source and available under the **MIT License**.

---

## 👨‍💻 Author

**Piyush**
Aspiring systems engineer focused on **low-latency systems & high-performance computing**

---

## 💀 Why This Project Stands Out

This is not just a basic implementation — it reflects:

* Systems-level thinking
* Performance engineering mindset
* Real-world trading system design
* Scalable architecture principles

---

⭐ If you found this useful, consider giving it a star!
