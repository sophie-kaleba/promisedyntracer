-- SQLite3 schema for promise tracer
-- Separate statements in this file with semicolons.
-- Do not use semicolons for anything else.

pragma synchronous = off;
pragma journal_mode = off;
pragma page_size = 8192;

create table if not exists metadata (
    --[ data ]-----------------------------------------------------------------
    key text not null,
    value text
);

create table if not exists functions (
    --[ identity ]-------------------------------------------------------------
    id text primary key, -- equiv. to pointer of function definition SEXP
    --[ data ]-----------------------------------------------------------------
    location text,
    definition text,
    type integer not null, -- 0: closure, 1: built-in, 2: special
                                -- values defined by function_type
    compiled boolean not null
);

create table if not exists arguments (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- arbitrary
    --[ data ]-----------------------------------------------------------------
    name text not null,
    position integer not null,
    formal_parameter_position integer not null,
    --[ relations ]------------------------------------------------------------
    call_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (call_id) references functions
);

create table if not exists calls (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- if CALL_ID is off this is equal to SEXP pointer
    -- pointer integer not null, -- we're not using this at all
    --[ data ]-----------------------------------------------------------------
    function_name text,
    callsite text,
    compiled boolean not null, -- TODO remove
    --[ relations ]------------------------------------------------------------
    function_id text not null,
    parent_id integer not null, -- ID of call that executed current call
    in_prom_id integer not null, -- ID of promise in which the call is executed
    parent_on_stack_type integer not null, -- promise = 1, call = 2, none = 0
    parent_on_stack_id integer null,
    call_expression text not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (function_id) references functions,
    foreign key (parent_id) references calls
);

create table if not exists call_returns (
    --[ relations ]------------------------------------------------------------
    call_id integer not null,
    --[ data ]-----------------------------------------------------------------
    return_value_type integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (call_id) references calls
);

create table if not exists promises (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- equal to promise pointer SEXP
    type integer not null,
    full_type text not null,
    in_prom_id integer not null, -- ID of promise in which the promise is executed
    parent_on_stack_type integer not null, -- promise = 1, call = 2, none = 0
    parent_on_stack_id integer null,
    promise_stack_depth integer not null,
    expression text not null
);

create table if not exists promise_associations (
    --[ relations ]------------------------------------------------------------
    promise_id integer null,
    call_id integer not null,
    argument_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (call_id) references calls,
    foreign key (argument_id) references arguments
);

create table if not exists promise_evaluations (
    --[ data ]-----------------------------------------------------------------
    clock integer primary key autoincrement, -- imposes an order on evaluations
    event_type integer not null, -- 0x0: lookup, 0xf: force
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    from_call_id integer not null,
    in_call_id integer not null,
    in_prom_id integer not null, -- ID of promise in which the promise is executed
    lifestyle integer not null, -- 0: virgin, 1: local, 2: branch-local/grandchild,
                                -- 3: escaped/leaked, 4: immediate-local,
                                -- 5: immediate-branch-local/child
    effective_distance_from_origin integer not null,
    actual_distance_from_origin integer not null,
    parent_on_stack_type integer not null, -- promise = 1, call = 2, none = 0
    parent_on_stack_id integer null,
    promise_stack_depth integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (from_call_id) references calls,
    foreign key (in_call_id) references calls
);

-- Whenever we evaluate a promise, we add the information about it here.
-- Two foreign keys have been added for convenience, at the expense of disk
-- space.
create table if not exists promise_returns (
    --[ data ]-----------------------------------------------------------------
    type integer not null,
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    clock integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (clock) references promise_evaluations
);

create table if not exists promise_lifecycle (
    --[ relation ]-------------------------------------------------------------
    promise_id integer not null,
    --[ data ]-----------------------------------------------------------------
    event_type integer not null, --- 0: creation, -- 1: lookup/force -- 2: unmark --3: expression lookup
    gc_trigger_counter integer not null,
    builtin_counter integer not null,
    special_counter integer not null,
    closure_counter integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (gc_trigger_counter) references gc_trigger
);

create table if not exists promise_argument_types (
    --[ data ]-----------------------------------------------------------------
    promise_id integer not null,
    default_argument boolean not null,
    --[ relations ]------------------------------------------------------------
    foreign key (promise_id) references promises
);

create table if not exists gc_trigger (
    --[ identity ]-------------------------------------------------------------
    counter integer primary key,
    --[ data ]-----------------------------------------------------------------
    ncells real not null,
    vcells real not null,
    builtin_calls integer not null,
    special_calls integer not null,
    closure_calls integer not null
);

create table if not exists type_distribution (
    --[ relation ]-------------------------------------------------------------
    gc_trigger_counter integer not null,
    --[ data ]-----------------------------------------------------------------
    type integer not null,
    length integer not null,
    bytes integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (gc_trigger_counter) references gc_trigger
);

create table if not exists environments (
    --[ data ]-----------------------------------------------------------------
    id integer primary key,
    function_id text not null,
    --[ relations ]------------------------------------------------------------
    foreign key (function_id) references functions
);

create table if not exists variables (
    --[ data ]-----------------------------------------------------------------
    id integer primary key,
    name text not null,
    environment_id integer not null,
    --[ relations ]------------------------------------------------------------
    foreign key (environment_id) references environments
);

create table if not exists variable_actions (
    --[ data ] ----------------------------------------------------------------
    promise_id integer not null,
    variable_id integer not null,
    action text not null,
    --[ relations ]------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (variable_id) references variables
);
