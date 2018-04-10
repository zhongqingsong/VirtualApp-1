//
// VirtualApp Native Project
//
#include <Jni/VAJni.h>
#include "VMHook.h"

namespace FunctionDef {
    typedef void (*Function_DalvikBridgeFunc)(const void **, void *, const void *, void *);

    typedef jobject (*JNI_openDexNativeFunc)(JNIEnv *, jclass, jstring, jstring, jint);

    typedef jobject (*JNI_openDexNativeFunc_N)(JNIEnv *, jclass, jstring, jstring, jint, jobject,
                                               jobject);


    typedef jint (*JNI_cameraNativeSetupFunc_T1)(JNIEnv *, jobject, jobject, jint, jstring);

    typedef jint (*JNI_cameraNativeSetupFunc_T2)(JNIEnv *, jobject, jobject, jint, jint,
                                                 jstring);

    typedef jint (*JNI_cameraNativeSetupFunc_T3)(JNIEnv *, jobject, jobject, jint, jint,
                                                 jstring,
                                                 jboolean);

    typedef jint (*JNI_cameraNativeSetupFunc_T4)(JNIEnv *, jobject, jobject, jint, jstring,
                                                 jboolean);

    typedef jint (*JNI_getCallingUid)(JNIEnv *, jclass);

    typedef jint (*JNI_audioRecordNativeCheckPermission)(JNIEnv *, jobject, jstring);

    typedef jstring (*JNI_nativeLoad)(JNIEnv *env, jclass, jstring, jobject, jstring);
}

using namespace FunctionDef;


static struct {
    bool is_art;
    int native_offset;
    char *host_packageName;
    jint api_level;
    jmethodID method_onGetCallingUid;
    jmethodID method_onOpenDexFileNative;

    void *art_work_around_app_jni_bugs;

    char *(*GetCstrFromString)(void *);

    void *(*GetStringFromCstr)(const char *);

    int (*native_getCallingUid)(int);

    int (*IPCThreadState_self)(void);

    JNI_getCallingUid orig_getCallingUid;

    Function_DalvikBridgeFunc orig_cameraNativeSetup_dvm;
    int cameraMethodType;
    union {
        JNI_cameraNativeSetupFunc_T1 t1;
        JNI_cameraNativeSetupFunc_T2 t2;
        JNI_cameraNativeSetupFunc_T3 t3;
        JNI_cameraNativeSetupFunc_T4 t4;
    } orig_native_cameraNativeSetupFunc;

    Function_DalvikBridgeFunc orig_openDexFile_dvm;
    union {
        JNI_openDexNativeFunc beforeN;
        JNI_openDexNativeFunc_N afterN;
    } orig_openDexNativeFunc_art;

    JNI_audioRecordNativeCheckPermission orig_audioRecordNativeCheckPermission;
    JNI_nativeLoad orig_nativeLoad;

} patchEnv;


jint new_getCallingUid(JNIEnv *env, jclass clazz) {
    int uid = patchEnv.orig_getCallingUid(Environment::ensureCurrentThreadIsAttached(), clazz);
    uid = Environment::ensureCurrentThreadIsAttached()->CallStaticIntMethod(nativeEngineClass.get(),
                                                                            patchEnv.method_onGetCallingUid,
                                                                            uid);
    return uid;
}


jstring new_nativeLoad(JNIEnv *env, jclass clazz, jstring _file, jobject classLoader, jstring _ld) {
    return patchEnv.orig_nativeLoad(env, clazz, _file, classLoader, _ld);
}


static jobject new_native_openDexNativeFunc(JNIEnv *env, jclass jclazz, jstring javaSourceName,
                                            jstring javaOutputName, jint options) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(2, stringClass, NULL);

    if (javaSourceName) {
        env->SetObjectArrayElement(array, 0, javaSourceName);
    }
    if (javaOutputName) {
        env->SetObjectArrayElement(array, 1, javaOutputName);
    }
    env->CallStaticVoidMethod(nativeEngineClass.get(), patchEnv.method_onOpenDexFileNative, array);

    jstring newSource = (jstring) env->GetObjectArrayElement(array, 0);
    jstring newOutput = (jstring) env->GetObjectArrayElement(array, 1);

    return patchEnv.orig_openDexNativeFunc_art.beforeN(env, jclazz, newSource, newOutput,
                                                       options);
}

