# About mis
MIS is a high-performance library for serialization and parsing data of a human-readable MIS format, inspired by CSS and JSON. It supports lists, objects, booleans, numbers, "fast numbers", strings, keywords, comments, nested comments, references, safe error handling, and logging.

MIS can be used in realtime systems such as games, modding, and my fantasy is gone.

## How To Use?
- Include headers from `include` directory to compiler.
- Put `mis.c` in your project's code directory.
- Put `#include <mis.h>` in file that will use MIS.
- Enjoy!

Note: Don't write `#include <mis_tables.h>`, don't look inside `mis_tables.h` and your mental will say you "thank".

## Plans:
- Fix possible Out Of Memory crashes.
- Find some other issues.

# Explanation of MIS Format
In MIS format, as already mentioned, there are objects and lists, which are "complex values" that can represent an entire file.

For example, an object file looks like this:
```
a: 1;
b: 2;
```
In turn, a list file would look like this:
```
1, 2, 3
```
As stated, MIS is inspired by CSS, and this is reflected in the "properties" of objects, which support multiple values separated by commas:
```
a: 1, 2, 3;
```
This is the same as a list.

It is important to note that values in MIS are separated by types, for example:
```
a: 1, "hello";
```
Will not be the same as:
```
a: 1, 2;
```
---
Next, MIS has numerical values, which have already been demonstrated, as well as strings:
```
number: 45;
string: "Hello! World!";
```
Strings support almost all escape characters, excluding `\x`, `\u`, and `\U`.

---
Next, MIS has boolean values true and false:
```
booleans: true, false;
```
They have the type boolean and are not keywords, which will be shown later.

---
Next, MIS has keywords, which are string literals without opening and closing symbols:
```
keyword: some_keyword;
```
Keywords support any Unicode character that is a script, from the English alphabet to Chinese characters.

It is important to note that `true` and `false` cannot be keywords; they are reserved for boolean values.

---
Next, MIS has lists:
```
list: (1, 2, 3, 4);
```
You can notice that lists have a tuple-like appearance.

This doesn't make them more convenient, but it separates them from objects, which are represented by a different method.

---
Next, MIS has objects:
```
object: [
  a: 1;
  b: 2;
];
```
Objects can contain properties, as already shown with object files, but they are enclosed within square brackets.

---
Next, MIS has comments and nested comments:
```
{
 Hello! It's a comment!
 { Hello! I'm an inner comment! }
}
a: 1;
```

---
Next, MIS has references, which start with the > symbol:
```
a: 1;
link: > a;
```
It is important to note that references cannot refer to data after them, nor to the properties and lists in which they are located.

From the user's perspective, references simply copy values by reference to their place.

However, internally, MIS performs processes such as simple copying for simple values, and more complex ones, such as borrowing values with tags to prevent double deallocations.

References resolve ambiguities with object properties by selecting the first element of the property to continue the reference, for example:
```
a: 1, 2;
link: > a;
```
In this case, link will contain the numerical value `1`.

It is important to note that these references can choose which property element to use:
```
a: 1, 2;
link: > a 2;
```
In this case, link will contain the numerical value `2`.

This syntax can also be used with lists.

It is important to note that MIS references are 1-indexed; 0 will cause an error `Trying to access not available index 0 in property. MIS links indexation starts from 1.`

---
Finally, MIS has "fast numbers", which are marked with `#` at the beginning, followed by 8 bytes of a binary double number written in little endian.

"Fast numbers" are parsed much faster than regular ones.

If only they are used, can be achieved parsing speed that are almost equal to a binary representation of MIS, if it will be existed.

It can be very useful in unreadable computer-generated MIS files for higher performance.

---
Code with all the listed MIS features, except "fast numbers":
```
{ Comment { another comment } }
bool: true;
number: 54.3;
string: "Hello, World!";
keyword: some;
many: 1, true;
list: (1, 2, 3);
object: [
  a: 1;
  b: 2;
];
link0: > many; { link0 == 1 }
link1: > many 2; { link1 == true }
link2: > object b; { link2 == 2 }
```

# MIS Using
The first what you need to know, every thread in application must call `mis_std_init` or `mis_init` before use MIS.

`mis_std_init` will initialize MIS with MIS standard error fallback function, which is defined as:
```c
void mis_std_fallback(has_caller_location, const char* format, ...) {
    va_list valist;
    printf("%s:%d founded MIS Error: ", caller_file, caller_line);
    va_start(valist, format);
    vprintf(format, valist);
    va_end(valist);
    putchar('\n');
}
```
In turn, `mis_init` will initialize MIS with your own error fallback function, that you will pass to.

## MIS Serializer

### MIS Serializer Features

