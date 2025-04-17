int replicatesignal(lua_State* L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);
    auto signal_userdata = lua_touserdata(L, 1);
    if (!signal_userdata)
        luaL_error(L, "Invalid RBXScriptSignal");

    const auto signal = *reinterpret_cast<uintptr_t*>(signal_userdata);

    uintptr_t classDescriptor = *reinterpret_cast<uintptr_t*>(signal + 0x18);
    uintptr_t eventDescriptorList = *reinterpret_cast<uintptr_t*>(classDescriptor + 0x40);

    const char* signalName = nullptr;
    if (lua_gettop(L) >= 2 && lua_isstring(L, 2)) {
        signalName = lua_tostring(L, 2);
    }

    uintptr_t foundDescriptor = 0;
    for (int i = 0; i < *reinterpret_cast<int*>(eventDescriptorList + 8); ++i) {
        uintptr_t descriptor = *reinterpret_cast<uintptr_t*>(eventDescriptorList + 16 + i * sizeof(uintptr_t));
        const char* name = *reinterpret_cast<const char**>(descriptor + 8);
        if (!signalName || strcmp(signalName, name) == 0) {
            foundDescriptor = descriptor;
            break;
        }
    }

    if (!foundDescriptor)
        luaL_error(L, "Event descriptor not found");

    uintptr_t raiseEventInvocation = *reinterpret_cast<uintptr_t*>(foundDescriptor + 0x48);
    uintptr_t replicateEvent = *reinterpret_cast<uintptr_t*>(foundDescriptor + 0x50);

    int nargs = lua_gettop(L) - 1;

    if (raiseEventInvocation) {
        using RaiseEventInvocationFn = void(__fastcall*)(uintptr_t event, uintptr_t instance, void** args, int argCount);
        auto raise = reinterpret_cast<RaiseEventInvocationFn>(raiseEventInvocation);

        void** args = new void*[nargs];
        for (int i = 0; i < nargs; ++i) {
            int idx = i + 2; // starting from 2nd arg
            if (lua_isuserdata(L, idx)) {
                args[i] = *reinterpret_cast<void**>(lua_touserdata(L, idx));
            } else if (lua_isnumber(L, idx)) {
                double* num = new double(lua_tonumber(L, idx));
                args[i] = num;
            } else if (lua_isboolean(L, idx)) {
                bool* b = new bool(lua_toboolean(L, idx));
                args[i] = b;
            } else if (lua_isstring(L, idx)) {
                args[i] = (void*)lua_tostring(L, idx);
            } else {
                delete[] args;
                luaL_error(L, "Unsupported argument type at position %d", idx);
            }
        }

        raise(signal, signal, args, nargs);
        delete[] args;
    }
    else if (replicateEvent) {
        using ReplicateEventFn = void(__fastcall*)(uintptr_t event, uintptr_t instance, uintptr_t extraData);
        auto replicate = reinterpret_cast<ReplicateEventFn>(replicateEvent);
        replicate(signal, signal, 0);
    }
    else {
        luaL_error(L, "Neither RaiseEventInvocation nor ReplicateEvent function found.");
    }

    return 0;
}

// this wont work for you doobie srry