static jobject new_native_openDexNativeFunc_N(JNIEnv *env, jclass jclazz, jstring javaSourceName,
                                              jstring javaOutputName, jint options, jobject loader,
                                              jobject elements) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(2, stringClass, NULL);

    if (javaSourceName) {
        env->SetObjectArrayElement(array, 0, javaSourceName);
    }
    if (javaOutputName) {
        env->SetObjectArrayElement(array, 1, javaOutputName);
    }
    env->CallStaticVoidMethod(nativeEngineClass.get(), patchEnv.method_onOpenDexFileNative, array);

    jstring newSource = (jstring) env->GetObjectArrayElement(array, 0);
    jstring newOutput = (jstring) env->GetObjectArrayElement(array, 1);

    return patchEnv.orig_openDexNativeFunc_art.afterN(env, jclazz, newSource, newOutput, options,
                                                      loader, elements);
}

static void
new_bridge_openDexNativeFunc(const void **args, void *pResult, const void *method, void *self) {

    JNIEnv *env = Environment::ensureCurrentThreadIsAttached();

    const char *source = args[0] == NULL ? NULL : patchEnv.GetCstrFromString((void *) args[0]);
    const char *output = args[1] == NULL ? NULL : patchEnv.GetCstrFromString((void *) args[1]);

    jstring orgSource = source == NULL ? NULL : env->NewStringUTF(source);
    jstring orgOutput = output == NULL ? NULL : env->NewStringUTF(output);

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(2, stringClass, NULL);
    if (orgSource) {
        env->SetObjectArrayElement(array, 0, orgSource);
    }
    if (orgOutput) {
        env->SetObjectArrayElement(array, 1, orgOutput);
    }
    env->CallStaticVoidMethod(nativeEngineClass.get(), patchEnv.method_onOpenDexFileNative, array);

    jstring newSource = (jstring) env->GetObjectArrayElement(array, 0);
    jstring newOutput = (jstring) env->GetObjectArrayElement(array, 1);

    const char *_newSource = newSource == NULL ? NULL : env->GetStringUTFChars(newSource, NULL);
    const char *_newOutput = newOutput == NULL ? NULL : env->GetStringUTFChars(newOutput, NULL);

    args[0] = _newSource == NULL ? NULL : patchEnv.GetStringFromCstr(_newSource);
    args[1] = _newOutput == NULL ? NULL : patchEnv.GetStringFromCstr(_newOutput);

    if (source && orgSource) {
        env->ReleaseStringUTFChars(orgSource, source);
    }
    if (output && orgOutput) {
        env->ReleaseStringUTFChars(orgOutput, output);
    }

    patchEnv.orig_openDexFile_dvm(args, pResult, method, self);
}

static void
new_bridge_cameraNativeSetupFunc(const void **args, void *pResult, const void *method, void *self) {
    // args[0] = this
    switch (patchEnv.cameraMethodType) {
        case 1:
            args[4] = patchEnv.GetStringFromCstr(patchEnv.host_packageName);
            break;
        case 2:
            args[5] = patchEnv.GetStringFromCstr(patchEnv.host_packageName);
            break;
        case 3:
            args[5] = patchEnv.GetStringFromCstr(patchEnv.host_packageName);
            break;
        case 4:
            args[4] = patchEnv.GetStringFromCstr(patchEnv.host_packageName);
            break;
    }
    patchEnv.orig_cameraNativeSetup_dvm(args, pResult, method, self);
}

static jint new_native_cameraNativeSetupFunc_T1(JNIEnv *env, jobject thiz, jobject camera_this,
                                                jint cameraId, jstring packageName) {

    jstring host = env->NewStringUTF(patchEnv.host_packageName);

    return patchEnv.orig_native_cameraNativeSetupFunc.t1(env, thiz, camera_this,
                                                         cameraId,
                                                         host);
}

static jint new_native_cameraNativeSetupFunc_T2(JNIEnv *env, jobject thiz, jobject camera_this,
                                                jint cameraId, jint halVersion,
                                                jstring packageName) {

    jstring host = env->NewStringUTF(patchEnv.host_packageName);

    return patchEnv.orig_native_cameraNativeSetupFunc.t2(env, thiz, camera_this, cameraId,
                                                         halVersion, host);
}

static jint new_native_cameraNativeSetupFunc_T3(JNIEnv *env, jobject thiz, jobject camera_this,
                                                jint cameraId, jint halVersion,
                                                jstring packageName, jboolean option) {

    jstring host = env->NewStringUTF(patchEnv.host_packageName);

    return patchEnv.orig_native_cameraNativeSetupFunc.t3(env, thiz, camera_this, cameraId,
                                                         halVersion, host, option);
}

static jint new_native_cameraNativeSetupFunc_T4(JNIEnv *env, jobject thiz, jobject camera_this,
                                                jint cameraId,
                                                jstring packageName, jboolean option) {

    jstring host = env->NewStringUTF(patchEnv.host_packageName);

    return patchEnv.orig_native_cameraNativeSetupFunc.t4(env, thiz, camera_this, cameraId, host,
                                                         option);
}


