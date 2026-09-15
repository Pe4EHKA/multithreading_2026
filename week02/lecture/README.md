# Взаимное исключение

## Data race vs Race condition

### Data race

Каждый язык программирования определяет свою **модель памяти** — спецификацию того, какие значения
вправе наблюдать читающий поток при конкурентном доступе к памяти. Позднее будет показано, что
операции записи и чтения на уровне процессора не атомарны: они разбиваются на несколько шагов,
и без явных гарантий результат может зависеть от планировщика. Модель памяти ограждает
программиста от необходимости думать об этих деталях — но ценой того, что программы,
нарушающие её правила, выводятся за рамки спецификации и могут вести себя произвольно.

Одна из таких ситуаций — **data race**: конкурентный доступ к одной ячейке памяти, где хотя бы
один из доступов является записью, без явной синхронизации. Модель памяти Go определяет её так:

```
A data race is defined as a write to a memory location happening concurrently with another
read or write to that same location, unless all the accesses involved are atomic data accesses
as provided by the sync/atomic package.
As noted already, programmers are strongly encouraged to use appropriate synchronization
to avoid data races.
```

Аналогичная зависимость от порядка выполнения может возникать и на уровне алгоритмов и структур
данных — не в отдельной инструкции, а между несколькими операциями. Такая ситуация называется
**race condition**.

Строго говоря, data race и race condition — два пересекающихся, но не вложенных множества:
data race не обязательно является race condition (поведение может быть неопределённым, но логика
верной), и наоборот. Тезис «data race — частный случай race condition» формально ошибочен.
```go
package main

import (
    "fmt"
    "sync"
)

func main() {
    var counter int
    var wg sync.WaitGroup

    for i := 0; i < 1000; i++ {
        wg.Add(1)
        go func() {
            defer wg.Done()
            counter++ // DATA RACE: read-modify-write не атомарен
        }()
    }

    wg.Wait()
    fmt.Println(counter) // непредсказуемый результат, не 1000
}
```

```go
// Race condition без data race:
// оба читают флаг атомарно, но логика всё равно ломается
package main

import (
    "fmt"
    "sync"
    "sync/atomic"
)

var initialized atomic.Bool

func maybeInit(wg *sync.WaitGroup) {
    defer wg.Done()
    if !initialized.Load() {
        // между Load и Store есть возможность для модификации
        initialized.Store(true)
        fmt.Println("инициализация")
    }
}

func main() {
    var wg sync.WaitGroup
    wg.Add(2)
    go maybeInit(&wg)
    go maybeInit(&wg)
    wg.Wait()
    // "инициализация" может быть напечатана дважды
}
```

## Mutex

### Мотивация

Data race и race condition показывают, что операция `counter++` небезопасна сама по себе:
между чтением и записью планировщик может переключить контекст, и оба потока запишут
результат поверх работы друг друга. Нужен механизм, гарантирующий **взаимное исключение**
(mutual exclusion): в каждый момент времени критическую секцию выполняет не более одного
воркера.

Такой механизм — **мьютекс** (mutex, от *mutual exclusion*). Воркер, не захвативший мьютекс,
не выполняет критическую секцию, а ждёт его освобождения.

### Go vs Java: как устроен мьютекс изнутри

**Java** (`synchronized`, `ReentrantLock`) работает поверх потоков. При
конкуренции JVM использует системный вызов **futex** (*fast userspace mutex*): попытка
захвата делается атомарной инструкцией в userspace, и только при неудаче поток уходит
в ядро (`futex(FUTEX_WAIT)`) — это дорогой syscall с переключением контекста.

**Go** (`sync.Mutex`) работает поверх горутин, в рантайме не получится найти вызов futex, позже мы поймем,
почему так сделано.

`sync.Mutex` гарантирует, что критическую секцию выполняет ровно одна горутина:

```go
package main

import (
    "fmt"
    "sync"
)

type SafeCounter struct {
    mu    sync.Mutex
    value int
}

func (c *SafeCounter) Inc() {
    c.mu.Lock()
    defer c.mu.Unlock()
    c.value++
}

func (c *SafeCounter) Get() int {
    c.mu.Lock()
    defer c.mu.Unlock()
    return c.value
}

func main() {
    var wg sync.WaitGroup
    c := &SafeCounter{}

    for i := 0; i < 1000; i++ {
        wg.Add(1)
        go func() {
            defer wg.Done()
            c.Inc()
        }()
    }

    wg.Wait()
    fmt.Println(c.Get()) // всегда 1000
}
```
---

## Deadlock

### Формальное определение

Deadlock — состояние системы, в котором существует множество процессов P₁, P₂, ..., Pₙ,
такое что каждый процесс Pᵢ ожидает освобождения ресурса, удерживаемого другим процессом
из этого же множества, и ни один из них не может продвинуться вперёд.

