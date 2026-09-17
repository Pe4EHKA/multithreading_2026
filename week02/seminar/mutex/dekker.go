package mutex

import (
	"runtime"
	"sync/atomic"
)

type DekkerLock struct {
	wants [2]atomic.Int32 // worker who wants to get lock
	turn  atomic.Int32    // which turn in case of conflict
}

func (l *DekkerLock) Lock(id int) {
	other := 1 - id

	// we want to lock
	l.wants[id].Store(1)

	// conflict
	for l.wants[other].Load() == 1 {
		if l.turn.Load() != int32(id) {
			l.wants[id].Store(0)
			for l.turn.Load() != int32(id) {
				// yield
				runtime.Goexit()
			}
			l.wants[id].Store(1)
		}
	}
}

func (l *DekkerLock) Unlock(id int) {
	other := 1 - id

	l.wants[id].Store(0)
	l.turn.Store(int32(other))
}
