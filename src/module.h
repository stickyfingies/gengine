#pragma once

/*

namespace { auto data = Global{} }

MODULE_CALLBACK("foo", START)
{
    std::cout << "startup code goes here " << data.baz << std::endl;
}

///////////

static auto start = Register("foo", []() {
    std::cout << "hello!" << std::endl;
});

*/

namespace gengine
{

enum class RuntimeStage
{
    START,
    STOP
};

using ModuleStartCallback = bool(*)();

auto register_module_callback(const char* name, RuntimeStage stage, ModuleStartCallback cb)->void;

auto execute_module_callbacks(RuntimeStage stage)->void;

}

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define MODULE_CALLBACK(name, stage)                                     \
    static_assert(true, name " must be string literal");                 \
    static bool CONCAT(callback_, __LINE__)();                           \
    namespace                                                            \
    {                                                                    \
    using namespace gengine;                                             \
    struct CONCAT(CallbackReistrant, __LINE__)                           \
    {                                                                    \
        CONCAT(CallbackReistrant, __LINE__)()                            \
        { register_module_callback(name, RuntimeStage::stage, CONCAT(callback_, __LINE__)); } \
    } CONCAT(registrant_, __LINE__);                                     \
    }                                                                    \
    static bool CONCAT(callback_, __LINE__)()
