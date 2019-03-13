#include "table.h"

#include "BinaryDataTableStream.h"
#include "TextDataTableStream.h"
#include "utilities.h"

DataTableStream* create_data_table(const std::string& table_filepath,
                                   const std::vector<std::string>& column_names,
                                   bool truncate,
                                   bool binary,
                                   int compression_level) {
    std::string extension = compression_level == 0 ? "" : ".zst";
    DataTableStream* stream = nullptr;
    if (binary) {
        stream = new BinaryDataTableStream(table_filepath + ".bin" + extension,
                                           column_names,
                                           truncate,
                                           compression_level);
    } else {
        stream = new TextDataTableStream(table_filepath + ".csv" + extension,
                                         column_names,
                                         truncate,
                                         compression_level);
    }
    return stream;
}

SEXP write_data_table(SEXP data_frame,
                      SEXP table_filepath,
                      SEXP truncate,
                      SEXP binary,
                      SEXP compression_level) {
    std::size_t column_count = LENGTH(data_frame);
    std::vector<std::string> column_names{column_count, ""};
    std::vector<SEXP> columns{column_count, R_NilValue};
    std::vector<DataTableStream::column_type_t> column_types{column_count,
                                                             {NILSXP, 0}};
    SEXP column_name_list = getAttrib(data_frame, R_NamesSymbol);
    SEXP column_name_str = R_NilValue;

    for (int column_index = 0; column_index < column_count; ++column_index) {
        columns[column_index] = VECTOR_ELT(data_frame, column_index);
        column_types[column_index] =
            std::make_pair((SEXPTYPE) TYPEOF(columns[column_index]), 0);

        if (column_name_list == R_NilValue) {
            continue;
        }

        column_name_str = STRING_ELT(column_name_list, column_index);

        if (TYPEOF(column_name_str) == CHARSXP) {
            column_names[column_index] = CHAR(column_name_str);
        }
    }

    std::size_t row_count = column_count == 0 ? 0 : LENGTH(columns[0]);

    DataTableStream* stream = create_data_table(sexp_to_string(table_filepath),
                                                column_names,
                                                sexp_to_bool(truncate),
                                                sexp_to_bool(binary),
                                                sexp_to_int(compression_level));

    for (int row_index = 0; row_index < row_count; ++row_index) {
        for (int column_index = 0; column_index < column_count;
             ++column_index) {
            switch (column_types[column_index].first) {
            case LGLSXP:
                stream->write_column(static_cast<bool>(
                    LOGICAL(columns[column_index])[row_index]));
                break;

            case INTSXP:
                stream->write_column(static_cast<int>(
                    INTEGER(columns[column_index])[row_index]));
                break;

            case REALSXP:
                stream->write_column(static_cast<double>(
                    REAL(columns[column_index])[row_index]));
                break;

            case STRSXP:
                stream->write_column(std::string(
                    CHAR(STRING_ELT(columns[column_index], row_index))));
                break;
            }
        }
    }

    delete stream;

    return R_NilValue;
}

static SEXP read_text_data_table(const std::string& filepath,
                                 int compression_level) {
    return R_NilValue;
}

int parse_integer(const char* buffer, const char** end, std::size_t bytes = 4) {
    int value = 0;
    std::memcpy(&value, buffer, bytes);
    *end = buffer + bytes;
    return value;
}

bool can_parse_integer(const char* buffer,
                       const char* end,
                       std::size_t bytes = 4) {
    return (buffer + bytes <= end);
}

double parse_real(const char* buffer, const char** end) {
    double value = 0.0;
    std::memcpy(&value, buffer, sizeof(double));
    *end = buffer + sizeof(double);
    return value;
}

bool can_parse_real(const char* buffer, const char* end) {
    return (buffer + sizeof(double) <= end);
}

int parse_logical(const char* buffer, const char** end) {
    int value = 0;
    std::memcpy(&value, buffer, sizeof(bool));
    *end = buffer + sizeof(bool);
    return value;
}

bool can_parse_logical(const char* buffer, const char* end) {
    return (buffer + sizeof(bool) <= end);
}

