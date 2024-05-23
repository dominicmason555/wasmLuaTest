#include <iostream>
#include <string>
#include <cstring>

#include "emscripten/bind.h"
#include "tinyalloc.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

using namespace emscripten;

constexpr size_t LUA_MEM_SIZE = 640000; // 640k ought to be enough for anybody


void *allocator(void *ud, void *ptr, size_t osize, size_t nsize) {
    // Free
    if (nsize == 0) {
        ta_free(ptr);
        return nullptr;
    }
    // Realloc
    else if (ptr != nullptr && nsize > osize) {
        void *new_ptr = ta_alloc(nsize);
        memcpy(new_ptr, ptr, osize);
        ta_free(ptr);
        return new_ptr;
    }
    // Malloc
    else if (ptr == nullptr && nsize > 0) {
        return ta_alloc(nsize);
    } else {
        return ptr;
    }
}


class LuaManager {
private:
    static lua_State* L;
    static bool initialised;
    static uint8_t LUA_MEM[LUA_MEM_SIZE];

public:
    static void init() {
        if (!initialised)
        {
            std::cout << "Initialising allocator" << std::endl;
            ta_init(LUA_MEM, LUA_MEM + LUA_MEM_SIZE - 1, 512, 16, 4);
            std::cout << "Creating Lua state" << std::endl;
            L = lua_newstate(allocator, nullptr);
            std::cout << "Opening Lua libraries" << std::endl;
            luaL_openlibs(L);
            initialised = true;
            std::cout << "Lua initialised" << std::endl;
        }
    }

    static bool isInitialised() { return initialised; }

    static int doString(std::string code) {
        if (initialised)
        {
            int res = luaL_dostring(L, code.c_str());
            if (res) {
                std::cout << "Lua Error: " << lua_tostring(L, -1) << std::endl;
            }
            return res;
        }
        else {
            return -1;
        }
    }

};

lua_State* LuaManager::L = nullptr;
bool LuaManager::initialised = false;
uint8_t LuaManager::LUA_MEM[LUA_MEM_SIZE] = {0};


int addTwoNums(int a, int b) {
    return a + b;
}

int main(int argc, char** argv) {
    std::cout << "hello world!" << std::endl;

    std::cout << "It's Real WASM Hours" << std::endl;
    std::cerr << "Is this an error?" << std::endl;

    LuaManager::init();
    std::cout << "Initialised: " << LuaManager::isInitialised() << std::endl;

    LuaManager::doString("print('Hi from Lua')");

    return 0;
}

EMSCRIPTEN_BINDINGS(webbed) {
    function("addTwoNums", &addTwoNums);
    class_<LuaManager>("LuaManager")
        .class_function("init", &LuaManager::init)
        .class_function("isInitialised", &LuaManager::isInitialised)
        .class_function("doString", &LuaManager::doString);
}
