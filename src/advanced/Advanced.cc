#include "Advanced.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/objects.hh>
#include "ecs/Engine.hh"

#define COUNTOF(Val) uint32_t(sizeof(Val) / sizeof(Val[0]))

namespace
{
inline uint32_t packSaturatedFloat4(tg::vec4 value)
{
    uint32_t res = 0;
    res |= (uint32_t)((value.w) * 255.0) << 0;
    res |= (uint32_t)((value.z) * 255.0) << 8;
    res |= (uint32_t)((value.y) * 255.0) << 16;
    res |= (uint32_t)((value.x) * 255.0) << 24;
    return res;
}

inline tg::vec4 unpackSaturatedFloat4(uint32_t value)
{
    tg::vec4 res;
    res.w = (float)((value >> 0) & 0xFF) / 255.f;
    res.z = (float)((value >> 8) & 0xFF) / 255.f;
    res.y = (float)((value >> 16) & 0xFF) / 255.f;
    res.x = (float)((value >> 24) & 0xFF) / 255.f;
    return res;
}
}

void gamedev::AdvancedFeatures::setECS(std::shared_ptr<gamedev::EngineECS> ecs) { mECS = ecs; }

void gamedev::AdvancedFeatures::destroy()
{
    gamedev::destroyStatistics(mStats);

    mWorld.destroy();

    if (mECS)
        mECS->AllInstancesDestroyed();
    else
        glow::error() << "Destroying world without ECS.";
}

void gamedev::AdvancedFeatures::createDemoInstances(glow::SharedVertexArray const& vaoSphere,
                                                    glow::SharedVertexArray const& vaoCube,
                                                    glow::SharedVertexArray const& vaoGenerated,
                                                    glow::SharedTexture2D const& texAlbedo,
                                                    glow::SharedTexture2D const& texNormal,
                                                    glow::SharedTexture2D const& texARM)
{
    // create instances
    uint32_t colorScheme[] = {0x003049, 0xD62828, 0xF77F00, 0xFCBF49, 0xEAE2B7}; // just hex colors "from the web"
    for (auto x = 0; x < 8; ++x)
    {
        for (auto z = 0; z < 9; ++z)
        {
            auto instanceHandle = mWorld.createInstance((x % 2 == 0 && z % 3 == 0) ? vaoSphere : vaoCube, texAlbedo, texNormal, texARM);
            gamedev::Instance& instance = mWorld.getInstance(instanceHandle);

            instance.xform.translation = {float(x) * 4.f, float(x + z) * 0.5f, float(z) * 4.f};

            instance.anim = (x % 3 == 0 && z % 2 == 0) ? gamedev::Instance::AnimationType::Bounce : gamedev::Instance::AnimationType::Spin;
            instance.animSpeed = 0.15f * float(x * z + 1.f);
            instance.animBaseY = instance.xform.translation.y;

            // alpha is stored in the 8 least significant bits
            float alpha = tg::min(1.f, float(x + z) / 7.f);
            instance.albedoBias = (colorScheme[(x * z) % COUNTOF(colorScheme)] << 8) | (uint32_t(alpha * 255.f) & 0xFF);
        }
    }

    mInstanceGenerated = mWorld.createInstance(vaoGenerated, texAlbedo, texNormal, texARM);
    mWorld.getInstanceTransform(mInstanceGenerated).translation = {-12, -2, 9};
    mWorld.getInstance(mInstanceGenerated).anim = gamedev::Instance::AnimationType::SinusTravel;

    // start with the first entity selected
    mSelectedInstance = mWorld.getNthInstanceHandle(0);
}

