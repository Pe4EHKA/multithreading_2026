package mutex

import "sync/atomic"

type Mutex struct {
	waitingRegister *FutexLike
	waitingCounter  atomic.Int32
}

func NewMutex() *Mutex {
	// TODO: implement me
	return nil
}

func (m *Mutex) Lock() {
	// TODO: implement me
}

func (m *Mutex) Unlock() {
	// TODO: implement me
}
