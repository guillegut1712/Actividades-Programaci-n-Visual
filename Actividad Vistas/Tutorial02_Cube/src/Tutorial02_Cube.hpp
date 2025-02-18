/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"


#include <d3d11.h>
#include "Tutorial02_Cube.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "RenderDeviceD3D11.h"
#include "DeviceContextD3D11.h"

namespace Diligent
{

class Tutorial02_Cube final : public SampleBase
{


public:
    virtual void        Initialize(const SampleInitInfo& InitInfo) override final;
    virtual void        Render() override final;
    virtual void        Update(double CurrTime, double ElapsedTime) override final;
    virtual const Char* GetSampleName() const override final { return "Tutorial02: Cube"; }

private:
    int  vistaActual = 0;
    void CreatePipelineState();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreatePyramidBuffers();
    void CreateRhombusBuffers();
    void CreateStarBuffers();
    void CreateRectangleBuffers();
    void CreateDiskBuffers();
    void UpdateViewMatrix();
    void vistasRender();


    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
    RefCntAutoPtr<IBuffer>                m_CubeVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_CubeIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_PyramidVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_PyramidIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_RhombusVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_RhombusIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_StarVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_StarIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_RectangleVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_RectangleIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_DiskVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_DiskIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_VSConstants;
    float4x4                              m_WorldViewProjMatrix[13];
    float4x4                              Vistas;
};

} // namespace Diligent
