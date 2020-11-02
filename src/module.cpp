#include "module.h"

#include <iostream>
#include <unordered_map>
#include <deque>

namespace
{
using namespace gengine;

auto get_module_start_callbacks()->std::deque<ModuleStartCallback>&
{
    static auto start_callbacks = std::deque<ModuleStartCallback>{};
    return start_callbacks;
}

auto get_module_stop_callbacks()->std::deque<ModuleStartCallback>&
{
    static auto stop_callbacks = std::deque<ModuleStartCallback>{};
    return stop_callbacks;
}

}

namespace gengine
{

auto register_module_callback(const char* name, RuntimeStage stage, ModuleStartCallback cb)->void
{
    std::cout << "[info]\t registering module callback: " << name << std::endl;

    switch (stage)
    {
        case RuntimeStage::START:
            get_module_start_callbacks().push_back(cb);
            break;
        case RuntimeStage::STOP:
            get_module_stop_callbacks().push_back(cb);
            break;
    }
}

auto execute_module_callbacks(RuntimeStage stage)->void
{
    // strategy: repeat callbacks until they succeed
    // assumption: all callbacks will eventually succeed

    switch (stage)
    {
        case RuntimeStage::START:
            while (get_module_start_callbacks().size())
            {
                const auto& cb = get_module_start_callbacks().front();
                get_module_start_callbacks().pop_front();

                if (!cb())
                {
                    get_module_start_callbacks().push_back(cb);
                }
            }
            break;
        case RuntimeStage::STOP:
            while (get_module_stop_callbacks().size())
            {
                const auto& cb = get_module_stop_callbacks().front();
                get_module_stop_callbacks().pop_front();

                if (!cb())
                {
                    get_module_stop_callbacks().push_back(cb);
                }
            }
            break;
    }
}

}