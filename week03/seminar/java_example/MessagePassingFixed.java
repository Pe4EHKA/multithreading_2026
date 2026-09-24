// Compile & run: javac MessagePassingFixed.java && java MessagePassingFixed
//
// Message Passing fixed with volatile.

public class MessagePassingFixed {

    static volatile int     data = 0;
    static volatile boolean flag = false;

    public static void main(String[] args) throws InterruptedException {

        Thread consumer = new Thread(() -> {
            while (!flag) { /* spin — volatile read prevents JIT from hoisting */ }
            
            System.out.println("Consumer: data = " + data);
        });

        Thread producer = new Thread(() -> {
            data = 42;
            flag = true;
        });

        consumer.start();
        Thread.sleep(50);
        producer.start();

        consumer.join();
        producer.join();
    }
}
