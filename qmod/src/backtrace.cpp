#include "backtrace.hpp"

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg) {
    auto* state = static_cast<BacktraceState *>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (state->first) {
        pc = state->firstPC;
        state->first = false;
    }
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void *>(pc);
        }
    }
    return _URC_NO_REASON;
}

size_t captureBacktrace(void **buffer, uint16_t max, uintptr_t pc) {
    BacktraceState state{true, pc, buffer, buffer + max};
    _Unwind_Backtrace(unwindCallback, &state);

    return state.current - buffer;
}

BacktraceDetails getBacktraceLines(uint16_t frameCount, uintptr_t targetPc, int framesToSkip) {
    static auto fmtWithMethod = "      #%02i pc %016lx  %s (%s)%s\n";
    static auto fmtWithoutMethod = "      #%02i pc %016lx  %s%s\n";
    
    BacktraceDetails output;
    output.lines.append("*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n");
    output.lines.append("backtrace:\n");

    void* buffer[frameCount + 1];
    captureBacktrace(buffer, frameCount + 1, targetPc);
    for (uint16_t i = framesToSkip; i < frameCount; ++i) {
        Dl_info info;
        if (dladdr(buffer[i + 1], &info)) {
            long addr = reinterpret_cast<char*>(buffer[i + 1]) - reinterpret_cast<char*>(info.dli_fbase) - 4;
            std::string buildId;
            std::string fname = std::string(info.dli_fname);
            if (fname.ends_with(".so")) {
                buildId = " (BuildId: " + getBuildId(info.dli_fname) + ")";
                if (fname.find("com.beatgames.beatsaber/") != std::string::npos) {
                    output.libraries.insert(info.dli_fname);
                }
            }
            if (info.dli_sname) {
                int status;
                const char *demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
                if (status) {
                    demangled = info.dli_sname;
                }
                output.lines.append(
                    string_format(fmtWithMethod, i - framesToSkip, addr, info.dli_fname, demangled, buildId.c_str())
                );
            } else {
                output.lines.append(
                    string_format(fmtWithoutMethod, i - framesToSkip, addr, info.dli_fname, buildId.c_str())
                );
            }
        } else {
            // break;
        }
    }
    output.lines.pop_back();
    return output;
}
