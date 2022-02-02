/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "smart_camera_ability.h"

namespace OHOS {
REGISTER_AA(SmartCameraAbility)

void SmartCameraAbility::OnStart(const Want &want)
{
    printf("SmartCameraAbility::OnStart\n");
    SetMainRoute("SmartCameraAbilitySlice");

    Ability::OnStart(want);
}

void SmartCameraAbility::OnInactive()
{
    printf("SmartCameraAbility::OnInactive\n");
    Ability::OnInactive();
}

void SmartCameraAbility::OnActive(const Want &want)
{
    printf("SmartCameraAbility::OnActive\n");
    Ability::OnActive(want);
}

void SmartCameraAbility::OnBackground()
{
    printf("SmartCameraAbility::OnBackground\n");
    Ability::OnBackground();
}

void SmartCameraAbility::OnStop()
{
    printf("SmartCameraAbility::OnStop\n");
    Ability::OnStop();
}
}