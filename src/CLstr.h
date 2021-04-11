/* date = April 11th 2021 4:51 pm */

#ifndef CL_STR_H
#define CL_STR_H

struct CLStr {
    u8* str;
    u64 capacity;
    u64 length;
};

CLStr StrMake(char* cStr) {
    CLStr str;
    
    str.str      = (u8*) cStr;
    str.length   = strlen(cStr);
    str.capacity = str.length + 1;
    
    return str;
}

CLStr StrMakeBuffer(char* buffer, i32 bufferSize) {
    CLStr str;
    
    str.str      = (u8*) buffer;
    str.length   = 0;
    str.capacity = bufferSize;
    
    return str;
}

#endif //CL_STR_H
