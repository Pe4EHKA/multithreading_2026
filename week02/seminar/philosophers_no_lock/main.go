package main

import (
	"fmt"
	"runtime"
	"sync"
)

type Fork struct {
	id    int
	owner *Philosopher
}

type Philosopher struct {
	id                  int
	leftFork, rightFork *Fork
	eatenCount          int
}

func (p *Philosopher) think() {
	fmt.Printf("Philosopher %d is thinking...\n", p.id)
	//time.Sleep(100 * time.Millisecond)
}

func (p *Philosopher) eat() {
	fmt.Printf("Philosopher %d is eating (meal %d)\n", p.id, p.eatenCount+1)

	if (p.leftFork.owner != p) || (p.rightFork.owner != p) {
		panic("fight between philosophers!")
	}

	//time.Sleep(200 * time.Millisecond)
	p.eatenCount++
}

func (p *Philosopher) borrowForks() {
	for {
		if p.leftFork.owner != nil || p.rightFork.owner != nil {
			runtime.Gosched()
			continue
		}

		p.leftFork.owner = p
		p.rightFork.owner = p

		fmt.Printf("Philosopher %d picked up both forks\n", p.id)
		return
	}
}

func (p *Philosopher) returnForks() {
	fmt.Printf("Philosopher %d put down both forks\n", p.id)
	p.leftFork.owner = nil
	p.rightFork.owner = nil
}

func (p *Philosopher) dine(meals int, wg *sync.WaitGroup) {
	defer wg.Done()

	for i := 0; i < meals; i++ {
		p.think()
		p.borrowForks()
		p.eat()
		p.returnForks()
	}
}

func main() {
	n := 5
	meals := 1000

	fmt.Printf("N=%d meals=%d\n", n, meals)

	forks := make([]*Fork, n)
	for i := 0; i < n; i++ {
		forks[i] = &Fork{id: i, owner: nil}
	}

	philosophers := make([]*Philosopher, n)
	for i := 0; i < n; i++ {
		philosophers[i] = &Philosopher{
			id:        i + 1,
			leftFork:  forks[i],
			rightFork: forks[(i+1)%n],
		}
	}

	var wg sync.WaitGroup
	wg.Add(n)

	for i := 0; i < n; i++ {
		p := philosophers[i]
		go p.dine(meals, &wg)
	}

	wg.Wait()

	for _, p := range philosophers {
		fmt.Printf("P%d ate %d/%d\n", p.id, p.eatenCount, meals)
	}
	fmt.Println("run with RACE=1 to see data races")
}