SEXPTYPE parse_sexptype(const char* buffer, const char** end) {
    std::uint32_t value = 0;
    std::memcpy(&value, buffer, sizeof(std::uint32_t));
    *end = buffer + sizeof(std::uint32_t);
    return value;
}

SEXP parse_character(const char* buffer,
                     const char** end,
                     char** dest,
                     std::size_t* dest_size) {
    std::uint32_t size = parse_integer(buffer, end, sizeof(std::uint32_t));

    if (size >= *dest_size) {
        free(*dest);
        *dest_size = 2 * size;
        *dest = static_cast<char*>(malloc_or_die(*dest_size));
    }

    std::memcpy(*dest, *end, size);
    *end = *end + size;
    (*dest)[size] = '\0';

    return mkChar(*dest);
}

bool can_parse_character(const char* buffer, const char* end) {
    std::size_t bytes = sizeof(std::uint32_t);

    if (!can_parse_integer(buffer, end, bytes)) {
        return false;
    }

    int character_size = 0;
    std::memcpy(&character_size, buffer, bytes);

    return (buffer + bytes + character_size <= end);
}

struct data_frame_t {
    SEXP object;
    std::size_t row_count;
    std::size_t column_count;
    std::vector<SEXP> columns;
    std::vector<DataTableStream::column_type_t> column_types;
};

static data_frame_t read_header(const char* buffer, const char** end) {
    data_frame_t data_frame{nullptr, 0, 0, {}, {}};
    SEXP column_names;
    SEXP row_names;
    std::size_t character_size = 1024 * 1024;
    int nprotect = 0;

    R_xlen_t row_count = parse_integer(buffer, end);
    R_xlen_t column_count = parse_integer(*end, end);

    data_frame.row_count = row_count;
    data_frame.column_count = column_count;

    if (row_count == 0) {
        column_count = 0;
    }

    data_frame.object = PROTECT(Rf_allocVector(VECSXP, column_count));
    ++nprotect;
    column_names = PROTECT(Rf_allocVector(STRSXP, column_count));
    ++nprotect;
    row_names = PROTECT(Rf_allocVector(STRSXP, row_count));
    ++nprotect;

    data_frame.columns.reserve(column_count);
    data_frame.column_types.reserve(column_count);
    char* character_value = static_cast<char*>(malloc(character_size));

    for (int column_index = 0; column_index < column_count; ++column_index) {
        SEXP name =
            parse_character(*end, end, &character_value, &character_size);
        SET_STRING_ELT(column_names, column_index, name);
        SEXPTYPE sexptype = parse_sexptype(*end, end);
        uint32_t size = parse_integer(*end, end);
        data_frame.column_types.push_back({sexptype, size});
        SEXP column = PROTECT(Rf_allocVector(sexptype, row_count));
        ++nprotect;
        SET_VECTOR_ELT(data_frame.object, column_index, column);
        data_frame.columns.push_back(column);
    }

    for (int row_index = 0; row_index < row_count; ++row_index) {
        sprintf(character_value, "%d", row_index + 1);
        SET_STRING_ELT(row_names, row_index, mkChar(character_value));
    }

    free(character_value);

    Rf_setAttrib(data_frame.object, R_RowNamesSymbol, row_names);
    Rf_setAttrib(data_frame.object, R_NamesSymbol, column_names);
    Rf_setAttrib(data_frame.object, R_ClassSymbol, Rf_mkString("data.frame"));

    UNPROTECT(nprotect);
    return data_frame;
}

