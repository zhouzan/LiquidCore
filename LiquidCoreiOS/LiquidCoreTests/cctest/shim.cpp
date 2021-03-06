/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "JSObjectRefPrivate.h"

#include "test/cctest/heap/heap-tester.h"
#include "test/cctest/heap/heap-utils.h"
#include "test/cctest/profiler-extension.h"
#include "test/cctest/print-extension.h"
#include "test/cctest/trace-extension.h"

#ifdef __cplusplus
extern "C" {
#endif
    void JSSynchronousGarbageCollectForDebugging(JSContextRef);
#ifdef __cplusplus
}
#endif

using namespace v8;

void CpuProfiler::Dispose() {}
void CpuProfiler::StartProfiling(Local<String> string, bool b) {}
CpuProfiler * CpuProfiler::New(Isolate* isolate) { return nullptr; }

void debug::SetDebugDelegate(Isolate* isolate, v8::debug::DebugDelegate* listener) {}

using namespace v8::internal;

//
// internal::Factory
//

// Creates a new on-heap JSTypedArray.
internal::Handle<JSTypedArray> Factory::NewJSTypedArray(ElementsKind elements_kind,
                                                        size_t number_of_elements,
                                                        PretenureFlag pretenure)
{
    assert(0);
    return Handle<JSTypedArray>();
}
internal::Handle<internal::String> Factory::InternalizeOneByteString(Vector<const uint8_t> str)
{
    assert(0);
    return Handle<internal::String>();
}

internal::Handle<FixedArray> Factory::NewFixedArray(int size, PretenureFlag pretenure)
{
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::EscapableHandleScope scope(isolate);
    MaybeLocal<v8::Array> array = Array::New(isolate);
    JSValueRef a = V82JSC::ToJSValueRef(array.ToLocalChecked(), isolate->GetCurrentContext());
    for (int i=0; i<size; i++) {
        JSObjectSetPropertyAtIndex(V82JSC::ToContextRef(isolate->GetCurrentContext()),
                                   (JSObjectRef)a, i,
                                   JSValueMakeNumber(V82JSC::ToContextRef(isolate->GetCurrentContext()), i),
                                   0);
    }
    if (array.IsEmpty()) return Handle<internal::FixedArray>();
    Local<v8::Array> local = scope.Escape(array.ToLocalChecked());
    Handle<internal::FixedArray> handle = * reinterpret_cast<Handle<FixedArray>*>(&local);
    return handle;
}

MaybeHandle<internal::String> Factory::NewExternalStringFromOneByte(const ExternalOneByteString::Resource* resource)
{
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::EscapableHandleScope scope(isolate);
    MaybeLocal<v8::String> string = v8::String::NewExternalOneByte(isolate, const_cast<ExternalOneByteString::Resource*>(resource));
    if (string.IsEmpty()) return MaybeHandle<internal::String>();
    Local<v8::String> local = scope.Escape(string.ToLocalChecked());
    MaybeHandle<internal::String> handle = * reinterpret_cast<Handle<String>*>(&local);
    return handle;
}

//
// internal::String
//
template <>
void internal::String::WriteToFlat(internal::String* source, unsigned short* sink, int from, int to)
{
    StringImpl* impl = reinterpret_cast<StringImpl*>(reinterpret_cast<intptr_t>(source) - internal::kHeapObjectTag);
    JSStringRef string = JSValueToStringCopy(impl->GetNullContext(), impl->m_value, 0);
    const uint16_t * str = JSStringGetCharactersPtr(string);
    memcpy(sink, &str[from], (to-from)*sizeof(uint16_t));
}
internal::Handle<internal::String> internal::String::SlowFlatten(Handle<ConsString> cons,
                           PretenureFlag tenure)
{
    assert(0);
    return Handle<String>();
}
bool internal::String::SlowEquals(internal::String* other)
{
    assert(0);
    return false;
}

bool internal::String::SlowAsArrayIndex(uint32_t* index)
{
    assert(0);
    return false;
}

bool internal::String::MakeExternal(v8::String::ExternalStringResource* resource)
{
    StringImpl* impl = reinterpret_cast<StringImpl*>(reinterpret_cast<intptr_t>(this) - internal::kHeapObjectTag);
    v8::HandleScope scope(V82JSC::ToIsolate(impl->GetIsolate()));
    Local<v8::String> string = V82JSC::CreateLocal<v8::String>(&impl->GetIsolate()->ii, impl);
    return string->MakeExternal(resource);
}