Writing any integer values using:
- mis_ser_add_int8
- mis_ser_add_int16
- mis_ser_add_int32
- mis_ser_add_int64
- mis_ser_add_uint8
- mis_ser_add_uint16
- mis_ser_add_uint32
- mis_ser_add_uint64

---
Writing any floating-point values using:
- mis_ser_add_float
- mis_ser_add_double
- mis_ser_add_fastdouble - for "fast numbers".

---
Writing strings with escape characters using:
- mis_ser_add_string - uses a null-terminator to find the string length.
- mis_ser_add_string_length - uses an already known string length.

---
Writing boolean values using:
- mis_ser_add_boolean

Writing properties using:
- mis_ser_property - mis_ser_end must be used to close the property.

---
Writing lists using:
- mis_ser_add_list - mis_ser_end must be used to close the list.

---
Writing objects using:
- mis_ser_add_object - mis_ser_end must be used to close the object.

### Serializer Examples
---
Example of using the MIS serializer for an object file:
```c
// Creates MIS serializer
MISSerializer ser = mis_ser_create();

// Creates MIS serializer for property "a"
MISPropertySerializer a;
mis_ser_property(&ser, &a, "a");

// Writes string to property
mis_ser_add_string(&a, "Hello!\nWorld!");

// End property "a"
mis_ser_end(&a);

// Finalize serialization and get the result
const char* result;
mis_ser_fin(&ser, &result);
```
Example of using the MIS serializer for a list file:
```
// Creates MIS serializer
MISSerializer ser = mis_ser_create();
MISListSerializer list;
mis_ser_as_list(&ser, &list);

// Writes string to list
mis_ser_add_string(&list, "Hello!\nWorld!");

// Writes int8 to list
mis_ser_add_int8(&list, 5);

// Don't end "list", it's a file-list that cannot be used in mis_ser_end because mis_ser_fin will end it

// Finalize serialization and get the result
const char* result;
mis_ser_fin(&ser, &result);

// ...

free(result);
```
Closure (`mis_ser_end`) is required to safe MIS structure in the result. 

Example of use with objects requiring closure:
```c
MISSerializer ser = mis_ser_create();
{
    MISPropertySerializer a;
    mis_ser_property(&ser, &a, "a");
    {
        MISListSerializer list;
        mis_ser_add_list(&a, &list);
        {
            mis_ser_add_boolean(&list, true);
            mis_ser_add_boolean(&list, false);
        }
        mis_ser_end(&list);
        MISObjectSerializer obj;
        mis_ser_add_object(&a, &obj);
        {
            MISPropertySerializer obj_a;
            mis_ser_property(&obj, &obj_a, "a");
            {
                mis_ser_add_double(&obj_a, 54.56);
            }
            mis_ser_end(&obj_a);
        }
        mis_ser_end(&obj);
    }
    mis_ser_end(&a);
}
const char* result;
mis_ser_fin(&ser, &result);

// ...

free(result);
```

## MIS parser
The MIS parser, in turn, can be used as follows:

```c
MISObjectContainer obj;
if (!mis_parse(&obj, "a: 35;", "filepath.mis", mis_std_parse_fallback)) {
   printf("MIS parse is caused an error");
   exit(1);
}

// ...

mis_free_container(obj);
```
After which mis_extract can be used on the obtained obj to use the same methods that are used on lists:
```c
MISObjectContainer obj;
if (!mis_parse(&obj, "a: 35;", "filepath.mis", mis_std_parse_fallback)) {
   printf("MIS parse is caused an error");
   exit(1);
}

MISProperty a_prop = mis_extract(obj, "a");
if (mis_len(a_prop) != 1) {
   printf("'a' property is not the same than needed");
   exit(1);
}

int a;
if (!mis_get_int32(a_prop, 0, &a)) {
   printf("'a' property 1 element is not a number");
   exit(1);
}

printf("'a' value is: %i\n", a);

mis_free_container(obj);
```
Similar to `mis_get_int32` methods exist for all types.

---
`mis_std_parse_fallback` is a MIS standard error fallback function, defined as:
```c
void mis_std_parse_fallback(MISParser prs, has_caller_location, const char* format, ...) {
    va_list valist;
    printf("%s:%d\n", caller_file, caller_line);
    printf("%s at %d founded MIS Error: ", prs.file, prs.ln);
    va_start(valist, format);
    vprintf(format, valist);
    va_end(valist);
    putchar('\n');
}
```
You can define your own parse error fallback function and pass it to `mis_parse`.

---
Many of text has been translated by Gemini and checked by me later because I'm not English native and it's very hard to me to translate all that text and safe it readable.

Thank you for reading.
