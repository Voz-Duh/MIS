//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// MIS was written by Sergey Epishkin, and is placed in the public domain.
// The author hereby disclaims copyright to this source code.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

#ifndef __MIS_H__
#define __MIS_H__


#define __mis_ctn__(a,b) a##b
#define __mis_ctn(a,b) __mis_ctn__(a,b)

typedef void (*MISFallbackType)(const char* caller_file, int caller_line, const char* format, ...);

#ifdef __cplusplus
extern "C" {
#endif

void mis_init(MISFallbackType fallback);
void mis_std_init();

#ifdef __cplusplus
}
#endif

struct __MISSerializerBuilder {
    char* text;
    int count, length;
};

struct __MISSerializerBase {
    int* back_valid;
    int valid;
    const char* action;
    char ender;
    int endable;
};

typedef struct {
    struct __MISSerializerBase;

    int good;
    struct __MISSerializerBuilder builder;
} MISSerializer;

typedef struct {
    struct __MISSerializerBase;

    MISSerializer* root;
    int first_elem;
} MISBranchSerializer;

typedef MISBranchSerializer MISListSerializer;
typedef MISBranchSerializer MISPropertySerializer; 
typedef MISBranchSerializer MISObjectSerializer;

#ifdef __cplusplus
extern "C" {
#endif
MISSerializer mis_ser_create();

int __mis_ser_object(MISListSerializer* ser, MISObjectSerializer* result, const char* caller_file, int caller_line);
int __mis_ser_list(MISListSerializer* ser, MISListSerializer* result, const char* caller_file, int caller_line);
int __mis_ser_as_list(MISSerializer* ser, MISListSerializer* result, const char* caller_file, int caller_line);
int __mis_ser_fin(MISSerializer* ser, const char** result, const char* caller_file, int caller_line);
int __mis_ser_end(MISBranchSerializer* ser, const char* caller_file, int caller_line);

#define mis_ser_add_object(ser, result) __mis_ser_object(_Generic((ser), MISPropertySerializer*: (MISListSerializer*)ser, default: ser), result, __FILE__, __LINE__)
#define mis_ser_add_list(ser, result) __mis_ser_list(_Generic((ser), MISPropertySerializer*: (MISListSerializer*)ser, default: ser), result, __FILE__, __LINE__)
#define mis_ser_as_list(ser, result) __mis_ser_as_list(ser, result, __FILE__, __LINE__)
#define mis_ser_fin(ser, result) __mis_ser_fin(ser, result, __FILE__, __LINE__)
#define mis_ser_end(ser) __mis_ser_end(ser, __FILE__, __LINE__)


#define mis_ser_property(ser, result, name, ...) __mis_ctn(_Generic((ser), \
                                        MISSerializer*: __mis_ser_property_root, \
                                        MISObjectSerializer*: __mis_ser_property_object \
                                    ), __VA_OPT__(_length))(ser, name __VA_OPT__(, __VA_ARGS__), result, __FILE__, __LINE__)

int __mis_ser_property_root         (MISSerializer*       ser, const char* name,             MISPropertySerializer* result, const char* caller_file, int caller_line);
int __mis_ser_property_object       (MISObjectSerializer* ser, const char* name,             MISPropertySerializer* result, const char* caller_file, int caller_line);
int __mis_ser_property_root_length  (MISSerializer*       ser, const char* name, int length, MISPropertySerializer* result, const char* caller_file, int caller_line);
int __mis_ser_property_object_length(MISObjectSerializer* ser, const char* name, int length, MISPropertySerializer* result, const char* caller_file, int caller_line);

