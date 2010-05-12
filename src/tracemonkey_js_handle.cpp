#include <string.h>

#include "tracemonkey_js_handle.h"
#include "mapper.h"
#include "reducer.h"
#include "dictionary.h"

using namespace meguro;

/* Globals */

/* The class of the global object. */
static JSClass __global_class = {
  "global", JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass __meguro_class = {
  "Meguro", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};


/* Prototypes */
void reportError(JSContext *cx, const char *message, JSErrorReport *report);
static JSBool emit(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
static JSBool emit_noop(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
static JSBool slog(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
static JSBool sset(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
static JSBool save(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
static JSBool dictionary(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

//-------------------------------------------
//-- P u b l i c  I m p l e m e n t a t i o n
//-------------------------------------------
TraceMonkeyJSHandle::TraceMonkeyJSHandle(const MeguroEnvironment* env) : JSHandle(env)
{
  /* This is for first time initialization (kind of hacky yes) */
  if (!tracemonkey_initialized) {
    JS_SetCStringsAreUTF8();
    tracemonkey_initialized = true;
  }
  rt_ = JS_NewRuntime(env->runtime_memory_size);
  if (rt_ == NULL)
    throw JSHandleException("TraceMonkey Runtime Init Error");

  ctx_ = JS_NewContext(rt_,8192);
  if (ctx_ == NULL) 
    throw JSHandleException("TraceMonkey Context Init Error");

  JS_BeginRequest(ctx_);
  JS_SetOptions(ctx_, JSOPTION_VAROBJFIX);
  JS_SetVersion(ctx_, JSVERSION_LATEST);
  JS_SetErrorReporter(ctx_, reportError);

  /* Create the global object. */
  global_ = JS_NewObject(ctx_, &__global_class, NULL, NULL);
  if (global_ == NULL)
    throw JSHandleException("TraceMonkey Global Error");

  /* Populate the global object with the standard globals,
   *        like Object and Array. */
  if (!JS_InitStandardClasses(ctx_, global_))
    throw JSHandleException("TraceMonkey Init Standard Classes Error");

  meguro_obj_ = JS_DefineObject(ctx_,global_,"Meguro",&__meguro_class,NULL,NULL);
  if (!JS_AddNamedRoot(ctx_,&meguro_obj_, "Meguro namespace object"))
    throw JSHandleException("Error saving named Meguro root");

  JSScript* script = JS_CompileFile(ctx_,JS_GetGlobalObject(ctx_), env->js_file);
  if (script == NULL) {
    throw JSHandleException("Tracemonkey Compile Error");
  }

  script_ = JS_NewScriptObject(ctx_,script);
  if (script_ == NULL)
    JS_DestroyScript(ctx_,script);

  if (!JS_AddNamedRoot(ctx_, &script_, "map/reduce script object"))
    fprintf(stderr,"Error saving named script root\n");

  jsval result;
  if (!JS_ExecuteScript(ctx_,global_, script, &result)) {
    throw JSHandleException("Tracemonkey Execute Error");
  }

  if (env->optimize_bucket_count) {
    JSFunction* emit_fn = JS_DefineFunction(ctx_,meguro_obj_,"emit",emit_noop, 2, 0);
    if (!emit_fn) 
      throw JSHandleException("Error Defining Mapper Emit");
  } else {
    JSFunction* emit_fn = JS_DefineFunction(ctx_,meguro_obj_,"emit",emit, 2, 0);
    if (!emit_fn)
      throw JSHandleException("Error Defining Mapper Emit");
  }

  JSFunction* set_fn = JS_DefineFunction(ctx_,meguro_obj_, "set", sset, 2,0);
  if (!set_fn)
    throw JSHandleException("Error Defining Mapper Set");

  JSFunction* log_fn = JS_DefineFunction(ctx_,meguro_obj_,"log",slog,1, 0);
  if (!log_fn)
    throw JSHandleException("Error Defining Log Function");

  JSFunction* save_fn = JS_DefineFunction(ctx_,meguro_obj_,"save",save,2, 0);
  if (!save_fn)
    throw JSHandleException("Error Defining Reduce Save Function");

  JSFunction* dict_fn = JS_DefineFunction(ctx_,meguro_obj_,"dictionary",dictionary,1, 0);
  if (!dict_fn)
    throw JSHandleException("Error defining dictionary function");

  if (!JS_SetPrivate(ctx_,meguro_obj_,this))
    throw JSHandleException("Tracemonkey private data error");
}

bool 
TraceMonkeyJSHandle::has_reduce_fn()
{
  jsval fn;
  JSBool ok = JS_LookupProperty(ctx_,global_,"reduce",&fn);
  if (ok) {
    if (JSVAL_IS_OBJECT(fn)) {
      JSObject* fn_obj = JSVAL_TO_OBJECT(fn);
      JSBool is_fn = JS_ObjectIsFunction(ctx_,fn_obj);
      return (is_fn == JS_TRUE);
    }
  } 
  return false;
}

bool 
TraceMonkeyJSHandle::has_map_fn()
{
  jsval fn;
  JSBool ok = JS_LookupProperty(ctx_,global_,"map",&fn);
  if (ok) {
    if (JSVAL_IS_OBJECT(fn)) {
      JSObject* fn_obj = JSVAL_TO_OBJECT(fn);
      JSBool is_fn = JS_ObjectIsFunction(ctx_,fn_obj);
      return (is_fn == JS_TRUE);
    }
  } 
  return false;
}

void
TraceMonkeyJSHandle::map(const string& key, const string& value)
{
  jsval argv[2];
  jsval result;
  JSBool ok;

  char* key_dup = JS_strdup(ctx_,key.c_str());
  if (!key_dup) {
    throw JSHandleException("Out of Memory");
  }
  char* val_dup = JS_strdup(ctx_,value.c_str());
  if (!val_dup) {
    free(key_dup);
    throw JSHandleException("Out of Memory");
  }

  JSString* key_jstr = JS_NewString(ctx_,key_dup,key.length());
  if (key_jstr) {
    argv[0] = STRING_TO_JSVAL(key_jstr);
  } else {
    free(key_dup);
    throw JSHandleException("Unable to allocate string in TraceMonkey");
  }


  JSString* value_jstr = JS_NewString(ctx_,val_dup,value.length());
  if (value_jstr) {
    argv[1] = STRING_TO_JSVAL(value_jstr);
  } else {
    free(val_dup);
    throw JSHandleException("Unable to allocate string in TraceMonkey");
  }

  ok = JS_CallFunctionName(ctx_, global_, "map", 2, argv, &result);
}

void
TraceMonkeyJSHandle::reduce(const string& key, const vector<char*>& values)
{
  jsval argv[2] = {0};
  jsval result = NULL;
  JSBool ok = false;

  JSObject* array = JS_NewArrayObject(ctx_,0,NULL);
  if (!array)
    throw JSHandleException("Unable to allocate array in Tracemonkey");
  //JS_AddRoot(ctx_,array);

  int idx = 0;
  char* str = NULL;
  for(vector<char*>::const_iterator ii = values.begin(); ii != values.end(); ii++) {
    str = *ii;
    char* str_dup = JS_strdup(ctx_,str);
    JSString* elem_str = JS_NewString(ctx_,str_dup,strlen(str_dup));
    if (elem_str) {
      jsval elem = STRING_TO_JSVAL(elem_str);
      JS_SetElement(ctx_,array,idx,&elem);
      idx++;
    } else {
      free(str_dup);
      throw JSHandleException("Unable to allocate string in TraceMonkey");
    }
  }

  char* key_dup = JS_strdup(ctx_,key.c_str());
  if (!key_dup) {
    throw JSHandleException("Out of memory");
  }

  JSString* key_jstr = JS_NewString(ctx_,key_dup,key.length());
  if (key_jstr) {
    argv[0] = STRING_TO_JSVAL(key_jstr);
  } else {
    free(key_dup);
    throw JSHandleException("Unable to allocate string in TraceMonkey");
  }

  argv[1] = OBJECT_TO_JSVAL(array);
  ok = JS_CallFunctionName(ctx_, global_, "reduce", 2, argv, &result);
}

void 
TraceMonkeyJSHandle::log(const string& msg)
{
  printf(" Meguro: %s\n",msg.c_str());
}

void 
TraceMonkeyJSHandle::mapper(Mapper* mapper)
{
  mapper_ = mapper;
}

void 
TraceMonkeyJSHandle::reducer(Reducer* reducer)
{
  reducer_ = reducer;
}

TraceMonkeyJSHandle::~TraceMonkeyJSHandle()
{
  JS_RemoveRoot(ctx_,&meguro_obj_);
  JS_RemoveRoot(ctx_,&script_);
  JS_EndRequest(ctx_);
  JS_DestroyContext(ctx_);
  JS_DestroyRuntime(rt_);
}

/* The error reporter callback. */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
  fprintf(stderr, "ctx:%p:%s:%u:%s\n",
    cx,report->filename ? report->filename : "<meguro js>",
    (unsigned int) report->lineno,
    message);
}

static JSBool emit(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)
{
  JSBool ok;

  const char* key, *val;
  ok = JS_ConvertArguments(ctx,argc,argv,"ss",&key,&val);

  JSHandle* handle = (JSHandle*) JS_GetPrivate(ctx,obj);
  Mapper* mapper = handle->mapper();
  if (mapper) {
    int rc = JS_SuspendRequest(ctx);
    mapper->emit(key,val);
    JS_ResumeRequest(ctx,rc);
  }

  *rval = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool slog(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)
{
  JSBool ok;

  const char* msg;
  ok = JS_ConvertArguments(ctx,argc,argv,"s",&msg);

  JSHandle* handle = (JSHandle*) JS_GetPrivate(ctx,obj);
  handle->log(msg);

  *rval = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool save(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)
{
  JSBool ok;

  const char* key, *val;
  ok = JS_ConvertArguments(ctx,argc,argv,"ss",&key,&val);

  JSHandle* handle = (JSHandle*) JS_GetPrivate(ctx,obj);
  Reducer* reducer = handle->reducer();
  if (reducer) {
    int rc = JS_SuspendRequest(ctx);
    reducer->save(key,val);
    JS_ResumeRequest(ctx,rc);
  }

  *rval = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool dictionary(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)
{
  JSBool ok;

  const char* key;
  ok = JS_ConvertArguments(ctx,argc,argv,"s",&key);
  JSHandle* handle = (JSHandle*) JS_GetPrivate(ctx,obj);
  Dictionary* dict = handle->env()->dictionary;
  if (dict) {
    const char* value = dict->get(key);
    if (value) {
      *rval = STRING_TO_JSVAL(JS_NewString(ctx,JS_strdup(ctx,value),strlen(value)));
    } else {
      *rval = JSVAL_NULL;
    }
  } else {
    *rval = JSVAL_NULL;
  }
  return JS_TRUE;
}

static JSBool emit_noop(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)
{
  JSBool ok;

  const char* key, *val;
  ok = JS_ConvertArguments(ctx,argc,argv,"ss",&key,&val);

  JSHandle* handle = (JSHandle*) JS_GetPrivate(ctx,obj);
  Mapper* mapper = handle->mapper();
  if (mapper) {
    int rc = JS_SuspendRequest(ctx);
    mapper->emit_noop(key,val);
    JS_ResumeRequest(ctx,rc);
  }

  *rval = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool sset(JSContext* ctx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)
{
  JSBool ok;

  const char* key, *val;
  ok = JS_ConvertArguments(ctx,argc,argv,"ss",&key,&val);

  JSHandle* handle = (JSHandle*) JS_GetPrivate(ctx,obj);
  Mapper* mapper = handle->mapper();
  if (mapper) {
    int rc = JS_SuspendRequest(ctx);
    mapper->set(key,val);
    JS_ResumeRequest(ctx,rc);
  }

  *rval = JSVAL_VOID;
  return JS_TRUE;
}

bool TraceMonkeyJSHandle::tracemonkey_initialized = false;
