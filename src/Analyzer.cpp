#include "Analyzer.h"

void Analyzer::inspect_function_caller_(const std::string &name) {
    for(auto& function_name : interesting_function_names_) {
        if(name != function_name) { continue; }
        function_caller_count_mapping_.insert(function_name, get_nth_closure_(1));
        return;
    }
    // for(auto& key_value : caller_count_mapping_) {
    //     /* if this is not the function we are looking for
    //        continue to check other functions in the table.
    //      */
    //     if(name != key_value.first) continue;
    //     const function_id_t caller_fn_id = infer_caller_();
    //     /* insert the inferred caller of this function in the table.
    //        If insertion fails, it means the caller has called this
    //        function before. In that case, increment the function counter.
    //      */
    //     auto iter = key_value.second.insert({caller_fn_id, 1});
    //     if(!iter.second) {
    //         ++iter.first -> second;
    //     }
    //     return;
    // }
}



/* When we enter a function, push information about it on a custom call stack.
   We also update the function table to create an entry for this function.
   This entry contains the evaluation information of the function's arguments.
 */



// void Analyzer::serialize_function_callers_() {
//     for(const auto & key_value : function_caller_count_mapping_) {
//         for (const auto & caller_count : key_value.second) {
//             function_caller_data_table_ -> write_row(key_value.first,
//                                                     caller_count.first,
//                                                     caller_count.second);
//         }
//     }
// }

// void Analyzer::serialize_function_callers_() {
//     for(const auto & key_value : function_caller_count_mapping_) {
//         for (const auto & caller_count : key_value.second) {
//             function_caller_data_table_ -> write_row(key_value.first,
//                                                      caller_count.first,
//                                                      caller_count.second);
//         }
//     }
// }

// void Analyzer::serialize_caller_count_mapping_(const caller_table_t& caller_count_mapping,
//                                                          DataTableStream* data_table) {
//     for(const auto & key_value : caller_count_mapping_) {
//         for (const auto & caller_count : key_value.second) {
//             data_table_ -> write_row(key_value.first,
//                                      caller_count.first,
//                                      caller_count.second);
//         }
//     }
// }

// void Analyzer::add_call_graph_edge_(const call_id_t callee_id) {
//     if (call_stack_.empty())
//         return;
//     call_graph_data_table_->write_row(
//         static_cast<double>(call_stack_.back()->get_call_id()),
//         static_cast<double>(callee_id));
// }

