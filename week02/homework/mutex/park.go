package mutex

import "sync/atomic"

type FutexLike struct {
	register atomic.Int32
	ch       chan struct{}
}

func newFutexLike() *FutexLike {
	return &FutexLike{ch: make(chan struct{}, 1)}
}

func (r *FutexLike) Park(val int32) {
	if r.register.Load() != val {
		return
	}
	<-r.ch
}

func (r *FutexLike) Wake() {
	select {
	case r.ch <- struct{}{}:
	default:
	}
}

func (r *FutexLike) Store(val int32) {
	r.register.Store(val)
}

func (r *FutexLike) Load() int32 {
	return r.register.Load()
}

func (r *FutexLike) Exchange(val int32) int32 {
	return r.register.Swap(val)
}