internal::Handle<internal::String> StringTable::LookupString(Isolate* isolate, internal::Handle<internal::String> string)
{
    ValueImpl *impl = reinterpret_cast<ValueImpl *>(V82JSC_HeapObject::FromHeapPointer(*string));
    impl->m_map = V82JSC_HeapObject::ToV8Map(reinterpret_cast<IsolateImpl*>(isolate)->m_internalized_string_map);
    return string;
}

//
// internal::Heap
//

// Returns of size of all objects residing in the heap.
size_t Heap::SizeOfObjects()
{
    V82JSC_HeapObject::HeapImpl *heapimpl = reinterpret_cast<V82JSC_HeapObject::HeapImpl*>(this);

    return heapimpl->m_allocated;
}

// Performs garbage collection operation.
// Returns whether there is a chance that another major GC could
// collect more garbage.
bool Heap::CollectGarbage(AllocationSpace space, GarbageCollectionReason gc_reason,
                          const GCCallbackFlags gc_callback_flags)
{
    reinterpret_cast<v8::Isolate*>(isolate())->
    RequestGarbageCollectionForTesting(v8::Isolate::GarbageCollectionType::kFullGarbageCollection);
    return false;
}

// Performs a full garbage collection.  If (flags & kMakeHeapIterableMask) is
// non-zero, then the slower precise sweeper is used, which leaves the heap
// in a state where we can iterate over the heap visiting all objects.
void Heap::CollectAllGarbage(int flags, GarbageCollectionReason gc_reason,
                             const GCCallbackFlags gc_callback_flags)
{
    CollectGarbage((v8::internal::AllocationSpace)0, gc_reason, gc_callback_flags);
}

// Last hope GC, should try to squeeze as much as possible.
void Heap::CollectAllAvailableGarbage(GarbageCollectionReason gc_reason)
{
    CollectAllGarbage(0, GarbageCollectionReason::kDebugger);
}

bool Heap::ShouldOptimizeForMemoryUsage()
{
    return reinterpret_cast<IsolateImpl*>(isolate_)->m_should_optimize_for_memory_usage;
}

// Start incremental marking and ensure that idle time handler can perform
// incremental steps.
void Heap::StartIdleIncrementalMarking(GarbageCollectionReason gc_reason,
                                       GCCallbackFlags gc_callback_flags)
{
    
}

//
// internal::HeapIterator
//
/* This is hack-a-palooza.  This only used by the tests to iterate searching for global
 * objects in the heap.  So we've hacked it to return non-null for each global context (global
 * objects in JSC can't live if their global context has been destroyed).  Do not use this
 * for anything else.  Also, note this assumes there is only one isolate.  Seriously, don't use
 * for anything other than this one purpose.
 */
HeapIterator::HeapIterator(Heap* heap, HeapObjectsFiltering filtering)
{
    filter_ = nullptr; // We are reusing this pointer to hold our iterator (ick)
    heap_ = heap;
}
HeapIterator::~HeapIterator()
{
    auto it = reinterpret_cast<std::map<JSGlobalContextRef, IsolateImpl*>::iterator*>(filter_);
    if (it) {
        delete it;
    }
}
HeapObject* HeapIterator::next()
{
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(heap_->isolate());
    auto it = reinterpret_cast<std::map<JSGlobalContextRef, Copyable(v8::Context)>::iterator*>(filter_);

    if (it == nullptr) {
        it = new std::map<JSGlobalContextRef, Copyable(v8::Context)>::iterator();
        *it = iso->m_global_contexts.begin();
        filter_ = reinterpret_cast<HeapObjectsFilter*>(it);
    } else {
        ++(*it);
    }
    if (*it == iso->m_global_contexts.end()) {
        return nullptr;
    } else {
        return reinterpret_cast<HeapObject*>(filter_);
    }
}

//
// internal::heap
//
void heap::SimulateFullSpace(v8::internal::NewSpace* space,
                       std::vector<Handle<FixedArray>>* out_handles)
{
    //FIXME: assert(0);
}

// Helper function that simulates a full old-space in the heap.
void heap::SimulateFullSpace(v8::internal::PagedSpace* space)
{
    printf( "FIXME! heap::SimulateFullSpace\n" );
}

//
// internal::IncrementalMarking
//

void IncrementalMarking::RecordWriteSlow(HeapObject* obj, Object** slot, Object* value)
{
    assert(0);
    *slot = nullptr;
}

//
// Allocation
//
void internal::FatalProcessOutOfMemory(const char* message)
{
    assert(0);
}

