#include "../main.h"
#include "gui.h"
#include "../game/game.h"
#include "../game/RW/RenderWare.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

RwRaster* g_FontRaster = nullptr;

namespace {

constexpr size_t kMaxVertices = 30000;
std::unique_ptr<RwIm2DVertex[]> g_vertices;
size_t g_vertexCapacity = 0;
ImVec4 g_scissorRect{};

bool EnsureVertexCapacity(size_t required) {
    if (required == 0 || required > kMaxVertices) return false;
    if (g_vertexCapacity >= required) return true;

    const size_t capacity = std::min(kMaxVertices, std::max(required, g_vertexCapacity ? g_vertexCapacity * 2 : size_t{4096}));
    std::unique_ptr<RwIm2DVertex[]> fresh(new (std::nothrow) RwIm2DVertex[capacity]);
    if (!fresh) {
        Log("GUI | Cannot allocate RenderWare ImGui vertex buffer (%zu vertices)", capacity);
        return false;
    }
    g_vertices = std::move(fresh);
    g_vertexCapacity = capacity;
    return true;
}

void SetScissor(const ImVec4& clip, const ImVec2& displaySize) {
    const float x1 = std::clamp(clip.x, 0.0f, displaySize.x);
    const float y1 = std::clamp(clip.y, 0.0f, displaySize.y);
    const float x2 = std::clamp(clip.z, 0.0f, displaySize.x);
    const float y2 = std::clamp(clip.w, 0.0f, displaySize.y);
    if (x2 <= x1 || y2 <= y1) {
        g_scissorRect = ImVec4(0, 0, 0, 0);
    } else {
        // Existing SetScissorRect expects the native bottom/top ordering.
        g_scissorRect = ImVec4(x1, y2, x2, y1);
    }
    SetScissorRect(&g_scissorRect);
}

}  // namespace

void ImGui_ImplRenderWare_RenderDrawData(ImDrawData* drawData) {
    if (drawData == nullptr || drawData->CmdListsCount <= 0 || drawData->TotalVtxCount <= 0) return;
    if (drawData->TotalVtxCount > static_cast<int>(kMaxVertices) ||
        !EnsureVertexCapacity(static_cast<size_t>(drawData->TotalVtxCount))) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 displaySize = io.DisplaySize;
    if (displaySize.x <= 0.0f || displaySize.y <= 0.0f) return;

    RwIm2DVertex* const vertexBase = g_vertices.get();
    size_t vertexOffset = 0;

    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
        const ImDrawList* list = drawData->CmdLists[listIndex];
        if (list == nullptr) continue;
        const size_t listVertexCount = static_cast<size_t>(list->VtxBuffer.Size);
        if (vertexOffset + listVertexCount > g_vertexCapacity) return;

        for (size_t i = 0; i < listVertexCount; ++i) {
            const ImDrawVert& src = list->VtxBuffer[static_cast<int>(i)];
            RwIm2DVertex* dst = &vertexBase[vertexOffset + i];
            RwIm2DVertexSetScreenX(dst, src.pos.x);
            RwIm2DVertexSetScreenY(dst, src.pos.y);
            RwIm2DVertexSetScreenZ(dst, CSprite2d::NearScreenZ);
            RwIm2DVertexSetRecipCameraZ(dst, CSprite2d::RecipNearClip);
            dst->emissiveColor = src.col;
            RwIm2DVertexSetU(dst, src.uv.x, recipCameraZ);
            RwIm2DVertexSetV(dst, src.uv.y, recipCameraZ);
        }

        for (int commandIndex = 0; commandIndex < list->CmdBuffer.Size; ++commandIndex) {
            const ImDrawCmd& cmd = list->CmdBuffer[commandIndex];
            if (cmd.UserCallback != nullptr) {
                if (cmd.UserCallback == ImDrawCallback_ResetRenderState) {
                    // State is set for every ordinary draw command below.
                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
                } else {
                    cmd.UserCallback(list, &cmd);
                }
                continue;
            }
            if (cmd.ElemCount == 0 || cmd.IdxOffset + cmd.ElemCount > static_cast<unsigned int>(list->IdxBuffer.Size)) continue;

            SetScissor(ImVec4(
                (cmd.ClipRect.x - drawData->DisplayPos.x) * drawData->FramebufferScale.x,
                (cmd.ClipRect.y - drawData->DisplayPos.y) * drawData->FramebufferScale.y,
                (cmd.ClipRect.z - drawData->DisplayPos.x) * drawData->FramebufferScale.x,
                (cmd.ClipRect.w - drawData->DisplayPos.y) * drawData->FramebufferScale.y), displaySize);
            if (g_scissorRect.z <= g_scissorRect.x || g_scissorRect.y <= g_scissorRect.w) continue;

            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
            RwRenderStateSet(rwRENDERSTATESRCBLEND, RWRSTATE(rwBLENDSRCALPHA));
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
            RwRenderStateSet(rwRENDERSTATEFOGENABLE, RWRSTATE(FALSE));
            RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, RWRSTATE(rwFILTERLINEAR));
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, RWRSTATE(rwTEXTUREADDRESSCLAMP));
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, cmd.GetTexID());

            const ImDrawIdx* indexBase = list->IdxBuffer.Data + cmd.IdxOffset;
            const size_t indexCount = cmd.ElemCount;
            const size_t maxIndex = list->VtxBuffer.Size;
            bool indicesValid = true;
            for (size_t i = 0; i < indexCount; ++i) {
                if (indexBase[i] >= maxIndex) {
                    indicesValid = false;
                    break;
                }
            }
            if (indicesValid) {
                RwIm2DRenderIndexedPrimitive(
                    rwPRIMTYPETRILIST,
                    &vertexBase[vertexOffset],
                    static_cast<RwInt32>(listVertexCount),
                    reinterpret_cast<RwImVertexIndex*>(const_cast<ImDrawIdx*>(indexBase)),
                    static_cast<RwInt32>(cmd.ElemCount));
            }
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
        }
        vertexOffset += listVertexCount;
    }

    g_scissorRect = ImVec4(0, 0, 0, 0);
    SetScissorRect(&g_scissorRect);
}

