// Compile & run: javac MessagePassingBroken.java && java -server MessagePassingBroken
//
// Message Passing test — broken without visibility guarantees.
//
//   Producer               Consumer
//   --------               --------
//   data = 42              while (!flag) {}
//   flag = true            use(data)
//
// The JIT is free to:
//   1. Cache `flag` in a register and consumer spins forever.
//   2. Reorder the two writes and consumer sees flag=true but data=0

public class MessagePassingBroken {

    static int     data = 0;
    static boolean flag = false;  // NOT volatile

    public static void main(String[] args) throws InterruptedException {

        Thread consumer = new Thread(() -> {
            // The JIT may transform this into:
            //   if (!flag) { while (true) {} }

            while (!flag) { /* spin */ }
            System.out.println("Consumer: data = " + data);  // may print 0
        });

        Thread producer = new Thread(() -> {
            data = 42;
            flag = true;
        });

        consumer.start();
        Thread.sleep(50);
        producer.start();

        consumer.join(3_000);
        producer.join();

        if (consumer.isAlive()) {
            System.out.println("Consumer is stuck.");
            consumer.interrupt();
        }
    }
}
