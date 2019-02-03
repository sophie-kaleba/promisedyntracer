#include "AnalysisSwitch.h"

std::ostream &operator<<(std::ostream &os,
                         const AnalysisSwitch &analysis_switch) {
    os << std::endl
       << "Metadata Analysis               : " << analysis_switch.metadata
       << std::endl
       << "Function Analysis               : " << analysis_switch.function
       << std::endl
       << "Strictness Analysis             : " << analysis_switch.strictness
       << std::endl
       << "Side Effect Analysis            : " << analysis_switch.side_effect
       << std::endl;

    return os;
}