Необходимые и достаточные условия возникновения дедлока:

1. **Взаимное исключение** — ресурс может удерживаться не более чем одним процессом одновременно.
2. **Удержание и ожидание** — процесс удерживает хотя бы один ресурс и ожидает ещё один.
3. **Отсутствие вытеснения** — ресурс не может быть принудительно изъят; он освобождается только добровольно.
4. **Циклическое ожидание** — существует цепочка P₁ → P₂ → ... → Pₙ → P₁, где каждый ждёт ресурса следующего.

Deadlock возникает тогда и только тогда, когда выполняются все четыре условия одновременно.
Для предотвращения достаточно нарушить любое одно из них.

```go
// Классический deadlock: два мьютекса, захваченных в разном порядке
package main

import (
    "sync"
    "time"
)

func transfer(from, to *sync.Mutex, amount int) {
    from.Lock()
    time.Sleep(10 * time.Millisecond)
    to.Lock()

    to.Unlock()
    from.Unlock()
}

func main() {
    var mu1, mu2 sync.Mutex

    go transfer(&mu1, &mu2, 100) // захватывает mu1 -> mu2
    go transfer(&mu2, &mu1, 200) // захватывает mu2 -> mu1
}
```

### Livelock

#### Формальное определение

Livelock — состояние системы, в котором каждый процесс множества P₁, P₂, ..., Pₙ непрерывно
изменяет своё состояние в ответ на действия других процессов, но ни один из них никогда не
достигает целевого состояния. В отличие от deadlock, процессы не заблокированы — они активно
выполняются, однако полезная работа не производится.

Классический бытовой аналог — два человека в узком коридоре, каждый шагает в сторону, чтобы
пропустить другого, и оба оказываются снова лицом к лицу, бесконечно повторяя этот манёвр.

---

## Wait graph

**Wait graph** (граф ожидания) — ориентированный граф G = (V, E), где:
- V — множество горутин (процессов, транзакций)
- ребро Gᵢ → Gⱼ означает: воркер Gᵢ ожидает ресурс, который в данный момент удерживает воркер Gⱼ»

**Инвариант**: deadlock существует тогда и только тогда, когда в wait graph есть цикл.

### Построение графа

Ребро добавляется, когда воркер пытается захватить уже занятый ресурс:

```
G1 захватил mu1, запрашивает mu2  →  добавляем G1 → G2
G2 захватил mu2, запрашивает mu1  →  добавляем G2 → G1

Граф:
  G1 ──▶ G2
  ▲       │
  └───────┘   ← цикл → deadlock
```

### Снятие ребра при освобождении ресурса

Когда воркер Gⱼ освобождает ресурс, все рёбра вида Gᵢ → Gⱼ, порождённые ожиданием именно
этого ресурса, **удаляются** из графа. Если среди ожидающих воркеров планировщик выбирает Gᵢ
и передаёт ему ресурс, то появляется новое ребро Gₖ → Gᵢ для каждого Gₖ, кто теперь ждёт
ресурс у Gᵢ. Таким образом, граф динамически перестраивается при каждом `Lock`/`Unlock`.

```
До Unlock (G2 держит mu2):        После Unlock G2:
  G1 → G2                           G1 получил mu2, ребро G1→G2 удалено
  G3 → G2                           G3 → G1  (G3 теперь ждёт mu2 у G1)
```

Go runtime отслеживает wait graph внутренне (только для горутин, заблокированных навсегда)
и при обнаружении цикла завершает программу:

```
fatal error: all goroutines are asleep - deadlock!
```

Для баз данных (например, PostgreSQL) детектор deadlock периодически обходит wait graph
в поиске циклов и прерывает одну из транзакций-участниц.

---

## Atomics

Атомарные операции выполняются за одну неделимую инструкцию процессора — без мьютексов и переключения контекста.
```go
package main

import (
	"sync/atomic"
	"time"
)

func loadConfig() map[string]string {
	return make(map[string]string)
}

func requests() chan int {
	return make(chan int)
}

func main() {
	var config atomic.Value // holds current server configuration
	// Create initial config value and store into config.
	config.Store(loadConfig())
	go func() {
		// Reload config every 10 seconds
		// and update config value with the new version.
		for {
			time.Sleep(10 * time.Second)
			config.Store(loadConfig())
		}
	}()
	// Create worker goroutines that handle incoming requests
	// using the latest config value.
	for i := 0; i < 10; i++ {
		go func() {
			for r := range requests() {
				c := config.Load()
				// Handle request r using config c.
				_, _ = r, c
			}
		}()
	}
}
```