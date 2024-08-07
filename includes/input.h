#ifndef INPUT_H
#define INPUT_H

#include <The-Forge/Common_3/Application/Interfaces/IInput.h>
#include <The-Forge/Common_3/Application/Interfaces/ICameraController.h>
#include <The-Forge/Common_3/Application/Interfaces/IUI.h>

namespace Input
{
    ICameraController* pCameraController = nullptr;

    bool Init(Renderer* pRenderer, WindowDesc* pWindow);
    void Update(float deltaTime, float width, float height, mat4& viewMat, CameraMatrix& projMat);
    void Cleanup();
}

bool Input::Init(Renderer* pRenderer, WindowDesc* pWindow)
{
    InputSystemDesc inputDesc = {};
    inputDesc.pRenderer = pRenderer;
    inputDesc.pWindow = pWindow;

    if (!initInputSystem(&inputDesc))
    {
        return false;
    }

    GlobalInputActionDesc globalInputActionDesc = {};
    globalInputActionDesc.mGlobalInputActionType = GlobalInputActionDesc::ANY_BUTTON_ACTION;
    globalInputActionDesc.pFunction = [](InputActionContext* ctx)
    {
        if (ctx->mActionId > UISystemInputActions::UI_ACTION_START_ID_)
        {
            uiOnInput(ctx->mActionId, ctx->mBool, ctx->pPosition, &ctx->mFloat2);
        }
        return true;
    };
    setGlobalInputAction(&globalInputActionDesc);

    CameraMotionParameters cmp = {};
    vec3 camPos{ 0.0f, 0.0f, 20.0f };
    vec3 lookAt{ 0.0f, -20.0f, 0.0f };

    pCameraController = initFpsCameraController(camPos, lookAt);
    pCameraController->setMotionParameters(cmp);

    typedef bool (*CameraInputHandler)(InputActionContext* ctx, DefaultInputActions::DefaultInputAction action);
    static CameraInputHandler onCameraInput = [](InputActionContext* ctx, DefaultInputActions::DefaultInputAction action)
    {
        if (*(ctx->pCaptured))
        {
            float2 delta = uiIsFocused() ? float2(0.f, 0.f) : ctx->mFloat2;
            switch (action)
            {
            case DefaultInputActions::ROTATE_CAMERA:
                pCameraController->onRotate(delta);
                break;

            case DefaultInputActions::TRANSLATE_CAMERA:
                pCameraController->onMove(delta);
                break;

            case DefaultInputActions::TRANSLATE_CAMERA_VERTICAL:
                pCameraController->onMoveY(delta[0]);
                break;

            default:
                break;
            }
        }
        return true;
    };
    InputActionDesc actionDesc = {};
    actionDesc.mActionId = DefaultInputActions::CAPTURE_INPUT;
    actionDesc.pFunction = [](InputActionContext* ctx)
    {
        setEnableCaptureInput(!uiIsFocused() && INPUT_ACTION_PHASE_CANCELED != ctx->mPhase);
        return true;
    };
    addInputAction(&actionDesc);

    actionDesc = {};
    actionDesc.mActionId = DefaultInputActions::ROTATE_CAMERA;
    actionDesc.pFunction = [](InputActionContext* ctx)
    {
        return onCameraInput(ctx, DefaultInputActions::ROTATE_CAMERA);
    };
    addInputAction(&actionDesc);

    actionDesc = {};
    actionDesc.mActionId = DefaultInputActions::TRANSLATE_CAMERA;
    actionDesc.pFunction = [](InputActionContext* ctx)
    { return onCameraInput(ctx, DefaultInputActions::TRANSLATE_CAMERA); };
    addInputAction(&actionDesc);

    actionDesc = {};
    actionDesc.mActionId = DefaultInputActions::TRANSLATE_CAMERA_VERTICAL;
    actionDesc.pFunction = [](InputActionContext* ctx)
    { return onCameraInput(ctx, DefaultInputActions::TRANSLATE_CAMERA_VERTICAL); };
    addInputAction(&actionDesc);

    actionDesc = {};
    actionDesc.mActionId = DefaultInputActions::RESET_CAMERA;
    actionDesc.pFunction = [](InputActionContext* ctx)
    {
        if (!uiWantTextInput())
            pCameraController->resetView();
        return true;
    };
    addInputAction(&actionDesc);

    return true;
}




void Input::Update(float deltaTime, float width, float height, mat4& viewMat, CameraMatrix& projMat)
{
    pCameraController->update(deltaTime);

    const float aspectInverse = height / width;
    const float horizontal_fov = PI / 2.0f;

    viewMat = pCameraController->getViewMatrix();
    projMat = CameraMatrix::perspectiveReverseZ(horizontal_fov, aspectInverse, 0.1f, 1000.0f);
}


void Input::Cleanup()
{
    exitCameraController(pCameraController);
    exitInputSystem();
}


#endif // INPUT_H
