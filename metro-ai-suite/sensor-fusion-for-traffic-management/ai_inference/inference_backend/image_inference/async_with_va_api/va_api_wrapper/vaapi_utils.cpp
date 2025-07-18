/*******************************************************************************
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include <assert.h>
#include <fcntl.h>
#include <glob.h>
#include <string.h>
#include <unistd.h>

#include <scope_guard.h>
#include <vaapi_utils.h>

namespace internal {

VaApiLibBinderImpl::VaApiLibBinderImpl() {
    _libva_so = SharedObject::getLibrary("libva.so.2");
    _libva_drm_so = SharedObject::getLibrary("libva-drm.so.2");
}

VADisplay VaApiLibBinderImpl::GetDisplayDRM(int file_descriptor) {
    auto dpy = _libva_drm_so->invoke<VADisplay(int)>("vaGetDisplayDRM", file_descriptor);
    if (!dpy) {
        throw std::runtime_error("Error opening VAAPI Display");
    }
    return dpy;
}

VAStatus VaApiLibBinderImpl::Initialize(VADisplay dpy, int *major_version, int *minor_version) {
    return _libva_so->invoke<VAStatus(VADisplay, int *, int *)>("vaInitialize", dpy, major_version, minor_version);
}

VAStatus VaApiLibBinderImpl::Terminate(VADisplay dpy) {
    return _libva_so->invoke<VAStatus(VADisplay)>("vaTerminate", dpy);
}

std::function<const char *(VAStatus)> VaApiLibBinderImpl::StatusToStrFunc() {
    return _libva_so->getFunction<const char *(VAStatus)>("vaErrorStr");
}

} /* namespace internal */

namespace {

static void message_callback_error(void * /*user_ctx*/, const char *message) {
    GVA_ERROR("%s", message);
}

static void message_callback_info(void * /*user_ctx*/, const char *message) {
    GVA_INFO("%s", message);
}

/**
 * Sets the internal VADisplay's error and info callbacks. Initializes the internal VADisplay.
 *
 * @pre _display must be set.
 * @pre libva_handle must be set to initialize VADisplay.
 * @pre message_callback_error, message_callback_info functions must reside in namespace to set callbacks.
 *
 * @throw std::runtime_error if the initialization failed.
 * @throw std::invalid_argument if display is null.
 */
void initializeVaDisplay(VaDpyWrapper display) {
    assert(display);

    display.dpyCtx()->error_callback = message_callback_error;
    display.dpyCtx()->error_callback_user_context = nullptr;

    display.dpyCtx()->info_callback = message_callback_info;
    display.dpyCtx()->info_callback_user_context = nullptr;

    int major_version = 0, minor_version = 0;
    VA_CALL(VaApiLibBinder::get().Initialize(display.raw(), &major_version, &minor_version));
}

} // namespace

internal::VaApiLibBinderImpl &VaApiLibBinder::get() {
    static internal::VaApiLibBinderImpl instance;
    return instance;
}

int VaDpyWrapper::currentSubDevice() const {
#if VA_CHECK_VERSION(1, 12, 0)
    VADisplayAttribValSubDevice reg;
    VADisplayAttribute reg_attr;
    reg_attr.type = VADisplayAttribType::VADisplayAttribSubDevice;
    if (drvVtable().vaGetDisplayAttributes(drvCtx(), &reg_attr, 1) == VA_STATUS_SUCCESS) {
        reg.value = reg_attr.value;
        if (reg.bits.sub_device_count > 0)
            return static_cast<int>(reg.bits.current_sub_device);
    }
#else
    GVA_WARNING("Current version of libva doesn't support sub-device API, version 2.12 or higher is required");
#endif
    return -1;
}
