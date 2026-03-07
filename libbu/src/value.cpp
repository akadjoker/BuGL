#include "value.hpp"
#include "interpreter.hpp"
#include "pool.hpp"
#include "platform.hpp"
#include <stdarg.h>

#include "value.hpp"
#include "interpreter.hpp"
#include "pool.hpp"
#include "platform.hpp"
#include <stdarg.h>

Value::Value() : type(ValueType::NIL)
{
    as.byte = 0;
}

 

// // Unpack
// uint8 getType(Value v) {
//     return (v.as.integer >> 24) & 0xFF;
// }
// uint16 getModuleId(Value v) {
//     return (v.as.integer >> 12) & 0xFFF;
// }
// uint16 getFuncId(Value v) {
//     return v.as.integer & 0xFFF;
// }




const char *valueTypeToString(ValueType type)
{
    switch (type)
    {
    case ValueType::NIL:
        return "nil";
    case ValueType::CHAR:
        return "char";
    case ValueType::BOOL:
        return "bool";
    case ValueType::INT:
        return "int";
    case ValueType::BYTE:
        return "byte";
    case ValueType::FLOAT:
        return "float";
    case ValueType::UINT:
        return "uint";
    case ValueType::LONG:
        return "long";
    case ValueType::ULONG:
        return "ulong";
    case ValueType::DOUBLE:
        return "float";
    case ValueType::STRING:
        return "string";
    case ValueType::ARRAY:
        return "array";
    case ValueType::MAP:
        return "map";
    case ValueType::BUFFER:
        return "buffer";
    case ValueType::FUNCTION:
        return "<function>";
    case ValueType::NATIVE:
        return "<native>";
    case ValueType::NATIVEPROCESS:
        return "<native_process>";
    case ValueType::PROCESS:
        return "<process>";
    case ValueType::PROCESS_INSTANCE:
        return "<process_instance>";
    case ValueType::STRUCT:
        return "<struct>";
    case ValueType::CLASS:
        return "<class>";
    case ValueType::STRUCTINSTANCE:
        return "<struct_instances>";        
    case ValueType::CLASSINSTANCE:
        return "<class_instances>";     
    case ValueType::NATIVECLASSINSTANCE:
        return "<native_class_instances>";   
    case ValueType::NATIVESTRUCTINSTANCE:
        return "<native_struct_instances>";
    case ValueType::POINTER:
        return "<pointer>";
    case ValueType::MODULEREFERENCE:
        return "<module_reference>";
    case ValueType::NATIVECLASS:
        return "<native_class>";
    case ValueType::NATIVESTRUCT:
        return "<native_struct>";
    case ValueType::CLOSURE:
        return "<closure>";
 

    }
    return "<unknown>";
}

