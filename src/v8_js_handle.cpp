#include <v8.h>
#include <iostream>

#include "js_handle.h"
#include "v8_js_handle.h"
#include "mapper.h"


using namespace std;
using namespace meguro;
using namespace v8;

/* Prototypes */
static Handle<String> read_file(const string& name);
static Handle<Value> emit(const Arguments& args);

/* Public Implementation */
V8JSHandle::V8JSHandle(Mapper* mapper,const string& js_file)
{
  HandleScope handle_scope;
  global_ = ObjectTemplate::New();
  Handle<Context> context = Context::New(NULL,global_);
  context_ = Persistent<Context>::New(context);
  Context::Scope context_scope(context);
  TryCatch try_catch;

  Handle<ObjectTemplate> meguro_template = ObjectTemplate::New();
  meguro_template->SetInternalFieldCount(1);
  meguro_template->Set(String::New("emit"),FunctionTemplate::New(emit));

  Local<Object> meguro_instance = meguro_template->NewInstance();
  meguro_instance->SetPointerInInternalField(0, mapper);
  context_->Global()->Set(String::New("Meguro"),meguro_instance);


  // Compile the script and check for errors.
  Handle<String> source = read_file(js_file);
  Handle<Script> compiled_script = Script::Compile(source);
  if (compiled_script.IsEmpty()) {
    String::Utf8Value error(try_catch.Exception());
    printf("Logged:%s\n",*error);
    // The script failed to compile; bail out.
  }

  Handle<Value> result = compiled_script->Run();
  if (result.IsEmpty()) {
    // The TryCatch above is still in effect and will have caught the error.
    String::Utf8Value error(try_catch.Exception());
    printf("Logged:%s\n",*error);
    // Running the script failed; bail out.
  }

  Handle<String> process_name = String::New("map");
  Handle<Value> process_val = context->Global()->Get(process_name);

  if (!process_val->IsFunction()) {
    printf("map is not a function in the JS file\n");
  } else {
    Handle<Function> process_fun = Handle<Function>::Cast(process_val);
    map_fn_ = Persistent<Function>::New(process_fun);
  }

  Handle<String> init_name = String::New("init");
  Handle<Value> init_val = context->Global()->Get(process_name);
}

string
V8JSHandle::map(const string& key, const string& value)
{
  HandleScope handle_scope;

  Context::Scope context_scope(context_);
  TryCatch try_catch;
  int argc = 2;
  Handle<Value> argv[2] = {
    String::New(key.c_str()),
    String::New(value.c_str()),
  };

  Handle<Value> result = map_fn_->Call(context_->Global(), argc,argv);
  if (result.IsEmpty()) {
    String::Utf8Value error(try_catch.Exception());
    printf("Logged:%s\n",*error);
    return "";
  } else {
    string res = *(String::Utf8Value(result));
    return res;
  }
}

string
V8JSHandle::reduce(const string& key, const string& value)
{
  return "";
}

/* Private Non Class Implementation */

// Reads a file into a v8 string.
static Handle<String> read_file(const string& name) {
  FILE* file = fopen(name.c_str(), "rb");
  if (file == NULL) return Handle<String>();

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = fread(&chars[i], 1, size - i, file);
    i += read;
  }
  fclose(file);
  Handle<String> result = String::New(chars, size);
  delete[] chars;
  return result;
}

static Handle<Value> emit(const Arguments& args)
{
  if (args.Length() < 2) 
    return v8::Undefined();

  HandleScope scope;

  Local<Object> self = args.This();
  Mapper* mapper = (Mapper*) self->GetPointerFromInternalField(0);
  Handle<Value> key_arg = args[0];
  Handle<Value> value_arg = args[1];
  String::Utf8Value key(key_arg);
  String::Utf8Value value(value_arg);
  mapper->emit(*key,*value);
  return v8::Undefined();
}
