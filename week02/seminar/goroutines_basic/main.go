package main

import (
	"fmt"
	"sync"
)

func main() {
	n := 4
	steps := 1_000_000

	var counter int
	var wg sync.WaitGroup
	mutex := sync.Mutex{}

	wg.Add(n)
	for i := 0; i < n; i++ {
		go func() {
			defer wg.Done()
			mutex.Lock()
			for j := 0; j < steps; j++ {
				counter++
			}
			mutex.Unlock()
		}()
	}

	wg.Wait()
	fmt.Println("Expected:", steps*n, " actual:", counter)
}