int __mis_ser_add_string_length(MISBranchSerializer* ser, const char* value, int length, const char* caller_file, int caller_line);
int __mis_ser_add_string       (MISBranchSerializer* ser, const char* value,             const char* caller_file, int caller_line);
int __mis_ser_add_float        (MISBranchSerializer* ser, float value,                     const char* caller_file, int caller_line);
int __mis_ser_add_double       (MISBranchSerializer* ser, double value,                     const char* caller_file, int caller_line);
int __mis_ser_add_int8         (MISBranchSerializer* ser, char value,                      const char* caller_file, int caller_line);
int __mis_ser_add_int16        (MISBranchSerializer* ser, short value,                     const char* caller_file, int caller_line);
int __mis_ser_add_int32        (MISBranchSerializer* ser, int value,                     const char* caller_file, int caller_line);
int __mis_ser_add_int64        (MISBranchSerializer* ser, long long value,                     const char* caller_file, int caller_line);
int __mis_ser_add_uint8        (MISBranchSerializer* ser, unsigned char  value,                      const char* caller_file, int caller_line);
int __mis_ser_add_uint16       (MISBranchSerializer* ser, unsigned short value,                     const char* caller_file, int caller_line);
int __mis_ser_add_uint32       (MISBranchSerializer* ser, unsigned int value,            const char* caller_file, int caller_line);
int __mis_ser_add_uint64       (MISBranchSerializer* ser, unsigned long long value,                     const char* caller_file, int caller_line);
int __mis_ser_add_boolean      (MISBranchSerializer* ser, int value,                    const char* caller_file, int caller_line);
int __mis_ser_add_fastdouble   (MISBranchSerializer* ser, double value,                     const char* caller_file, int caller_line);

#define mis_ser_add_string_length(ser, value, length) __mis_ser_add_string_length(ser, value, length, __FILE__, __LINE__)
#define mis_ser_add_string(ser, value)                __mis_ser_add_string       (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_float(ser, value)                 __mis_ser_add_float        (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_double(ser, value)                __mis_ser_add_double       (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_int8(ser, value)                  __mis_ser_add_int8         (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_int16(ser, value)                 __mis_ser_add_int16        (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_int32(ser, value)                 __mis_ser_add_int32        (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_int64(ser, value)                 __mis_ser_add_int64        (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_uint8(ser, value)                 __mis_ser_add_uint8        (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_uint16(ser, value)                __mis_ser_add_uint16       (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_uint32(ser, value)                __mis_ser_add_uint32       (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_uint64(ser, value)                __mis_ser_add_uint64       (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_boolean(ser, value)               __mis_ser_add_boolean      (ser, value,         __FILE__, __LINE__)
#define mis_ser_add_fastdouble(ser, value)            __mis_ser_add_fastdouble   (ser, value,         __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////
// ---------------- MIS PARSER ---------------- //
//////////////////////////////////////////////////


typedef struct __MISListBase* MISList;
typedef struct __MISListBase* MISProperty;
typedef struct { const char* value; } MISKeyword;
typedef struct __MISObjectDict* MISObject;
typedef struct { MISObject value; void* container; } MISObjectContainer;
typedef struct { MISList   value; void* container; } MISListContainer;

#define mis_free_container(contain) do { if ((contain).value) { mis_free((contain).value); _aligned_free((contain).container); } } while (0)

typedef enum { MIS_OBJECT, MIS_LIST } MISParserRootType;

typedef struct {
    const char* caller_file; int caller_line;
    int* source;
    int source_length;
    const char* file;
    int i, ln, cl;
} MISParser;

typedef void (*MISParseFallbackType)(MISParser parser, const char* caller_file, int caller_line, const char* format, ...);

#ifdef __cplusplus
extern "C" {
#endif

#define mis_parse(value, file, fallback, ...) _Generic((value), \
                                                MISListContainer*:  _Generic((fallback), \
                                                                        MISParseFallbackType: __mis_parse_file_list, \
                                                                        default:              __mis_parse_source_list \
                                                                    ), \
                                                MISObjectContainer*:_Generic((fallback), \
                                                                        MISParseFallbackType: __mis_parse_file_object, \
                                                                        default:              __mis_parse_source_object \
                                                                    ) \
                                            )(value, file, fallback __VA_OPT__(, __VA_ARGS__), __FILE__, __LINE__)

