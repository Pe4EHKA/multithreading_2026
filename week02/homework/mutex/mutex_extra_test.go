package mutex

import (
	"runtime"
	"sync"
	"testing"
	"time"
)

var _ sync.Locker = (*Mutex)(nil)

func TestMutexFastPath(t *testing.T) {
	m := NewMutex()
	for i := 0; i < 1000; i++ {
		m.Lock()
		if got := m.waitingRegister.Load(); got != mutexLocked {
			t.Fatalf("state after Lock = %d, want %d", got, mutexLocked)
		}
		if got := m.waitingCounter.Load(); got != 0 {
			t.Fatalf("waiters on uncontended path = %d, want 0", got)
		}
		m.Unlock()
		if got := m.waitingRegister.Load(); got != mutexUnlocked {
			t.Fatalf("state after Unlock = %d, want %d", got, mutexUnlocked)
		}
		if len(m.waitingRegister.ch) != 0 {
			t.Fatal("uncontended Unlock left a wake notification")
		}
	}
}

func TestMutexRegisteredWaiters(t *testing.T) {
	const workers = 32
	for round := 0; round < 10; round++ {
		m := NewMutex()
		m.Lock()
		var wg sync.WaitGroup
		counter := 0
		wg.Add(workers)
		for i := 0; i < workers; i++ {
			go func() {
				defer wg.Done()
				m.Lock()
				counter++
				m.Unlock()
			}()
		}
		done := make(chan struct{})
		go func() {
			wg.Wait()
			close(done)
		}()
		deadline := time.Now().Add(5 * time.Second)
		for m.waitingCounter.Load() != workers {
			if time.Now().After(deadline) {
				m.Unlock()
				t.Fatal("workers did not register in the slow path")
			}
			runtime.Gosched()
		}
		m.Unlock()
		select {
		case <-done:
		case <-time.After(5 * time.Second):
			t.Fatal("registered workers did not finish")
		}
		if counter != workers {
			t.Fatalf("counter = %d, want %d", counter, workers)
		}
		if got := m.waitingCounter.Load(); got != 0 {
			t.Fatalf("waiters after completion = %d, want 0", got)
		}
	}
}

func TestMutexUnlockUnlockedPanics(t *testing.T) {
	defer func() {
		if recover() == nil {
			t.Fatal("Unlock of an unlocked mutex did not panic")
		}
	}()
	NewMutex().Unlock()
}

func TestMutexCrossGoroutineUnlock(t *testing.T) {
	m := NewMutex()
	m.Lock()
	done := make(chan struct{})
	go func() {
		m.Unlock()
		close(done)
	}()
	select {
	case <-done:
	case <-time.After(5 * time.Second):
		t.Fatal("Unlock in another goroutine did not finish")
	}
	m.Lock()
	m.Unlock()
}
