package mutex

import (
	"sync/atomic"
)

type PetersonLock struct {
	wants [2]atomic.Int32 // worker who wants to get lock
	turn  atomic.Int32    // which turn in case of conflict
}

func (l *PetersonLock) Lock(id int) {
	other := 1 - id
	l.wants[id].Store(1)
	l.turn.Store(int32(other))

	for l.wants[other].Load() == 1 && l.turn.Load() == int32(other) {
		//runtime.Gosched()
	}

}

func (l *PetersonLock) Unlock(id int) {
	l.wants[id].Store(0)
}