int __mis_parse_source_list  (MISListContainer*   list,   const char* source, const char* file, MISParseFallbackType fallback, const char* caller_file, int caller_line);
int __mis_parse_file_list    (MISListContainer*   list,   const char* file,                     MISParseFallbackType fallback, const char* caller_file, int caller_line);
int __mis_parse_source_object(MISObjectContainer* object, const char* source, const char* file, MISParseFallbackType fallback, const char* caller_file, int caller_line);
int __mis_parse_file_object  (MISObjectContainer* object, const char* file,                     MISParseFallbackType fallback, const char* caller_file, int caller_line);


#define mis_free(value) _Generic((value), \
                            MISObject: __mis_free_object, \
                            MISList:   __mis_free_list \
                        )(value, __FILE__, __LINE__)

void __mis_free_object(MISObject obj, const char* caller_file, int caller_line);
void __mis_free_list(MISList list, const char* caller_file, int caller_line);

///////////////////////////////////////////////////
// ---------------- MIS GETTERS ---------------- //
///////////////////////////////////////////////////


#define __mis_can_be_container(v, name, type) _Generic((v), \
                                                type:    __mis_ctn(name, _container), \
                                                default: name \
                                            )

#define mis_get_int8(v)    __mis_can_be_container(v, __mis_get_int8,    MISListContainer)(v)
#define mis_get_int16(v)   __mis_can_be_container(v, __mis_get_int16,   MISListContainer)(v)
#define mis_get_int32(v)   __mis_can_be_container(v, __mis_get_int32,   MISListContainer)(v)
#define mis_get_int64(v)   __mis_can_be_container(v, __mis_get_int64,   MISListContainer)(v)
#define mis_get_uint8(v)   __mis_can_be_container(v, __mis_get_uint8,   MISListContainer)(v)
#define mis_get_uint16(v)  __mis_can_be_container(v, __mis_get_uint16,  MISListContainer)(v)
#define mis_get_uint32(v)  __mis_can_be_container(v, __mis_get_uint32,  MISListContainer)(v)
#define mis_get_uint64(v)  __mis_can_be_container(v, __mis_get_uint64,  MISListContainer)(v)
#define mis_get_float(v)   __mis_can_be_container(v, __mis_get_float,   MISListContainer)(v)
#define mis_get_double(v)  __mis_can_be_container(v, __mis_get_double,  MISListContainer)(v)
#define mis_get_bool(v)    __mis_can_be_container(v, __mis_get_bool,    MISListContainer)(v)
#define mis_get_list(v)    __mis_can_be_container(v, __mis_get_list,    MISListContainer)(v)
#define mis_get_object(v)  __mis_can_be_container(v, __mis_get_object,  MISListContainer)(v)
#define mis_get_keyword(v) __mis_can_be_container(v, __mis_get_keyword, MISListContainer)(v)
#define mis_get_string(v)  __mis_can_be_container(v, __mis_get_string,  MISListContainer)(v)

// context getter
#define mis_get(list, i, result) _Generic((result), \
                                      signed char*:      mis_get_i8(list, i, result), \
                                      signed short*:     mis_get_i16(list, i, result), \
                                      signed int*:       mis_get_i32(list, i, result), \
                                      signed long long*: mis_get_i64(list, i, result), \
                                    unsigned char*:      mis_get_u8(list, i, result), \
                                    unsigned short*:     mis_get_u16(list, i, result), \
                                    unsigned int*:       mis_get_u32(list, i, result), \
                                    unsigned long long*: mis_get_u64(list, i, result), \
                                    float*:              mis_get_f32(list, i, result), \
                                    double*:             mis_get_f64(list, i, result), \
                                    MISList*:            mis_get_list(list, i, result), \
                                    MISObject*:          mis_get_object(list, i, result), \
                                    MISKeyword*:         mis_get_keyword(list, i, result), \
                                    const char**:        mis_get_string(list, i, result) \
                                )

