create_dyntracer <- function(trace_filepath, output_dir,
                             truncate=FALSE, enable_trace=TRUE,
                             verbose=FALSE, binary=TRUE,
                             compression_level=1) {
    .Call(C_create_dyntracer, trace_filepath,
          truncate, enable_trace, verbose,
          output_dir, binary, compression_level)
}

destroy_dyntracer <- function(dyntracer)
   invisible(.Call(C_destroy_dyntracer, dyntracer))

dyntrace_promises <- function(expr, trace_filepath, output_dir,
                              truncate=FALSE, enable_trace = TRUE,
                              verbose=FALSE, binary=TRUE,
                              compression_level=1) {
  write(Sys.time(), file.path(output_dir, "BEGIN"))
  dyntracer <- create_dyntracer(trace_filepath, output_dir,
                                truncate, enable_trace,
                                verbose, binary,
                                compression_level)

  result <- dyntrace(dyntracer, expr)
  destroy_dyntracer(dyntracer)
  write(Sys.time(), file.path(output_dir, "FINISH"))
  result
}

write_data_table <- function(df, filepath, truncate = TRUE,
                             binary = TRUE, compression_level = 1) {
    invisible(.Call(C_write_data_table, df, filepath, truncate,
                    binary, compression_level))
}

read_data_table <- function(filepath_without_ext, binary = TRUE,
                            compression_level = 0) {

    binary = as.logical(binary)
    compression_level <- as.integer(compression_level)
    ext <- data_table_extension(binary, compression_level)

    filepath <- paste0(filepath_without_ext, ".", ext)

    if (!binary & compression_level == 0) {
        read.table(filepath, header = TRUE,
                   sep = "\x1f", comment.char = "",
                   stringsAsFactors = FALSE)
    }
    else {
        .Call(C_read_data_table, filepath, binary, compression_level)
    }
}

data_table_extension <- function(binary = TRUE, compression_level = 0) {
    ext <- if (binary) "bin" else "csv"
    if (compression_level == 0) ext else paste0(ext, ".zst")
}
