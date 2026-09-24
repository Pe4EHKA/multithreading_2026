// Compile & run: javac StoreBufferingFixed.java && java StoreBufferingFixed
//
// volatile in Java guarantees:
//   - Compiler won't reorder across volatile accesses
//   - JVM inserts StoreLoad barrier after a volatile write
//     (equivalent to MFENCE on x86 — flushes the store buffer)
//
// r1==0 && r2==0 is impossible with volatile.

public class StoreBufferingFixed {

    static volatile int x = 0;
    static volatile int y = 0;

    static int r1, r2;  // why not volitile?

    public static void main(String[] args) throws InterruptedException {
        int reorderCount = 0;

        for (int i = 0; i < 500_000; i++) {
            x = 0;
            y = 0;

            Thread t1 = new Thread(() -> {
                x = 1;
                r1 = y;
            });

            Thread t2 = new Thread(() -> {
                y = 1;
                r2 = x;
            });

            t1.start();
            t2.start();
            t1.join();
            t2.join();

            if (r1 == 0 && r2 == 0) {
                reorderCount++;
            }
        }

        System.out.println("Store buffering observed: " + reorderCount + " times");
        System.out.println("With volatile, result should always be 0");
    }
}
