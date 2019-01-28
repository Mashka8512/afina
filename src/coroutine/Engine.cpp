#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <cstring>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char Higher;
    ctx.Hight = &Higher;
    ctx.Low = this->StackBottom;
    std::size_t length = abs(ctx.Hight - ctx.Low);
    char* stack = new char[length];
    std::memcpy(stack, ctx.Low, length);
    delete[] std::get<0>(ctx.Stack);
    ctx.Stack = std::make_tuple(stack, length);
}

void Engine::Restore(context &ctx) {
    char now;
    while (((ctx.Low < &now) && (&now < ctx.Hight)) || ((ctx.Low > &now) && (&now > ctx.Hight))) {
        Restore(ctx);
    }
    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    cur_routine = &ctx;
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    auto routine = alive;
    if (routine == cur_routine) {
        routine = alive->next;
    }
    if (routine != nullptr) {
        sched(static_cast<void*>(routine));
    }
}

void Engine::sched(void *routine_) {
    if (setjmp(cur_routine->Environment) == 0) {
        Store(*cur_routine);
        context* ctx = static_cast<context*>(routine_);
        Restore(*ctx);
    }
}

} // namespace Coroutine
} // namespace Afina
