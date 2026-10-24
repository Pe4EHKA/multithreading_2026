package mutex

import (
	"fmt"
	"runtime"
	"sort"
	"sync"
	"sync/atomic"
	"testing"
	"time"
)

var benchmarkSink atomic.Uint64
var benchmarkLockerSink sync.Locker

var benchmarkImplementations = []struct {
	name string
	new  func() sync.Locker
}{
	{name: "custom", new: func() sync.Locker { return NewMutex() }},
	{name: "sync", new: func() sync.Locker { return &sync.Mutex{} }},
}

func BenchmarkMutexUncontended(b *testing.B) {
	b.Run("impl=custom", func(b *testing.B) {
		m := NewMutex()
		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			m.Lock()
			m.Unlock()
		}
	})
	b.Run("impl=sync", func(b *testing.B) {
		var m sync.Mutex
		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			m.Lock()
			m.Unlock()
		}
	})
}

func BenchmarkMutexWorkload(b *testing.B) {
	cases := []struct {
		name        string
		inside      int
		outside     int
		parallelism int
		yield       bool
		hold        time.Duration
	}{
		{name: "Counter", parallelism: 1},
		{name: "Inside64", inside: 64, parallelism: 1},
		{name: "Inside1024", inside: 1024, parallelism: 1},
		{name: "Outside1024", inside: 8, outside: 1024, parallelism: 1},
		{name: "Oversubscribed", parallelism: 8},
		{name: "YieldInCS", parallelism: 4, yield: true},
		{name: "Sleep100usInCS", parallelism: 4, hold: 100 * time.Microsecond},
	}
	for _, tc := range cases {
		for _, impl := range benchmarkImplementations {
			b.Run(fmt.Sprintf("case=%s/impl=%s", tc.name, impl.name), func(b *testing.B) {
				m := impl.new()
				var operations int
				value := uint64(1)
				b.SetParallelism(tc.parallelism)
				b.ReportAllocs()
				b.ResetTimer()
				b.RunParallel(func(pb *testing.PB) {
					local := uint64(1)
					for pb.Next() {
						for j := 0; j < tc.outside; j++ {
							local = local*1664525 + 1013904223
						}
						m.Lock()
						operations++
						for j := 0; j < tc.inside; j++ {
							value = value*1664525 + 1013904223
						}
						if tc.yield {
							runtime.Gosched()
						}
						if tc.hold > 0 {
							time.Sleep(tc.hold)
						}
						m.Unlock()
					}
					benchmarkSink.Add(local)
				})
				b.StopTimer()
				if operations != b.N {
					b.Fatalf("operations = %d, want %d", operations, b.N)
				}
				benchmarkSink.Store(value)
			})
		}
	}
}

func BenchmarkMutexWaitLatency(b *testing.B) {
	for _, impl := range benchmarkImplementations {
		b.Run("impl="+impl.name, func(b *testing.B) {
			m := impl.new()
			workers := 8 * runtime.GOMAXPROCS(0)
			samples := make([]int64, b.N)
			start := make(chan struct{})
			var wg sync.WaitGroup
			var operations int
			wg.Add(workers)
			for worker := 0; worker < workers; worker++ {
				go func(id int) {
					defer wg.Done()
					<-start
					for i := id; i < b.N; i += workers {
						before := time.Now()
						m.Lock()
						wait := time.Since(before).Nanoseconds()
						operations++
						m.Unlock()
						samples[i] = wait
					}
				}(worker)
			}
			b.ReportAllocs()
			b.ResetTimer()
			close(start)
			wg.Wait()
			b.StopTimer()
			if operations != b.N {
				b.Fatalf("operations = %d, want %d", operations, b.N)
			}
			sort.Slice(samples, func(i, j int) bool { return samples[i] < samples[j] })
			b.ReportMetric(float64(samples[(len(samples)-1)*50/100]), "p50-wait-ns")
			b.ReportMetric(float64(samples[(len(samples)-1)*99/100]), "p99-wait-ns")
			b.ReportMetric(float64(samples[len(samples)-1]), "max-wait-ns")
		})
	}
}

func BenchmarkMutexConstructor(b *testing.B) {
	for _, impl := range benchmarkImplementations {
		b.Run("impl="+impl.name, func(b *testing.B) {
			b.ReportAllocs()
			for i := 0; i < b.N; i++ {
				benchmarkLockerSink = impl.new()
			}
		})
	}
}
