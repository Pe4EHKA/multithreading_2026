package mutex

import (
	"fmt"
	"sync"
	"testing"
	"time"
)

var busySink uint64

func busyWork(d time.Duration) {
	deadline := time.Now().Add(d)
	var x uint64
	for time.Now().Before(deadline) {
		x = x*1664525 + 1013904223
	}
	busySink = x

}

func TestMutexIsLocker(t *testing.T) {
	for _, workers := range []int{1, 2, 10, 100, 1000} {
		t.Run(fmt.Sprintf("workers=%d", workers), func(t *testing.T) {
			counter := 0
			const iters = 1000
			mutex := NewMutex()
			wg := &sync.WaitGroup{}
			wg.Add(workers)

			for i := 0; i < workers; i++ {
				go func() {
					defer wg.Done()
					for j := 0; j < iters; j++ {
						mutex.Lock()
						counter++
						mutex.Unlock()
					}
				}()
			}

			wg.Wait()

			expected := workers * iters
			if counter != expected {
				t.Errorf("counter should be %d, but got %d, mutex is not mutual", expected, counter)
			}
		})
	}
}

func TestMutexHasLiveness(t *testing.T) {
	for _, workers := range []int{1, 2, 10, 100} {
		t.Run(fmt.Sprintf("workers=%d", workers), func(t *testing.T) {
			mutex := NewMutex()
			wg := &sync.WaitGroup{}
			wg.Add(workers)

			for i := 0; i < workers; i++ {
				go func() {
					defer wg.Done()
					mutex.Lock()
					busyWork(time.Second)
					mutex.Unlock()
				}()
			}

			done := make(chan struct{})
			go func() {
				wg.Wait()
				close(done)
			}()

			select {
			case <-done:
				// all goroutines acquired the mutex — liveness holds
			case <-time.After(time.Duration(workers)*time.Second + time.Microsecond*time.Duration(10)):
				t.Errorf("liveness violation: %d workers did not all acquire the mutex within timeout", workers)
			}
		})
	}
}