bool ImGui_ImplRenderWare_Init() {
    if (RsGlobal == nullptr) return false;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(RsGlobal->maximumWidth), static_cast<float>(RsGlobal->maximumHeight));
    return io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f;
}

bool ImGui_ImplRenderWare_CreateDeviceObjects() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts == nullptr) return false;

    unsigned char* pixels = nullptr;
    int width = 0, height = 0, bytesPerPixel = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);
    if (pixels == nullptr || width <= 0 || height <= 0 || bytesPerPixel != 4) return false;

    RwImage* image = RwImageCreate(width, height, bytesPerPixel * 8);
    if (image == nullptr) return false;
    if (RwImageAllocatePixels(image) == nullptr || image->cpPixels == nullptr) {
        RwImageDestroy(image);
        return false;
    }

    const size_t rowBytes = static_cast<size_t>(width) * static_cast<size_t>(bytesPerPixel);
    for (int y = 0; y < height; ++y) {
        std::memcpy(image->cpPixels + static_cast<size_t>(y) * image->stride,
                    pixels + static_cast<size_t>(y) * rowBytes,
                    std::min(rowBytes, static_cast<size_t>(image->stride)));
    }

    RwInt32 rasterWidth = 0, rasterHeight = 0, depth = 0, flags = 0;
    if (!RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, &rasterWidth, &rasterHeight, &depth, &flags)) {
        RwImageDestroy(image);
        return false;
    }
    RwRaster* raster = RwRasterCreate(rasterWidth, rasterHeight, depth, flags);
    if (raster == nullptr) {
        RwImageDestroy(image);
        return false;
    }
    raster = RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);
    if (raster == nullptr) return false;

    if (g_FontRaster != nullptr) RwRasterDestroy(g_FontRaster);
    g_FontRaster = raster;
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(g_FontRaster));
    return true;
}

void ImGui_ImplRenderWare_ShutDown() {
    if (g_FontRaster != nullptr) {
        RwRasterDestroy(g_FontRaster);
        g_FontRaster = nullptr;
    }
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().Fonts != nullptr) {
        ImGui::GetIO().Fonts->SetTexID(nullptr);
    }
    g_vertices.reset();
    g_vertexCapacity = 0;
}

bool ImGui_ImplRenderWare_NewFrame() {
    if (RsGlobal == nullptr) return false;
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 newSize(static_cast<float>(RsGlobal->maximumWidth), static_cast<float>(RsGlobal->maximumHeight));
    if (newSize.x <= 0.0f || newSize.y <= 0.0f) return false;
    io.DisplaySize = newSize;
    if (g_FontRaster == nullptr && !ImGui_ImplRenderWare_CreateDeviceObjects()) return false;
    return true;
}
