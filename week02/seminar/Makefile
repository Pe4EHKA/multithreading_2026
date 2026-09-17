.PHONY: all run_stage_00 run_stage_01

GO ?= go
RACE ?=

GO_RUN_FLAGS :=
GO_BUILD_FLAGS :=

ifneq ($(RACE),)
GO_RUN_FLAGS += -race
GO_BUILD_FLAGS += -race
endif

STAGE_00 := ./goroutines_basic
STAGE_01 := ./philosophers_naive_deadlock

all:
	$(GO) build $(GO_BUILD_FLAGS) ./...

run_stage_00:
	$(GO) run $(GO_RUN_FLAGS) $(STAGE_00)

run_stage_01:
	$(GO) run $(GO_RUN_FLAGS) $(STAGE_01)

clean:
	$(GO) clean -cache