char* internal::StrDup(const char* str)
{
    return strdup(str);
}

//
// internal::LookupIterator
//
template <bool is_element>
void LookupIterator::Start() {
    //assert(0);
}
template void LookupIterator::Start<false>();
template void LookupIterator::Start<true>();

internal::Handle<JSReceiver> LookupIterator::GetRootForNonJSReceiver(Isolate* isolate,
                                                           Handle<Object> receiver, uint32_t index)
{
    assert(0);
    return Handle<JSReceiver>();
}
internal::Handle<internal::Object> LookupIterator::GetAccessors() const {
    assert(0);
    return Handle<internal::Object>();
}

//
// internal::Isolate
//
bool internal::Isolate::IsFastArrayConstructorPrototypeChainIntact()
{
    assert(0);
    return false;
}
MaybeHandle<JSPromise> internal::Isolate::RunHostImportModuleDynamicallyCallback(
                                                              Handle<String> referrer, Handle<Object> specifier)
{
    assert(0);
    return MaybeHandle<JSPromise>();
}
base::RandomNumberGenerator* internal::Isolate::random_number_generator()
{
    assert(0);
    return nullptr;
}

//
// internal::CpuFeatures
//
void CpuFeatures::PrintTarget() {assert(0);}
void CpuFeatures::PrintFeatures() {assert(0);}
// Platform-dependent implementation.
void CpuFeatures::ProbeImpl(bool cross_compile) {assert(0);}
bool CpuFeatures::initialized_ = false;

//
// Utils
//
void PRINTF_FORMAT(1, 2) internal::PrintF(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
void PRINTF_FORMAT(2, 3) internal::PrintF(FILE* out, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(out, format, args);
    va_end(args);
}
// Safe formatting print. Ensures that str is always null-terminated.
// Returns the number of chars written, or -1 if output was truncated.
int PRINTF_FORMAT(2, 3) internal::SNPrintF(Vector<char> str, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int r = vsnprintf(str.start(), str.size(), format, args);
    va_end(args);
    return r;
}

//
// internal::CpuProfiler
//
void internal::CpuProfiler::DeleteAllProfiles()
{
    printf ("FIXME! internal::CpuProfiler::DeleteAllProfiles()\n");
}

//
// internal::Builtins
//
internal::Handle<Code> Builtins::InterpreterEnterBytecodeDispatch()
{
    assert(0);
    return Handle<Code>();
}
internal::Handle<Code> Builtins::InterpreterEnterBytecodeAdvance()
{
    assert(0);
    return Handle<Code>();
}
internal::Handle<Code> Builtins::InterpreterEntryTrampoline()
{
    assert(0);
    return Handle<Code>();
}

//
// internal::Object
//
// Returns the permanent hash code associated with this object. May return
// undefined if not yet created.
internal::Object* internal::Object::GetHash()
{
    if (this->IsSmi()) return this;

    ValueImpl* impl = (ValueImpl*)(reinterpret_cast<uintptr_t>(this) & ~3);
    Local<v8::Context> context = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(impl))->GetCurrentContext();
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef value = impl->m_value;
    
    if (JSValueIsObject(ctx, value)) {
        TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef) value);
        if (!wrap) wrap = makePrivateInstance(V82JSC::ToIsolateImpl(impl), ctx, (JSObjectRef) value);
        return Smi::FromInt(wrap->m_hash);
    }
    return Smi::FromInt(JSValueToNumber(ctx, value, 0));
}

// Returns the permanent hash code associated with this object depending on
// the actual object type. May create and store a hash code if needed and none
// exists.
Smi* internal::Object::GetOrCreateHash(Isolate* isolate, Handle<Object> object)
{
    if ((*object.location())->IsSmi()) return (reinterpret_cast<Smi*>(*object.location()));
    
    HandleScope scope(isolate);
    
    Local<v8::Object> o = V82JSC::__local<v8::Object>(object.location()).toLocal();
    Local<v8::Context> context = V82JSC::ToCurrentContext(*o);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef value = V82JSC::ToJSValueRef(o, context);
    if (JSValueIsObject(ctx, value)) {
        int hash = o->GetIdentityHash();
        return Smi::FromInt(hash);
    }
    return nullptr;
}

