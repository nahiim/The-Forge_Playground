/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include "../../Utilities/Interfaces/IFileSystem.h"

#include "LuaManagerCommon.h"
#define IMEMORY_FROM_HEADER
#include "../../Utilities/Interfaces/IMemory.h"

class LuaManagerImpl;

class LuaManager
{
public:
    void Init();
    void Exit();
    ~LuaManager();

    template<class T>
    void SetFunction(const char* functionName, T function);

    bool RunScript(const char* scriptFile);
    void AddAsyncScript(const char* scriptFile, ScriptDoneCallback callback);
    void AddAsyncScript(const char* scriptFile);

    template<class T>
    void AddAsyncScript(const char* scriptFile, T callbackLambda);

    // updateFunctionName - function that will be called on Update()
    bool SetUpdatableScript(const char* scriptFile, const char* updateFunctionName, const char* exitFunctionName);
    bool ReloadUpdatableScript();
    // updateFunctionName - function that will be called.
    // If nullptr then function from SetUpdateScript arg is used.
    bool Update(float deltaTime, const char* updateFunctionName = nullptr);

private:
    LuaManagerImpl* m_Impl;

    void SetFunction(ILuaFunctionWrap* wrap);
    void AddAsyncScript(const char* scriptFile, IScriptCallbackWrap* callbackLambda);
};

template<typename T>
void LuaManager::SetFunction(const char* functionName, T function)
{
    LuaFunctionWrap<T>* functionWrap = (LuaFunctionWrap<T>*)tf_calloc(1, sizeof(LuaFunctionWrap<T>));
    tf_placement_new<LuaFunctionWrap<T>>(functionWrap, function, functionName);
    SetFunction(functionWrap);
}

template<class T>
void LuaManager::AddAsyncScript(const char* scriptFile, T callbackLambda)
{
    IScriptCallbackWrap* lambdaWrap = (IScriptCallbackWrap*)tf_calloc(1, sizeof(ScriptCallbackWrap<T>));
    tf_placement_new<ScriptCallbackWrap<T>>(lambdaWrap, callbackLambda);
    AddAsyncScript(scriptFile, lambdaWrap);
}