static jint
new_native_audioRecordNativeCheckPermission(JNIEnv *env, jobject thiz, jstring _packagename) {
    jstring host = env->NewStringUTF(patchEnv.host_packageName);
    return patchEnv.orig_audioRecordNativeCheckPermission(env, thiz, host);
}


void mark() {
    // Do nothing
};


void measureNativeOffset(bool isArt) {

    jmethodID markMethod = nativeEngineClass->getStaticMethod<void(void)>("nativeMark").getId();

    size_t start = (size_t) markMethod;
    size_t target = (size_t) mark;

    if (isArt && patchEnv.art_work_around_app_jni_bugs) {
        target = (size_t) patchEnv.art_work_around_app_jni_bugs;
    }

    int offset = 0;
    bool found = false;
    while (true) {
        if (*((size_t *) (start + offset)) == target) {
            found = true;
            break;
        }
        offset += 4;
        if (offset >= 100) {
            ALOGE("Error: Cannot find the jni function offset.");
            break;
        }
    }
    if (found) {
        patchEnv.native_offset = offset;
    }
}

void vmUseJNIFunction(jmethodID method, void *jniFunction) {
    void **funPtr = (void **) (reinterpret_cast<size_t>(method) + patchEnv.native_offset);
    *funPtr = jniFunction;
}

void *vmGetJNIFunction(jmethodID method) {
    void **funPtr = (void **) (reinterpret_cast<size_t>(method) + patchEnv.native_offset);
    return *funPtr;
}


void hookJNIMethod(jmethodID method, void *new_jni_func, void **orig_jni_func) {
    *orig_jni_func = vmGetJNIFunction(method);
    vmUseJNIFunction(method, new_jni_func);
}


void hookGetCallingUid() {
    JNIEnv *env = Environment::current();
    jclass binderClass = env->FindClass("android/os/Binder");
    jmethodID getCallingUid = env->GetStaticMethodID(binderClass, "getCallingUid", "()I");
    hookJNIMethod(getCallingUid,
                  (void *) new_getCallingUid,
                  (void **) &patchEnv.orig_getCallingUid
    );
}

void hookOpenDexFileNative(jobject javaMethod,jboolean  isArt, int apiLevel) {

    jmethodID method = Environment::current()->FromReflectedMethod(javaMethod);
    void *jniFunc = vmGetJNIFunction(method);
    if (!isArt) {
        patchEnv.orig_openDexFile_dvm = (Function_DalvikBridgeFunc) (jniFunc);
        vmUseJNIFunction(method, (void *) new_bridge_openDexNativeFunc);
    } else {
        if (apiLevel < 24) {
            patchEnv.orig_openDexNativeFunc_art.beforeN = (JNI_openDexNativeFunc) (jniFunc);
            vmUseJNIFunction(method, (void *) new_native_openDexNativeFunc);
        } else {
            patchEnv.orig_openDexNativeFunc_art.afterN = (JNI_openDexNativeFunc_N) (jniFunc);
            vmUseJNIFunction(method, (void *) new_native_openDexNativeFunc_N);
        }
    }
}

void hookRuntimeNativeLoad() {
    if (patchEnv.is_art) {
        JNIEnv *env = Environment::current();
        jclass runtimeClass = env->FindClass("java/lang/Runtime");
        jmethodID nativeLoad = env->GetStaticMethodID(runtimeClass, "nativeLoad",
                                                      "(Ljava/lang/String;Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/String;");
        env->ExceptionClear();
        if (nativeLoad) {
            hookJNIMethod(nativeLoad, (void *) new_nativeLoad, (void **) &patchEnv.orig_nativeLoad);
        } else {
            ALOGE("Error: cannot find nativeLoad method.");
        }
    }
}

inline void
hookCameraNativeSetup(jobject javaMethod, jboolean isArt, int apiLevel) {

    if (!javaMethod) {
        return;
    }
    jmethodID method = Environment::current()->FromReflectedMethod(javaMethod);
    if (!isArt) {
        hookJNIMethod(method,
                      (void *) new_bridge_cameraNativeSetupFunc,
                      (void **) &patchEnv.orig_cameraNativeSetup_dvm
        );
    } else {
        switch (patchEnv.cameraMethodType) {
            case 1:
                hookJNIMethod(method,
                              (void *) new_native_cameraNativeSetupFunc_T1,
                              (void **) &patchEnv.orig_native_cameraNativeSetupFunc.t1
                );
                break;
            case 2:
                hookJNIMethod(method,
                              (void *) new_native_cameraNativeSetupFunc_T2,
                              (void **) &patchEnv.orig_native_cameraNativeSetupFunc.t2
                );
                break;
            case 3:
                hookJNIMethod(method,
                              (void *) new_native_cameraNativeSetupFunc_T3,
                              (void **) &patchEnv.orig_native_cameraNativeSetupFunc.t3
                );
                break;
            case 4:
                hookJNIMethod(method,
                              (void *) new_native_cameraNativeSetupFunc_T4,
                              (void **) &patchEnv.orig_native_cameraNativeSetupFunc.t4
                );
                break;
            default:
                break;
        }
    }

}

