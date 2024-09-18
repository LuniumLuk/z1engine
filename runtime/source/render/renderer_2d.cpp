#include "pch.h"
//#include "Focal/Renderer/Renderer.h"
//#include "Focal/Renderer/Renderer2D.h"
//#include "Focal/Renderer/VertexArray.h"
//#include "Focal/Renderer/Image.h"
//#include "Focal/Renderer/Shader.h"
//#include "glm/gtc/matrix_transform.hpp"
//
//namespace Focal {
//
//    static struct Renderer2DStorage {
//        Ref<VertexArray> QuadVertexArray = nullptr;
//        Ref<Shader> Sprite2DShader = nullptr;
//        Ref<Image2D> WhiteTexture = nullptr;
//        glm::mat4 ProjView = glm::mat4(1.0f);
//    } s_Renderer2DStorage;
//
//    void Renderer2D::Init() {
//        FE_PROFILE_FUNCTION();
//        float quadVertices[] = {
//            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
//             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
//            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
//            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
//             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
//             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
//        };
//
//        auto vertexBuffer = VertexBuffer::Create(quadVertices, sizeof(quadVertices),
//            {
//                { DataType::Float3 }, // Position
//                { DataType::Float2 }, // Texture Coordinates
//            });
//
//        s_Renderer2DStorage.QuadVertexArray = VertexArray::Create({ vertexBuffer });
//
//        uint32_t whiteColor = 0xffffffff;
//        s_Renderer2DStorage.WhiteTexture = Image2D::Create(&whiteColor, sizeof(uint32_t), 1, 1);
//
//        s_Renderer2DStorage.Sprite2DShader = Shader::Create(GetEnginePath() / "Shader/Sprite2D.glsl");
//
//        Renderer::GetRenderer().SetBlend(true);
//        Renderer::GetRenderer().SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
//    }
//
//    void Renderer2D::Shutdown() {
//        FE_PROFILE_FUNCTION();
//        s_Renderer2DStorage.QuadVertexArray.reset();
//        s_Renderer2DStorage.WhiteTexture.reset();
//        s_Renderer2DStorage.Sprite2DShader.reset();
//    }
//
//    void Renderer2D::BeginScene(Ref<Camera> const& camera) {
//        FE_PROFILE_FUNCTION();
//        s_Renderer2DStorage.ProjView = camera->GetProjView();
//
//        s_Renderer2DStorage.Sprite2DShader->Bind();
//        s_Renderer2DStorage.Sprite2DShader->SetUniform("u_ProjView", &s_Renderer2DStorage.ProjView);
//        s_Renderer2DStorage.QuadVertexArray->Bind();
//    }
//
//    void Renderer2D::EndScene() {
//        FE_PROFILE_FUNCTION();
//        s_Renderer2DStorage.QuadVertexArray->Unbind();
//        s_Renderer2DStorage.Sprite2DShader->Unbind();
//    }
//
//    void Renderer2D::DrawQuad(glm::vec3 const& position, glm::vec2 const& size, glm::vec4 const& color, Ref<Image2D> const& image) {
//        FE_PROFILE_FUNCTION();
//        auto model = glm::translate(glm::mat4(1.0f), position);
//
//        s_Renderer2DStorage.Sprite2DShader->SetUniform("u_Model", &model);
//        s_Renderer2DStorage.Sprite2DShader->SetUniform("u_Color", &color);
//
//        s_Renderer2DStorage.Sprite2DShader->SetUniformBinding(
//            "u_Texture", Resource::BindResource((image ? image : s_Renderer2DStorage.WhiteTexture)->GetResourceID()));
//
//        s_Renderer2DStorage.QuadVertexArray->Draw(PrimitiveType::Triangles);
//        Resource::UnbindResource((image ? image : s_Renderer2DStorage.WhiteTexture)->GetResourceID());
//    }
//
//}
