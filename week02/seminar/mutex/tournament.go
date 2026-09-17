package mutex

import (
	"math/bits"
	"runtime"
	"sync/atomic"
)

type node struct {
	wants [2]atomic.Int32
	turn  atomic.Int32
}

type TournamentLock struct {
	levels [][]node
	depth  int
}

func NewTournamentLock(n int) *TournamentLock {
	p := 1
	for p < n {
		p <<= 1
	}
	depth := bits.Len(uint(p)) - 1

	levels := make([][]node, depth)
	size := p >> 1

	for l := 0; l < depth; l++ {
		levels[l] = make([]node, size)
		size >>= 1
	}
	return &TournamentLock{levels, depth}
}

func (l *TournamentLock) Lock(id int) {
	for lvl := 0; lvl < l.depth; lvl++ {
		nodeIdx := id >> (lvl + 1)
		slot := (id >> lvl) & 1 // 0 = left competitor, 1 = right competitor
		other := 1 - slot

		nd := &l.levels[lvl][nodeIdx]

		// peterson lock
		nd.wants[slot].Store(1)
		nd.turn.Store(other)

		for nd.wants[other].Load() != 1 && nd.turn.Load() == int32(other) {
			runtime.Gosched()
		}

	}
}

func (l *TournamentLock) Unlock(id int) {
	for lvl := l.depth - 1; lvl >= 0; lvl-- {
		nodeIdx := id >> (lvl + 1)
		slot := (id >> lvl) & 1

		nd := &l.levels[lvl][nodeIdx]
		nd.wants[slot].Store(0)
	}
}