static SEXP read_uncompressed_binary_data_table(const std::string& filepath) {
    auto const [buf, buffer_size] = map_to_memory(filepath);

    if (buf == nullptr || buffer_size == 0) {
        REprintf("file '%s' is empty.\n", filepath.c_str());
        return R_NilValue;
    }

    const char* buffer = static_cast<const char*>(buf);
    const char* const end_of_buffer = buffer + buffer_size;
    const char* end = nullptr;

    data_frame_t data_frame{read_header(buffer, &end)};

    if (data_frame.row_count == 0 || data_frame.column_count == 0) {
        unmap_memory(buf, buffer_size);
        return data_frame.object;
    }

    int nprotect = 0;

    PROTECT(data_frame.object);
    ++nprotect;

    std::size_t character_size = 1024 * 1024;
    char* character_value = static_cast<char*>(malloc(character_size));

    int row_index = 0;
    int column_index = 0;
    while (row_index < data_frame.row_count) {
        switch (data_frame.column_types[column_index].first) {
        case LGLSXP:
            LOGICAL(data_frame.columns[column_index])
            [row_index] = parse_logical(end, &end);
            break;

        case INTSXP:
            INTEGER(data_frame.columns[column_index])
            [row_index] = parse_integer(
                end, &end, data_frame.column_types[column_index].second);
            break;

        case REALSXP:
            REAL(data_frame.columns[column_index])
            [row_index] = parse_real(end, &end);
            break;

        case STRSXP:
            SET_STRING_ELT(
                data_frame.columns[column_index],
                row_index,
                parse_character(end, &end, &character_value, &character_size));
            break;

        default:
            Rf_error("unhandled column type %d of column %d in %s ",
                     data_frame.column_types[column_index].first,
                     column_index,
                     filepath.c_str());
        }

        ++column_index;
        if (column_index == data_frame.column_count) {
            column_index = 0;
            ++row_index;
        }
    }

    UNPROTECT(nprotect);
    std::free(character_value);
    unmap_memory(buf, buffer_size);
    return data_frame.object;
}

