#include "utilities.h"
#include "base64.h"
#include "lookup.h"
#include <algorithm>

int get_file_size(std::ifstream &file) {
    int position = file.tellg();
    file.seekg(0, std::ios_base::end);
    int length = file.tellg();
    file.seekg(position, std::ios_base::beg);
    return length;
}

std::string readfile(std::ifstream &file) {
    std::string contents;
    file.seekg(0, std::ios::end);
    contents.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    contents.assign(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>());
    return contents;
}

bool file_exists(const std::string &filepath) {
    return std::ifstream(filepath).good();
}

char *copy_string(char *destination, const char *source, size_t buffer_size) {
    size_t l = strlen(source);
    if (l >= buffer_size) {
        strncpy(destination, source, buffer_size - 1);
        destination[buffer_size - 1] = '\0';
    } else {
        strcpy(destination, source);
    }
    return destination;
}

bool sexp_to_bool(SEXP value) { return LOGICAL(value)[0] == TRUE; }

int sexp_to_int(SEXP value) { return (int)*INTEGER(value); }

std::string sexp_to_string(SEXP value) {
    return std::string(CHAR(STRING_ELT(value, 0)));
}

const char* get_name(SEXP sexp) {
    const char *s = NULL;

    switch (TYPEOF(sexp)) {
        case CHARSXP:
            s = CHAR(sexp);
            break;
        case LANGSXP:
            s = get_name(CAR(sexp));
            break;
        case BUILTINSXP:
        case SPECIALSXP:
            s = CHAR(PRIMNAME(sexp));
            break;
        case SYMSXP:
            s = CHAR(PRINTNAME(sexp));
            break;
    }

    return s == NULL ? "" : s;
}

static int get_lineno(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {

        if (TYPEOF(srcref) == VECSXP) {
            srcref = VECTOR_ELT(srcref, 0);
        }

        return asInteger(srcref);
    }

    return -1;
}

static int get_colno(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {

        if (TYPEOF(srcref) == VECSXP) {
            srcref = VECTOR_ELT(srcref, 0);
        }

        //        INTEGER(val)[0] = lloc->first_line;
        //        INTEGER(val)[1] = lloc->first_byte;
        //        INTEGER(val)[2] = lloc->last_line;
        //        INTEGER(val)[3] = lloc->last_byte;
        //        INTEGER(val)[4] = lloc->first_column;
        //        INTEGER(val)[5] = lloc->last_column;
        //        INTEGER(val)[6] = lloc->first_parsed;
        //        INTEGER(val)[7] = lloc->last_parsed;

        if (TYPEOF(srcref) == INTSXP) {
            // lineno = INTEGER(srcref)[0];
            return INTEGER(srcref)[4];
        } else {
            // This should never happen, right?
            return -1;
        }
    }

    return -1;
}

#include <Rinternals.h>
static const char *get_filename(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {
        if (TYPEOF(srcref) == VECSXP)
            srcref = VECTOR_ELT(srcref, 0);
        SEXP srcfile = getAttrib(srcref, R_SrcfileSymbol);
        if (TYPEOF(srcfile) == ENVSXP) {
            lookup_result r = find_binding_in_environment(
                install("filename"), srcfile); // TODO we should move
                                               // install("filename") to be
                                               // executed only once
            if (r.status == lookup_status::SUCCESS) {
                SEXP filename = r.value;
                if (isString(filename) && Rf_length(filename)) {
                    return CHAR(STRING_ELT(filename, 0));
                }
            } else {
                // Not sure what the frequency of this is. Making it an error
                // for now, and we'll see what happens.
                std::string msg = lookup_status_to_string(r.status);
                dyntrace_log_error("%s", msg.c_str());
            }
        }
    }

    return NULL;
}

inline std::string extract_location_information(SEXP srcref) {
    const char *filename = get_filename(srcref);
    int lineno = get_lineno(srcref);
    int colno = get_colno(srcref);

    if (filename) {
        std::stringstream result;
        result << ((strlen(filename) > 0) ? filename : "<console>") << ":"
               << std::to_string(lineno) << "," << std::to_string(colno);
        return result.str();
    } else
        return "";
}

std::string get_callsite_cpp(int how_far_in_the_past) {
    SEXP srcref = R_GetCurrentSrcref(how_far_in_the_past);
    return extract_location_information(srcref);
}

std::string get_definition_location_cpp(SEXP op) {
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    return extract_location_information(srcref);
}



std::string get_expression(SEXP e) {
    std::string expression;
    int linecount = 0;
    SEXP strvec = serialize_sexp(e, &linecount);
    for (int i = 0; i < linecount - 1; ++i) {
        expression.append(CHAR(STRING_ELT(strvec, i))).append("\n");
    }
    if (linecount >= 1) {
        expression.append(CHAR(STRING_ELT(strvec, linecount - 1)));
    }
    return expression;
}

std::string escape(const std::string &s) {
    // https://stackoverflow.com/questions/5612182/convert-string-with-explicit-escape-sequence-into-relative-character
    std::string res;
    std::string::const_iterator it = s.begin();
    while (it != s.end()) {
        char c = *it++;
        if (c == '\n') {
            res += "    ";
        } else if (c == '\t') {
            res += "    ";
        } else {
            res += c;
        }
    }
    return res;
}

// returns a monotonic timestamp in microseconds
uint64_t timestamp() {
    uint64_t t;
// The __MACH__ bit is from http://stackoverflow.com/a/6725161/6846474
#if !defined(HAVE_CLOCK_GETTIME) && defined(__MACH__)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    t = mts.tv_sec * 1e9 + mts.tv_nsec;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t = ts.tv_sec * 1e9 + ts.tv_nsec;
#endif
    return t;
}



std::string compute_hash(const char *data) {
    const EVP_MD *md = EVP_md5();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX mdctx;
    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);
    EVP_DigestUpdate(&mdctx, data, strlen(data));
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);
#else
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_MD_CTX_init(mdctx);
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, data, strlen(data));
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);
#endif
    std::string result{base64_encode(
        reinterpret_cast<const unsigned char *>(md_value), md_len)};

    // This replacement is done so that the hash can be directly used
    // as a filename. If this is not done, the / in the hash prevents
    // it from being used as the name of the file which contains the
    // function which is hashed.
    std::replace(result.begin(), result.end(), '/', '#');
    return result;
}

const char *remove_null(const char *value) { return value ? value : ""; }

std::string clock_ticks_to_string(clock_t ticks) {
    return std::to_string((double)ticks / CLOCKS_PER_SEC);
}

std::string to_string(const char *str) {
    return str ? std::string(str) : std::string("");
}

std::string pos_seq_to_string(const pos_seq_t &pos_seq) {

    if(pos_seq.size() == 0) {
        return "()";
    }

    std::string str = "(" + std::to_string(pos_seq[0]);

    for(auto i = 1; i < pos_seq.size(); ++i) {
        str.append(" ").append(std::to_string(pos_seq[i]));
    }

    return str + ")";
}
