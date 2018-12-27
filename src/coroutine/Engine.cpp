#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    // if (setjmp(ctx.Environment) == 0) {
        char Higher;
        ctx.Hight = &Higher;
        ctx.Low = this->StackBottom;
        std::size_t length = ctx.Hight - ctx.Low;
        char* stack = new char[length];
        std::memcpy(stack, ctx.Low, length);
        ctx.Stack = std::make_tuple(stack, length);
    //}
}

void Engine::Restore(context &ctx) {
    char now;
    while (now < ctx.Hight) {
        Restore(ctx);
    }
    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    auto routine = alive;
    while (routine == cur_routine) {
        routine = alive->next;
    }
    if (routine != nullptr) {
        Restore(routine);
    }
}

void Engine::sched(void *routine_) {
    auto ctx = std::static_cast<context>(routine_);
    Restore(ctx);
}

} // namespace Coroutine
} // namespace Afina
