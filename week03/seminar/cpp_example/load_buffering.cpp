// Compile: g++ -O2 -std=c++20 -pthread -o load_buffering load_buffering.cpp

#include <iostream>
#include <thread>
#include <atomic>

constexpr int ITERATIONS = 2'000'000;

// LB on ARM

static std::atomic<int> x_relaxed{0}, y_relaxed{0};
static int r1_relaxed, r2_relaxed;
static int lb_relaxed = 0;

static void test_relaxed() {
    for (int i = 0; i < ITERATIONS; i++) {
        x_relaxed.store(0, std::memory_order_relaxed);
        y_relaxed.store(0, std::memory_order_relaxed);

        std::thread t1([&]() {
            r1_relaxed = x_relaxed.load(std::memory_order_relaxed); // load x
            y_relaxed.store(1, std::memory_order_relaxed);           // store y
        });

        std::thread t2([&]() {
            r2_relaxed = y_relaxed.load(std::memory_order_relaxed); // load y
            x_relaxed.store(1, std::memory_order_relaxed);           // store x
        });

        t1.join();
        t2.join();

        if (r1_relaxed == 1 && r2_relaxed == 1) {
            lb_relaxed++;
        }

        // std::cout << "iteration: " << i << "/" << ITERATIONS << std::endl;
    }
}

// no LB everywhere

static std::atomic<int> x_sc{0}, y_sc{0};
static int r1_sc, r2_sc;
static int lb_sc = 0;

static void test_seqcst() {
    for (int i = 0; i < ITERATIONS; i++) {
        x_sc.store(0);
        y_sc.store(0);

        std::thread t1([&]() {
            r1_sc = x_sc.load(); // seq_cst load: cannot reorder past subsequent store
            y_sc.store(1);
        });

        std::thread t2([&]() {
            r2_sc = y_sc.load();
            x_sc.store(1);
        });

        t1.join();
        t2.join();

        if (r1_sc == 1 && r2_sc == 1) {
            lb_sc++;
        }

        // std::cout << "iteration: " << i << "/" << ITERATIONS << std::endl;
    }
}

int main() {
    test_relaxed();
    std::cout << "[relaxed]  r1==1 && r2==1: " << lb_relaxed
              << " / " << ITERATIONS
              << "  (x86: always 0; ARM: may be > 0)\n";

    test_seqcst();
    std::cout << "[seq_cst]  r1==1 && r2==1: " << lb_sc
              << " / " << ITERATIONS
              << "  (always 0 on any architecture)\n";
    return 0;
}