void
hookAudioRecordNativeCheckPermission(jobject javaMethod, jboolean isArt, int api) {
    if (!javaMethod || !isArt) {
        return;
    }
    jmethodID method = Environment::current()->FromReflectedMethod(javaMethod);
    hookJNIMethod(method,
                  (void *) new_native_audioRecordNativeCheckPermission,
                  (void **) &patchEnv.orig_audioRecordNativeCheckPermission
    );
}

void *getDvmOrArtSOHandle() {
    char so_name[25] = {0};
    __system_property_get("persist.sys.dalvik.vm.lib.2", so_name);
    if (strlen(so_name) == 0) {
        __system_property_get("persist.sys.dalvik.vm.lib", so_name);
    }
    void *soInfo = dlopen(so_name, 0);
    if (!soInfo) {
        soInfo = RTLD_DEFAULT;
    }
    return soInfo;
}

/**
 * Only called once.
 * @param javaMethod Method from Java
 * @param isArt Dalvik or Art
 * @param apiLevel Api level from Java
 */
void hookAndroidVM(JArrayClass<jobject> javaMethods,
                   jstring packageName, jboolean isArt, jint apiLevel,
                   jint cameraMethodType) {

    JNIEnv *env = Environment::current();

    JNINativeMethod methods[] = {
            NATIVE_METHOD((void *) mark, "nativeMark", "()V"),
    };
    if (env->RegisterNatives(nativeEngineClass.get(), methods, 1) < 0) {
        return;
    }
    patchEnv.is_art = isArt;
    patchEnv.cameraMethodType = cameraMethodType;
    patchEnv.host_packageName = (char *) env->GetStringUTFChars(packageName,
                                                                NULL);
    patchEnv.api_level = apiLevel;
    patchEnv.method_onGetCallingUid = nativeEngineClass->getStaticMethod<jint(jint)>(
            "onGetCallingUid").getId();
    patchEnv.method_onOpenDexFileNative = env->GetStaticMethodID(nativeEngineClass.get(),
                                                                 "onOpenDexFileNative",
                                                                 "([Ljava/lang/String;)V");

    void *soInfo = getDvmOrArtSOHandle();
    if (isArt) {
        patchEnv.art_work_around_app_jni_bugs = dlsym(soInfo, "art_work_around_app_jni_bugs");
    }else{
        // workaround for dlsym returns null when system has libhoudini
        void *h = dlopen("/system/lib/libandroid_runtime.so", RTLD_LAZY);
        {
            patchEnv.IPCThreadState_self = (int (*)(void)) dlsym(RTLD_DEFAULT,
                                                                 "_ZN7android14IPCThreadState4selfEv");
            patchEnv.native_getCallingUid = (int (*)(int)) dlsym(RTLD_DEFAULT,
                                                                 "_ZNK7android14IPCThreadState13getCallingUidEv");
            if (patchEnv.native_getCallingUid == NULL) {
                patchEnv.native_getCallingUid = (int (*)(int)) dlsym(RTLD_DEFAULT,
                                                                     "_ZN7android14IPCThreadState13getCallingUidEv");
            }
        }
        if (h != NULL) {
            dlclose(h);
        }

        patchEnv.GetCstrFromString = (char *(*)(void *)) dlsym(soInfo,
                                                               "_Z23dvmCreateCstrFromStringPK12StringObject");
        if (!patchEnv.GetCstrFromString) {
            patchEnv.GetCstrFromString = (char *(*)(void *)) dlsym(soInfo,
                                                                   "dvmCreateCstrFromString");
        }
        patchEnv.GetStringFromCstr = (void *(*)(const char *)) dlsym(soInfo,
                                                                     "_Z23dvmCreateStringFromCstrPKc");
        if (!patchEnv.GetStringFromCstr) {
            patchEnv.GetStringFromCstr = (void *(*)(const char *)) dlsym(soInfo,
                                                                         "dvmCreateStringFromCstr");
        }
    }
    measureNativeOffset(isArt);
    hookGetCallingUid();
    hookOpenDexFileNative(javaMethods.getElement(OPEN_DEX).get(), isArt, apiLevel);
    hookCameraNativeSetup(javaMethods.getElement(CAMERA_SETUP).get(), isArt, apiLevel);
    hookAudioRecordNativeCheckPermission(
            javaMethods.getElement(AUDIO_NATIVE_CHECK_PERMISSION).get(), isArt, apiLevel);
    hookRuntimeNativeLoad();
}