int __mis_len(struct __MISListBase* list);
int __mis_len_container(MISListContainer list);

int __mis_get_int8 (struct __MISListBase* list, int i, char*  result);
int __mis_get_int16(struct __MISListBase* list, int i, short* result);
int __mis_get_int32(struct __MISListBase* list, int i, int* result);
int __mis_get_int64(struct __MISListBase* list, int i, long long* result);
int __mis_get_uint8 (struct __MISListBase* list, int i, unsigned char*  result);
int __mis_get_uint16(struct __MISListBase* list, int i, unsigned short* result);
int __mis_get_uint32(struct __MISListBase* list, int i, unsigned int* result);
int __mis_get_uint64(struct __MISListBase* list, int i, unsigned long long* result);
int __mis_get_float(struct __MISListBase* list, int i, float* result);
int __mis_get_double(struct __MISListBase* list, int i, double* result);
int __mis_get_boolean(struct __MISListBase* list, int i, int* result);
int __mis_get_list(struct __MISListBase* list, int i, MISList* result);
int __mis_get_object(struct __MISListBase* list, int i, MISObject* result);
int __mis_get_keyword(struct __MISListBase* list, int i, MISKeyword* result);
int __mis_get_string(struct __MISListBase* list, int i, const char** result);

int __mis_get_int8_container (MISListContainer list, int i, char*  result);
int __mis_get_int16_container(MISListContainer list, int i, short* result);
int __mis_get_int32_container(MISListContainer list, int i, int* result);
int __mis_get_int64_container(MISListContainer list, int i, long long* result);
int __mis_get_uint8_container (MISListContainer list, int i, unsigned char*  result);
int __mis_get_uint16_container(MISListContainer list, int i, unsigned short* result);
int __mis_get_uint32_container(MISListContainer list, int i, unsigned int* result);
int __mis_get_uint64_container(MISListContainer list, int i, unsigned long long* result);
int __mis_get_float_container(MISListContainer list, int i, float* result);
int __mis_get_double_container(MISListContainer list, int i, double* result);
int __mis_get_boolean_container(MISListContainer list, int i, int* result);
int __mis_get_list_container(MISListContainer list, int i, MISList* result);
int __mis_get_object_container(MISListContainer list, int i, MISObject* result);
int __mis_get_keyword_container(MISListContainer list, int i, MISKeyword* result);
int __mis_get_string_container(MISListContainer list, int i, const char** result);


#define mis_extract(object, key) _Generic((object), \
                                    MISObjectContainer: __mis_extract_container, \
                                    default: __mis_extract \
                                )(object, key)

#define mis_extract_length(object, key, key_length) _Generic((object), \
                                    MISObjectContainer: __mis_extract_length_container, \
                                    default: __mis_extract_length \
                                )(object, key, key_length)

MISProperty __mis_extract(MISObject object, const char* key);
MISProperty __mis_extract_container(MISObjectContainer object, const char* key);
MISProperty __mis_extract_length(MISObject object, const char* key, int key_length);
MISProperty __mis_extract_length_container(MISObjectContainer object, const char* key, int key_length);

#ifdef __cplusplus
}
#endif

// massive getters (unpackers)
#pragma region Massive getters (unpackers)
#define mis_unpack1 (list, \
    type1, v1) \
    (mis_len(list) == 1 \
    && mis_get(list, 0, v1))
#define mis_unpack2 (list, \
    type1, v1, \
    type2, v2) \
    (mis_len(list) == 2 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2))
#define mis_unpack3 (list, \
    type1, v1, \
    type2, v2, \
    type3, v3) \
    (mis_len(list) == 3 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3))
#define mis_unpack4 (list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4) \
    (mis_len(list) == 4 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4))
#define mis_unpack5 (list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5) \
    (mis_len(list) == 5 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5))
#define mis_unpack6 (list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6) \
    (mis_len(list) == 6 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6))
