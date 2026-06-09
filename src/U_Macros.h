#pragma once

#define SET_NULL_DELETE(obj) delete obj; obj = nullptr;
#define SET_NULL_DELETE_ARRAY(obj) delete[] obj; obj = nullptr;
#define EMPTY_OBJECT(obj, name) obj name = obj();
#define RETURN_EMPTY_OBJECT(obj) obj __ret_empty = obj(); return __ret_empty;
#define CHECK_NULL_DELETE(obj) if(obj) { delete obj; }
#define CHECK_NULL_DELETE_ARRAY(obj) if(obj) { delete[] obj; }
#define SAFE_DELETE(obj) if(obj) { delete obj; }
#define SAFE_DELETE_ARRAY(obj) if(obj) { delete[] obj; }
#define SAFE_DELETE_SET_NULL(obj) if(obj) { delete obj; obj = nullptr; }
#define SAFE_DELETE_ARRAY_SET_NULL(obj) if(obj) { delete[] obj; obj = nullptr; }
#define VOID_TO_CLASS(var, class) reinterpret_cast<class>(var)
#define CLASS_TO_VOID(var) reinterpret_cast<void*>(var)
#define SAFE_CALL(ptr, call) if(ptr) { ptr->call; }
#define SAFE_CALL_SINGLE(ptr) if(ptr) { ptr; }
#define SAFE_GET(var, ptr, call) if(ptr) { var = ptr->call; }
#define SAFE_GET_SINGLE(var, ptr) if(ptr) { var = ptr; }