MaybeHandle<internal::Object> internal::Object::GetProperty(LookupIterator* it)
{
    internal::Object *o = *it->GetReceiver();
    Local<v8::Object> obj = V82JSC::__local<v8::Object>(&o).toLocal();
    Local<v8::Context> context = V82JSC::ToCurrentContext(*obj);
    MaybeLocal<v8::Value> val = obj->Get(context, it->index());
    if (val.IsEmpty()) {
        return MaybeHandle<Object>();
    }
    return Handle<Object>(HandleScope::GetHandle(reinterpret_cast<internal::Isolate*>(V82JSC::ToIsolate(*obj)),
                                                 * reinterpret_cast<internal::Object**>(*val.ToLocalChecked())));
}

//
// internal::MessageHandler
//
internal::Handle<JSMessageObject> MessageHandler::MakeMessageObject(
                                                 Isolate* isolate, MessageTemplate::Template type,
                                                 const MessageLocation* location, Handle<Object> argument,
                                                 Handle<FixedArray> stack_frames)
{
    assert(0);
    return Handle<JSMessageObject>();
}
// Report a formatted message (needs JS allocation).
void MessageHandler::ReportMessage(Isolate* isolate, const MessageLocation* loc,
                          Handle<JSMessageObject> message)
{
    assert(0);
}

//
// internal::AccountingAllocator
//
AccountingAllocator::AccountingAllocator() {}
AccountingAllocator::~AccountingAllocator() {}
// Gets an empty segment from the pool or creates a new one.
Segment* AccountingAllocator::GetSegment(size_t bytes)
{
    assert(0);
    return nullptr;
}
// Return unneeded segments to either insert them into the pool or release
// them if the pool is already full or memory pressure is high.
void AccountingAllocator::ReturnSegment(Segment* memory)
{
    assert(0);
}

//
// internal::Zone
//
Zone::Zone(AccountingAllocator* allocator, const char* name) {assert(0);}
Zone::~Zone() {assert(0);}

//
// internal::CompilationCache
//
void CompilationCache::Clear()
{
    printf("FIXME! CompliationCache::Clear()\n");
}

//
// internal::V8
//
extern Platform* currentPlatform;
Platform* internal::V8::GetCurrentPlatform() { return currentPlatform; }

//
// Extensions
//
Local<FunctionTemplate> TraceExtension::GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Local<v8::String> name)
{
    assert(0);
    return Local<FunctionTemplate>();
}
const char* TraceExtension::kSource = "";
//
// internal::TimedHistogram
//
// Start the timer. Log if isolate non-null.
void TimedHistogram::Start(base::ElapsedTimer* timer, Isolate* isolate) {assert(0);}
void TimedHistogram::Stop(base::ElapsedTimer* timer, Isolate* isolate) {assert(0);}

//
// internal::Script
//
// Init line_ends array with source code positions of line ends.
void internal::Script::InitLineEnds(Handle<Script> script) { assert(0); }
bool internal::Script::GetPositionInfo(int position, PositionInfo* info,
                     OffsetFlag offset_flag) const
{
    assert(0);
    return false;
}

//
// internal::ScriptData
//
ScriptData::ScriptData(const byte* data, int length)
{
    this->data_ = data;
    this->length_ = length;
}

//
// internal::FixedArrayBase
//
bool FixedArrayBase::IsCowArray() const { assert(0); return false; }

//
// internal::JSReceiver
//
internal::Handle<internal::Context> JSReceiver::GetCreationContext()
{
    assert(IsHeapObject());
    ValueImpl *impl = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(this) - internal::kHeapObjectTag);
    IsolateImpl* i = V82JSC::ToIsolateImpl(impl);
    v8::HandleScope scope(V82JSC::ToIsolate(i));
    Local<v8::Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(i));
    JSGlobalContextRef ctx = JSContextGetGlobalContext(V82JSC::ToContextRef(context));
    assert(i->m_global_contexts.count(ctx) == 1);
    context = i->m_global_contexts[ctx].Get(V82JSC::ToIsolate(i));
    Handle<Context> c = * reinterpret_cast<Handle<Context>*>(&context);
    return c;
}

//
// internal::StackGuard
//
void StackGuard::RequestInterrupt(InterruptFlag flag) { /*assert(0);*/ }

//
// internal:MarkCompactCollector
//
void MarkCompactCollector::EnsureSweepingCompleted() { assert(0); }

//
// internal::MessageLocation
//
MessageLocation::MessageLocation(Handle<Script> script, int start_pos, int end_pos) { assert(0); }

std::ostream& internal::operator<<(std::ostream& os, v8::internal::InstanceType i)
{
    assert(0);
    return os;
}
