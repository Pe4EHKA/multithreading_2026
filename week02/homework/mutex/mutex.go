package mutex

import "sync/atomic"

const (
	mutexUnlocked int32 = 0
	mutexLocked   int32 = 1
)

type Mutex struct {
	waitingRegister *FutexLike
	waitingCounter  atomic.Int32
}

func NewMutex() *Mutex {
	return &Mutex{waitingRegister: newFutexLike()}
}

func (m *Mutex) Lock() {
	if m.waitingRegister.Exchange(mutexLocked) == mutexUnlocked {
		return
	}
	m.lockSlow()
}

func (m *Mutex) lockSlow() {
	m.waitingCounter.Add(1)
	for m.waitingRegister.Exchange(mutexLocked) != mutexUnlocked {
		m.waitingRegister.Park(mutexLocked)
	}

	m.waitingCounter.Add(-1)
}

func (m *Mutex) Unlock() {
	if m.waitingRegister.Exchange(mutexUnlocked) != mutexLocked {
		panic("Mutex: unlock of unlocked mutex")
	}
	if m.waitingCounter.Load() > 0 {
		m.waitingRegister.Wake()
	}
}