void gamedev::AdvancedFeatures::drawWorld(glow::UsedProgram& shader)
{
    // loop over instances in the world and draw them
    for (auto i = 0u; i < mWorld.getNumLiveInstances(); ++i)
    {
        gamedev::Instance const& instance = mWorld.getNthInstance(i);
        gamedev::InstanceHandle const handle = mWorld.getNthInstanceHandle(i);

        // bind the textures
        shader["uTexAlbedo"] = instance.texAlbedo;
        shader["uTexNormal"] = instance.texNormal;
        shader["uTexARM"] = instance.texARM;

        // compute and upload the model matrix
        tg::mat4x3 const modelMatrix = instance.xform.transform_mat();
        shader["uModel"] = modelMatrix;

        // set the instance handle value so it can be written to the picking buffer
        shader["uPickingID"] = handle._value;

        // set the color bias
        shader["uAlbedoBiasPacked"] = instance.albedoBias;

        // bind and draw the mesh
        instance.vao->bind().draw();
    }

    *mStatNumDraws += float(mWorld.getNumLiveInstances());
}

void gamedev::AdvancedFeatures::drawPolyWorld(glow::UsedProgram& shader)
{
    // loop over instances in the world and draw them
    for (auto i = 0u; i < mWorld.getNumLiveInstances(); ++i)
    {
        gamedev::Instance const& instance = mWorld.getNthInstance(i);
        gamedev::InstanceHandle const handle = mWorld.getNthInstanceHandle(i);

        // bind the textures
        shader["uTexAlbedo"] = instance.texAlbedo;

        // compute and upload the model matrix
        tg::mat4x3 const modelMatrix = instance.xform.transform_mat();
        shader["uModel"] = modelMatrix;

        // set the instance handle value so it can be written to the picking buffer
        shader["uPickingID"] = handle._value;

        // set the color bias
        shader["uAlbedoBiasPacked"] = instance.albedoBias;

        // bind and draw the mesh
        instance.vao->bind().draw();
    }

    *mStatNumDraws += float(mWorld.getNumLiveInstances());
}

void gamedev::AdvancedFeatures::drawOutlineIfSelected(glow::SharedTextureRectangle const& tgtOutline,
                                                      glow::SharedFramebuffer const& fbOutline,
                                                      glow::SharedProgram const& shaderOutline,
                                                      tg::mat4 const& proj,
                                                      tg::mat4 const& view)
{
    tg::vec4 clear = {0, 0, 0, 0};
    tgtOutline->clear(GL_RGBA, GL_FLOAT, tg::data_ptr(clear));

    auto fb = fbOutline->bind();

    // enable depth test and backface culling for this scope
    // the GLOW_SCOPED macro stores the current state and restores it at the end of the scope
    GLOW_SCOPED(enable, GL_DEPTH_TEST);
    GLOW_SCOPED(enable, GL_CULL_FACE);

    GLOW_SCOPED(depthFunc, GL_EQUAL);

    if (mWorld.isLiveHandle(mSelectedInstance))
    {
        auto shader = shaderOutline->use();
        shader["uProj"] = proj;
        shader["uView"] = view;

        auto const& instance = mWorld.getInstance(mSelectedInstance);

        tg::mat4x3 const modelMatrix = instance.xform.transform_mat();
        shader["uModel"] = modelMatrix;

        instance.vao->bind().draw();

        *mStatNumDraws += 1.f;
    }
}

