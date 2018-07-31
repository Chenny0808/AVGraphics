//
// Created by zzh on 2018/5/30 0030.
//

#include <android/log.h>
#include <pthread.h>
#include <android/native_window.h>
#include <log.h>
#include "EGLDemo.h"
#include "Triangle.h"

void *startThreadCallback(void *arg);

EGLDemo::EGLDemo(ANativeWindow *window) : mWindow(window), mEGLCore(new EGLCore()), mStartThread(0),
                                          mIsRendering(false) {
    pthread_mutex_init(&mMutex, nullptr);
    pthread_cond_init(&mCondition, nullptr);
}

void EGLDemo::start() {
    if (mWindow == nullptr || mWidth == 0 || mHeight == 0) {
        LOGE("not configured, cannot start");
        return;
    }
    pthread_create(&mStartThread, nullptr, startThreadCallback, (void *) this);
}

void *startThreadCallback(void *arg) {
    EGLDemo *demo = (EGLDemo *) arg;
    if (demo->init()) {
        demo->renderLoop();
        demo->stopDrawing();
    }
    return 0;
}

bool EGLDemo::init() {
    if (!mEGLCore->buildContext(mWindow)) {
        LOGE("buildContext failed");
        return false;
    }

    return doInit();
}

void EGLDemo::renderLoop() {
    mIsRendering = true;
    LOGD("renderLoop started");
    while (mIsRendering) {
        pthread_mutex_lock(&mMutex);

        drawAndSwapBuffer();
        pthread_cond_wait(&mCondition, &mMutex);

        pthread_mutex_unlock(&mMutex);
    }
    LOGD("renderLoop ended");
}

void EGLDemo::drawAndSwapBuffer() {
    doDraw();
    glFlush();
    mEGLCore->swapBuffer();
}

void EGLDemo::stopDrawing() {
    doStop();
    glDeleteProgram(mProgram);
    mEGLCore->release();
}

void EGLDemo::draw() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = true;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);
}

void EGLDemo::stop() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = false;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);

    pthread_join(mStartThread, 0);
}

EGLDemo::~EGLDemo() {
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCondition);

    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }

    if (mEGLCore) {
        delete mEGLCore;
        mEGLCore = nullptr;
    }
}