void printValue(const Value &value)
{
    switch (value.type)
    {
    case ValueType::NIL:
        OsPrintf("nil");
        break;
    case ValueType::BOOL:
        OsPrintf("%s", value.as.boolean ? "true" : "false");
        break;
    case ValueType::BYTE:
        OsPrintf("%d", value.as.byte);
        break;
    case ValueType::INT:
        OsPrintf("%d", value.as.integer);
        break;
    case ValueType::UINT:
        OsPrintf("%u", value.as.unsignedInteger);
        break;
    case ValueType::FLOAT:
        OsPrintf("%.4f", value.as.real);
        break;
    case ValueType::DOUBLE:
        OsPrintf("%.4f", value.as.number);
        break;
    case ValueType::STRING:
    {
        String *str = value.as.string;
        const char *chars = str->chars();
        size_t len = str->length();

        // Se termina com '\n', imprime sem ele
        if (len > 0 && chars[len - 1] == '\n')
        {
            OsPrintf("%.*s", (int)(len - 1), chars); // Imprime len-1 chars
        }
        else
        {
            OsPrintf("%s", chars); // Imprime normal
        }
        break;
    }
    case ValueType::FUNCTION:
    {
        int Id = value.asFunctionId();
        OsPrintf("<function %d>", Id);
        break;
    }
    case ValueType::NATIVE:
        OsPrintf("<native>");
        break;
    case ValueType::PROCESS:
        OsPrintf("<process>");
        break;
    case ValueType::PROCESS_INSTANCE:
        OsPrintf("<process_instance>");
        break;

    case ValueType::ARRAY:
    {
        ArrayInstance *arr = value.asArray();
        OsPrintf("[");
        for (int i = 0; i < (int)arr->values.size(); i++)
        {
            printValue(arr->values[i]);
            if (i < (int)arr->values.size() - 1)
                OsPrintf(", ");
        }
        OsPrintf("]");
        break;
    }
    case ValueType::BUFFER:
    {
        BufferInstance *buffer = value.asBuffer();
        OsPrintf("[");
        OsPrintf("0x%08x", buffer->data);
        OsPrintf("]");
        break;
    }
    case ValueType::MAP:
    {

        MapInstance *map = value.asMap();
        OsPrintf("{");

        int i = 0;
        map->table.forEach([&](String *key, Value val)
                           {
        if (i > 0) OsPrintf(", ");
        OsPrintf("%s: ", key->chars());
        printValue(val);
        i++; });

        OsPrintf("}");
        break;
    }

    case ValueType::STRUCT:
    {
        int Id = value.asStructId();
        OsPrintf("<struct %d>", Id);
        break;
    }
    case ValueType::STRUCTINSTANCE:
    {
        StructInstance *instance = value.as.sInstance;
        OsPrintf("struct '%s' [", instance->def->name->chars());

        bool first = true;

        instance->def->names.forEach([&](String *key, int fieldIndex)
                                     {
             if (!first) 
            {
                OsPrintf(", ");
            }
            first = false;

            OsPrintf("%s = ", key->chars());
            printValue(instance->values[fieldIndex]); });

        OsPrintf("]\n");
        break;
    }
    case ValueType::CLASS:
    {
        int classId = value.asClassId();
        OsPrintf("<class %d>", classId);
        break;
    }
    case ValueType::CLASSINSTANCE:
    {
        ClassInstance *inst = value.asClassInstance();
        OsPrintf("<instance %s>", inst->klass->name->chars());
        break;
    }
    case ValueType::NATIVECLASSINSTANCE:
    {
        NativeClassInstance *inst = value.as.sClassInstance;
        OsPrintf("<native_instance %s>", inst->klass->name->chars());
        break;
    }
    case ValueType::NATIVESTRUCTINSTANCE:
    {
        NativeStructInstance *inst = value.as.sNativeStruct;
        OsPrintf("<native_struct_instance %s>", inst->def->name->chars());
        break;
    }
    case ValueType::POINTER:
    {

        OsPrintf("<pointer %p>", value.as.pointer);
        break;
    }
    case ValueType::MODULEREFERENCE:
    {
        OsPrintf("<module_reference %d %d %d>", value.as.unsignedInteger >> 24, (value.as.unsignedInteger >> 12) & 0xFFF, value.as.unsignedInteger & 0xFFF);
        break;
    }
    case ValueType::NATIVESTRUCT:
    {
        OsPrintf("<native_struct>");
        break;
    }
    case ValueType::NATIVECLASS:
    {
        OsPrintf("<native_class>");
        break;
    }
    case ValueType::CLOSURE:
    {
        OsPrintf("<closure>");
        break;
    }
 
    default:
    {
        const char* str = valueTypeToString(value.type);
        OsPrintf("<?%s?>", str);
        break;
    }
    }
}

void printValueNl(const Value &value)
{
    printValue(value);
    OsPrintf("\n");
}

void valueToBuffer(const Value &v, char *out, size_t size)
{
    switch (v.type)
    {
    case ValueType::NIL:
        snprintf(out, size, "nil");
        break;
    case ValueType::BOOL:
        snprintf(out, size, "%s", v.as.boolean ? "true" : "false");
        break;
    case ValueType::BYTE:
        snprintf(out, size, "%u", v.as.byte);
        break;
    case ValueType::INT:
        snprintf(out, size, "%d", v.as.integer);
        break;
    case ValueType::UINT:
        snprintf(out, size, "%u", v.as.unsignedInteger);
        break;
    case ValueType::FLOAT:
        snprintf(out, size, "%.4f", v.as.real);
        break;
    case ValueType::DOUBLE:
        snprintf(out, size, "%.4f", v.as.number);
        break;
    case ValueType::STRING:
        snprintf(out, size, "%s", v.as.string->chars());
        break;
    case ValueType::ARRAY:
        snprintf(out, size, "[array]");
        break;
    case ValueType::MAP:
        snprintf(out, size, "{map}");
        break;
    case ValueType::STRUCT:
        snprintf(out, size, "<struct>");
    case ValueType::STRUCTINSTANCE:
        snprintf(out, size, "<struct_instance>");
    case ValueType::CLASS:
        snprintf(out, size, "<class>");
    case ValueType::CLASSINSTANCE:
        snprintf(out, size, "<class_instance>");
    case ValueType::NATIVECLASSINSTANCE:
        snprintf(out, size, "<native_class_instance>");
    case ValueType::NATIVESTRUCTINSTANCE:
        snprintf(out, size, "<native_struct_instance>");
    case ValueType::POINTER:
        snprintf(out, size, "<pointer>");
    case ValueType::MODULEREFERENCE:
        snprintf(out, size, "<module_reference>");
    case ValueType::NATIVESTRUCT:
        snprintf(out, size, "<native_struct>");
    default:
        snprintf(out, size, "<object>");
    }
}
