#ifndef TURBOTRACER_EVENT_H
#define TURBOTRACER_EVENT_H

#include <string>

enum class Event {
    DyntraceEntry = 0,
    DyntraceExit,
    EvalEntry,
    ClosureEntry,
    ClosureExit,
    BuiltinEntry,
    BuiltinExit,
    SpecialEntry,
    SpecialExit,
    S3DispatchEntry,
    S4DispatchArgument,
    ContextEntry,
    ContextJump,
    ContextExit,
    GcAllocate,
    PromiseForceEntry,
    PromiseForceExit,
    PromiseValueLookup,
    PromiseExpressionLookup,
    PromiseEnvironmentLookup,
    PromiseExpressionAssign,
    PromiseValueAssign,
    PromiseEnvironmentAssign,
    PromiseSubstitute,
    GcUnmark,
    EnvironmentVariableDefine,
    EnvironmentVariableAssign,
    EnvironmentVariableRemove,
    EnvironmentVariableLookup,
    COUNT
};

inline std::string to_string(const Event event) {
    switch (event) {
    case Event::DyntraceEntry:
        return "DyntraceEntry";
    case Event::DyntraceExit:
        return "DyntraceExit";
    case Event::EvalEntry:
        return "EvalEntry";
    case Event::ClosureEntry:
        return "ClosureEntry";
    case Event::ClosureExit:
        return "ClosureExit";
    case Event::BuiltinEntry:
        return "BuiltinEntry";
    case Event::BuiltinExit:
        return "BuiltinExit";
    case Event::SpecialEntry:
        return "SpecialEntry";
    case Event::SpecialExit:
        return "SpecialExit";
    case Event::S3DispatchEntry:
        return "S3DispatchEntry";
    case Event::S4DispatchArgument:
        return "S4DispatchArgument";
    case Event::ContextEntry:
        return "ContextEntry";
    case Event::ContextJump:
        return "ContextJump";
    case Event::ContextExit:
        return "ContextExit";
    case Event::GcAllocate:
        return "GcAllocate";
    case Event::PromiseForceEntry:
        return "PromiseForceEntry";
    case Event::PromiseForceExit:
        return "PromiseForceExit";
    case Event::PromiseValueLookup:
        return "PromiseValueLookup";
    case Event::PromiseExpressionLookup:
        return "PromiseExpressionLookup";
    case Event::PromiseEnvironmentLookup:
        return "PromiseEnvironmentLookup";
    case Event::PromiseExpressionAssign:
        return "PromiseExpressionAssign";
    case Event::PromiseValueAssign:
        return "PromiseValueAssign";
    case Event::PromiseEnvironmentAssign:
        return "PromiseEnvironmentAssign";
    case Event::PromiseSubstitute:
        return "PromiseSubstitute";
    case Event::GcUnmark:
        return "GcUnmark";
    case Event::EnvironmentVariableDefine:
        return "EnvironmentVariableDefine";
    case Event::EnvironmentVariableAssign:
        return "EnvironmentVariableAssign";
    case Event::EnvironmentVariableRemove:
        return "EnvironmentVariableRemove";
    case Event::EnvironmentVariableLookup:
        return "EnvironmentVariableLookup";
    case Event::COUNT:
        return "UnknownEvent";
    }

    return "UnknownEvent";
};

#endif /* TURBOTRACER_EVENT_H */