static SEXP read_compressed_binary_data_table(const std::string& filepath,
                                              int compression_level) {
    auto const [buf, input_buffer_size] = map_to_memory(filepath);

    if (buf == nullptr || input_buffer_size == 0) {
        REprintf("file '%s' is empty.\n", filepath.c_str());
        return R_NilValue;
    }

    const char* input_buffer = static_cast<const char*>(buf);

    const char* const input_buffer_end = input_buffer + input_buffer_size;

    const char* input_buffer_current = nullptr;

    data_frame_t data_frame{read_header(input_buffer, &input_buffer_current)};

    if (data_frame.row_count == 0 || data_frame.column_count == 0) {
        unmap_memory(buf, input_buffer_size);
        return data_frame.object;
    }

    int nprotect = 0;

    PROTECT(data_frame.object);
    ++nprotect;

    ZSTD_inBuffer input{
        input_buffer_current,
        static_cast<std::size_t>(input_buffer_end - input_buffer_current),
        0};

    std::size_t character_size = 1024 * 1024;
    char* character_value = static_cast<char*>(malloc_or_die(character_size));

    std::size_t output_buffer_size = ZSTD_DStreamOutSize();
    char* output_buffer = static_cast<char*>(malloc_or_die(output_buffer_size));
    std::size_t current_output_buffer_size = output_buffer_size;

    ZSTD_DStream* decompression_stream = ZSTD_createDStream();

    if (decompression_stream == NULL) {
        /* TODO - cleanup at this point */
        Rf_error("file '%s': ZSTD_createDStream() error", filepath.c_str());
    }

    const std::size_t init_result = ZSTD_initDStream(decompression_stream);

    if (ZSTD_isError(init_result)) {
        /* TODO - cleanup at this point */
        Rf_error("file '%s': ZSTD_initDStream() error : %s \n",
                 filepath.c_str(),
                 ZSTD_getErrorName(init_result));
    }

    std::size_t remaining_bytes = 0;
    int row_index = 0;
    int column_index = 0;
    const char* output_buffer_cur = nullptr;
    const char* output_buffer_end = nullptr;

    /* run the loop while there is uncompressed data in input file */
    while (input.pos < input.size) {
        ZSTD_outBuffer output = {output_buffer + remaining_bytes,
                                 current_output_buffer_size - remaining_bytes,
                                 0};

        std::size_t decompressed_bytes =
            ZSTD_decompressStream(decompression_stream, &output, &input);

        bool have_enough_data = true;

        if (ZSTD_isError(decompressed_bytes)) {
            /* TODO - cleanup at this point */
            Rf_error("file '%s': ZSTD_decompressStream() error : %s \n",
                     filepath.c_str(),
                     ZSTD_getErrorName(decompressed_bytes));
        }

        /* reading starts from beginning of output buffer to ensure that
           the previously unread partial content is also read */
        output_buffer_cur = output_buffer;
        output_buffer_end = output_buffer + output.pos + remaining_bytes;

        while ((row_index < data_frame.row_count) && have_enough_data) {
            SEXP column = data_frame.columns[column_index];

            switch (data_frame.column_types[column_index].first) {
            case LGLSXP: {
                if (can_parse_logical(output_buffer_cur, output_buffer_end)) {
                    LOGICAL(column)
                    [row_index] =
                        parse_logical(output_buffer_cur, &output_buffer_cur);
                } else {
                    have_enough_data = false;
                }
            }

            break;

            case INTSXP: {
                auto int_bytes = data_frame.column_types[column_index].second;

                if (can_parse_integer(
                        output_buffer_cur, output_buffer_end, int_bytes)) {
                    INTEGER(column)
                    [row_index] = parse_integer(
                        output_buffer_cur, &output_buffer_cur, int_bytes);
                } else {
                    have_enough_data = false;
                }
            }

            break;

            case REALSXP: {
                if (can_parse_real(output_buffer_cur, output_buffer_end)) {
                    REAL(column)
                    [row_index] =
                        parse_real(output_buffer_cur, &output_buffer_cur);
                } else {
                    have_enough_data = false;
                }
            }

            break;

            case STRSXP: {
                if (can_parse_character(output_buffer_cur, output_buffer_end)) {
                    SET_STRING_ELT(column,
                                   row_index,
                                   parse_character(output_buffer_cur,
                                                   &output_buffer_cur,
                                                   &character_value,
                                                   &character_size));
                } else {
                    have_enough_data = false;
                }
            }

            break;

            default: {
                Rf_error("unhandled column type %d of column %d in %s ",
                         data_frame.column_types[column_index].first,
                         column_index,
                         filepath.c_str());
            } break;
            }

            /* The column is read iff there is enough data in the buffer.
               Only if the column is read, do we increment the column_index
               and increase the row_index.
               If there is not enough data in the buffer to read a complete
               column, we have to take care of the partial column data. A simple
               solution is to realloc the buffer to twice its size. This
               algorithm will always work, but may result in a few reallocs
               initially until the buffer size is big enough. */
            if (have_enough_data) {
                ++column_index;
                if (column_index == data_frame.column_count) {
                    column_index = 0;
                    ++row_index;
                }
            } else {
                remaining_bytes = output_buffer_end - output_buffer_cur;

                std::memmove(output_buffer, output_buffer_cur, remaining_bytes);

                if (remaining_bytes + output_buffer_size <=
                    current_output_buffer_size) {
                    /* do nothing because this means that there is enough space
                       left on the buffer for another frame to be decompressed
                     */
                } else {
                    current_output_buffer_size = 2 * current_output_buffer_size;
                    output_buffer = static_cast<char*>(realloc_or_die(
                        output_buffer, current_output_buffer_size));
                }
            }
        }
    }

    std::free(character_value);
    std::free(output_buffer);
    unmap_memory(buf, input_buffer_size);
    UNPROTECT(nprotect);

    if (row_index < data_frame.row_count) {
        Rf_error("PROMISEDYNTRACER ERROR: read_compressed_binary_data_table: ",
                 "input file processed completely but all rows not read.");
    }

    return data_frame.object;
}

SEXP read_data_table(SEXP table_filepath, SEXP binary, SEXP compression_level) {
    const std::string filepath = sexp_to_string(table_filepath);
    bool is_binary = sexp_to_bool(binary);
    int compression_level_value = sexp_to_int(compression_level);

    if (is_binary && (compression_level_value == 0)) {
        return read_uncompressed_binary_data_table(filepath);
    } else if (is_binary && (compression_level_value != 0)) {
        return read_compressed_binary_data_table(filepath,
                                                 compression_level_value);
    } else {
        return read_text_data_table(filepath, compression_level_value);
    }
}
