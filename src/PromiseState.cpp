#include "PromiseState.h"

std::string to_string(PromiseState::SlotMutation slot_mutation) {
    switch (slot_mutation) {
        case PromiseState::SlotMutation::ENVIRONMENT_LOOKUP:
            return "ENVIRONMENT_LOOKUP";
        case PromiseState::SlotMutation::ENVIRONMENT_ASSIGN:
            return "ENVIRONMENT_ASSIGN";
        case PromiseState::SlotMutation::EXPRESSION_LOOKUP:
            return "EXPRESSION_LOOKUP";
        case PromiseState::SlotMutation::EXPRESSION_ASSIGN:
            return "EXPRESSION_ASSIGN";
        case PromiseState::SlotMutation::VALUE_LOOKUP:
            return "VALUE_LOOKUP";
        case PromiseState::SlotMutation::VALUE_ASSIGN:
            return "VALUE_ASSIGN";
        case PromiseState::SlotMutation::COUNT:
            return "COUNT";
        default:
            return "UNKNOWN_SLOT_MUTATION";
    }
}