void gamedev::AdvancedFeatures::runImgui(bool& outFlyToSelection, bool& outCreateNewInstance)
{
    if (ImGui::Begin("Objects"))
    {
        if (mWorld.isLiveHandle(mSelectedInstance))
        {
            gamedev::Instance& inst = mWorld.getInstance(mSelectedInstance);
            ImGui::Text("Selected Instance 0x%08X", mSelectedInstance._value);

            if (ImGui::Button("Focus on Selection (F)"))
            {
                outFlyToSelection = true;
            }
            ImGui::SameLine();

            if (ImGui::Button("Destroy Selection"))
            {
                mWorld.destroyInstance(mSelectedInstance);
                if (mECS)
                    mECS->InstanceDestroyed(mSelectedInstance);
                else
                    glow::error() << "Destroying instance without ECS.";
                mSelectedInstance = {};
            }

            tg::vec4 unpackedColor = unpackSaturatedFloat4(inst.albedoBias);
            if (ImGui::ColorEdit4("Color Bias", tg::data_ptr(unpackedColor), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview))
            {
                inst.albedoBias = packSaturatedFloat4(unpackedColor);
            }

            ImGui::TextUnformatted("Animation");

            if (ImGui::RadioButton("None##Anim", inst.anim == gamedev::Instance::AnimationType::None))
                inst.anim = gamedev::Instance::AnimationType::None;

            ImGui::SameLine();

            if (ImGui::RadioButton("Spin##Anim", inst.anim == gamedev::Instance::AnimationType::Spin))
                inst.anim = gamedev::Instance::AnimationType::Spin;

            ImGui::SameLine();

            if (ImGui::RadioButton("Bounce##Anim", inst.anim == gamedev::Instance::AnimationType::Bounce))
            {
                if (inst.anim != gamedev::Instance::AnimationType::Bounce)
                {
                    inst.anim = gamedev::Instance::AnimationType::Bounce;
                    inst.animBaseY = inst.xform.translation.y;
                }
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("Travel##Anim", inst.anim == gamedev::Instance::AnimationType::SinusTravel))
                inst.anim = gamedev::Instance::AnimationType::SinusTravel;

            ImGui::SliderFloat("Animation Speed", &inst.animSpeed, 0.f, 50.f);
            ImGui::SliderFloat("Animation Base Y", &inst.animBaseY, -50.f, 50.f);
        }
        else
        {
            ImGui::TextUnformatted("Left click to select an object");
            ImGui::Dummy(ImVec2(100, 128));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        static float lastSpawnX = 0.f;
        if (ImGui::Button("Spawn Instance"))
        {
            outCreateNewInstance = true;
        }

        static bool destroyAllArmed = false;
        if (ImGui::Button(destroyAllArmed ? "Really Destroy All Instances?" : "Destroy All Instances"))
        {
            if (destroyAllArmed)
            {
                mWorld.destroyAllInstances();
                if (mECS)
                    mECS->AllInstancesDestroyed();
                else
                    glow::error() << "Destroying world without ECS.";
            }
            destroyAllArmed = !destroyAllArmed;
        }
        if (destroyAllArmed)
        {
            ImGui::SameLine();
            if (ImGui::Button("Cancel##DestroyAll"))
                destroyAllArmed = false;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // An input box for the random seed that allows integer numbers only
        // ImGui::InputInt("Procedural Mesh Seed", &mSeed);

        //// Recreate the procedural mesh on on button click
        // if (ImGui::Button("Regenerate Procedural Mesh"))
        //{
        //    if (mWorld.isLiveHandle(mInstanceGenerated))
        //    {
        //        // regenerate
        //        mProcMesh = buildProceduralMesh(mSeed);
        //        // upload to gpu
        //        mWorld.getInstance(mInstanceGenerated).vao = mProcMesh->createVertexArray();
        //    }

        //    // bump the seed so you can spam this button and something happens
        //    ++mSeed;
        //}
    }
    ImGui::End();

    gamedev::runStatImgui(mStats);
}

void gamedev::AdvancedFeatures::updateAnimationsInWorld(float dt, float currentTime)
{
    using AnimType = gamedev::Instance::AnimationType;

    for (auto& instance : mWorld.getLiveInstancesSpan())
    {
        switch (instance.anim)
        {
        case AnimType::Spin:
        {
            auto const rotation = tg::quat::from_axis_angle({0, 1, 0}, tg::degree(90.f * dt * instance.animSpeed));
            instance.xform.rotate(rotation);
        }
        break;
        case AnimType::Bounce:
        {
            auto const val = tg::sin(100_deg * currentTime * instance.animSpeed) * .5f;
            instance.xform.translation.y = instance.animBaseY + val;
        }
        break;
        case AnimType::SinusTravel:
        {
            auto const angle = 90_deg * currentTime;
            instance.xform.translation.x += tg::sin(angle) * dt * instance.animSpeed;
            instance.xform.translation.z += tg::cos(angle) * dt * instance.animSpeed;
        }
        break;
        default:
            break;
        }
    }
}

void gamedev::AdvancedFeatures::updateStatDraws(float draws) {*mStatNumDraws += draws; }