#define mis_unpack7 (list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7) \
    (mis_len(list) == 7 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7))
#define mis_unpack8 (list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8) \
    (mis_len(list) == 8 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8))
#define mis_unpack9 (list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9) \
    (mis_len(list) == 9 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9))
#define mis_unpack10(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10) \
    (mis_len(list) == 10 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10))
#define mis_unpack11(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11) \
    (mis_len(list) == 11 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11))
#define mis_unpack12(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12) \
    (mis_len(list) == 12 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12))
#define mis_unpack13(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13) \
    (mis_len(list) == 13 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13))
#define mis_unpack14(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13, \
    type14, v14) \
    (mis_len(list) == 14 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13) \
    && mis_get(list, 13, v14))
#define mis_unpack15(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13, \
    type14, v14, \
    type15, v15) \
    (mis_len(list) == 15 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13) \
    && mis_get(list, 13, v14) \
    && mis_get(list, 14, v15))
#define mis_unpack16(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13, \
    type14, v14, \
    type15, v15, \
    type16, v16) \
    (mis_len(list) == 16 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13) \
    && mis_get(list, 13, v14) \
    && mis_get(list, 14, v15) \
    && mis_get(list, 15, v16))
#define mis_unpack17(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13, \
    type14, v14, \
    type15, v15, \
    type16, v16, \
    type17, v17) \
    (mis_len(list) == 17 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13) \
    && mis_get(list, 13, v14) \
    && mis_get(list, 14, v15) \
    && mis_get(list, 15, v16) \
    && mis_get(list, 16, v17))
#define mis_unpack18(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13, \
    type14, v14, \
    type15, v15, \
    type16, v16, \
    type17, v17, \
    type18, v18) \
    (mis_len(list) == 18 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13) \
    && mis_get(list, 13, v14) \
    && mis_get(list, 14, v15) \
    && mis_get(list, 15, v16) \
    && mis_get(list, 16, v17) \
    && mis_get(list, 17, v18))
#define mis_unpack19(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13, \
    type14, v14, \
    type15, v15, \
    type16, v16, \
    type17, v17, \
    type18, v18, \
    type19, v19) \
    (mis_len(list) == 19 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13) \
    && mis_get(list, 13, v14) \
    && mis_get(list, 14, v15) \
    && mis_get(list, 15, v16) \
    && mis_get(list, 16, v17) \
    && mis_get(list, 17, v18) \
    && mis_get(list, 18, v19))
#define mis_unpack20(list, \
    type1, v1, \
    type2, v2, \
    type3, v3, \
    type4, v4, \
    type5, v5, \
    type6, v6, \
    type7, v7, \
    type8, v8, \
    type9, v9, \
    type10, v10, \
    type11, v11, \
    type12, v12, \
    type13, v13, \
    type14, v14, \
    type15, v15, \
    type16, v16, \
    type17, v17, \
    type18, v18, \
    type19, v19, \
    typ20, v20) \
    (mis_len(list) == 20 \
    && mis_get(list, 0, v1) \
    && mis_get(list, 1, v2) \
    && mis_get(list, 2, v3) \
    && mis_get(list, 3, v4) \
    && mis_get(list, 4, v5) \
    && mis_get(list, 5, v6) \
    && mis_get(list, 6, v7) \
    && mis_get(list, 7, v8) \
    && mis_get(list, 8, v9) \
    && mis_get(list, 9, v10) \
    && mis_get(list, 10, v11) \
    && mis_get(list, 11, v12) \
    && mis_get(list, 12, v13) \
    && mis_get(list, 13, v14) \
    && mis_get(list, 14, v15) \
    && mis_get(list, 15, v16) \
    && mis_get(list, 16, v17) \
    && mis_get(list, 17, v18) \
    && mis_get(list, 18, v19) \
    && mis_get(list, 19, v20))
#pragma endregion Massive getters (unpackers)

void mis_test();

#endif
