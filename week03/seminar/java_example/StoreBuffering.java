public class StoreBuffering {

    static int x = 0;
    static int y = 0;
    static int r1, r2;

    public static void main(String[] args) throws InterruptedException {
        int reorderCount = 0;

        for (int i = 0; i < 1_000_000; i++) {
            // Reset shared variables
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

            // If both r1 == 0 and r2 == 0, it means:
            // t1 read y BEFORE t2's write to y was visible
            // t2 read x BEFORE t1's write to x was visible
            // This is the store buffering effect!
            if (r1 == 0 && r2 == 0) {
                reorderCount++;
            }
        }

        System.out.println("Store buffering observed: " + reorderCount + " times");
    }
}