#pragma once

#include <vector>

#include <typed-geometry/detail/optional.hh>
#include <typed-geometry/tg-lean.hh>

#include "CameraController.hh"

namespace glow::viewer
{
// NOTE: whenever a new setting is added it must also be added to computeHash
struct SceneConfig
{
    //
    // Graphics settings
    //

    /// if true, shows a grid on the ground
    bool enableGrid = true;

    /// if true, print mode makes the background white to save ink
    bool enablePrintMode = false;

    /// if true, shows a shadow on the ground
    bool enableShadows = true;

    /// if true, performs forward rendering
    /// NOTE: this can be disabled if bg / shadows are to be rendered separately
    bool enableForwardRendering = true;

    /// if true, performs transparent rendering
    /// NOTE: this can be disabled if bg / shadows are to be rendered separately
    bool enableTransparentRendering = true;

    /// if true, shows outlines on objects
    bool enableOutlines = true;

    /// if true, does not stop progressive rendering accumulation
    bool infiniteAccumulation = false;

    /// if true, always clears the progressive rendering accumulation
    bool clearAccumulation = false;

    /// background color (center)
    tg::color3 bgColorInner = tg::color3(.9f);
    /// background color (border)
    tg::color3 bgColorOuter = tg::color3(.6f);

    /// "strength" / "darkness" of the SSAO
    float ssaoPower = 4.f;
    /// relative radius of the SSAO
    float ssaoRadius = 1.f;

    /// if true, enables tonemapping
    bool enableTonemap = false;
    /// exposure setting for tonemapping
    float tonemapExposure = 1.f;


    //
    // Camera settings
    //

    /// if true, forces the next gv::view to reuse the current camera transformation
    bool preserveCamera = false;
    /// if true, reuses the previous gv::view camera transformation
    bool reuseCamera = false;

    /// if true, overwrites the starting camera orientation
    bool customCameraOrientation = false;
    /// custom camera azimuth (requries customCameraOrientation == true)
    tg::angle cameraAzimuth = 0_deg;
    /// custom camera altitude (requries customCameraOrientation == true)
    tg::angle cameraAltitude = 0_deg;
    /// custom camera distance (requries customCameraOrientation == true)
    float cameraDistance = -1.f;

    /// custom camera fov
    tg::optional<tg::angle> cameraHorizontalFov;

    /// if true, overwrites the starting camera position
    bool customCameraPosition = false;
    /// custom camera position (requires customCameraPosition == true)
    tg::pos3 cameraPosition = tg::pos3(1, 1, 1);
    /// custom camera target (requires customCameraPosition == true)
    tg::pos3 cameraTarget = tg::pos3::zero;

    /// if true, uses an orthogonal projection
    bool enableOrthogonalProjection = false;
    /// view volume of the orthogonal projection (requires enableOrthogonalProjection == true)
    tg::aabb3 orthogonalProjectionBounds;

    /// if non-null, uses a custom camera controller
    SharedCameraController customCameraController = nullptr;


    //
    // Scene settings
    //

    /// if set, uses this as bounding box instead of deriving it from renderables
    /// (useful to make the grid steady)
    tg::optional<tg::aabb3> customBounds;

    /// if set, this is where the ground grid originates
    tg::optional<tg::pos3> customGridCenter;

    /// if set, this is the size of a ground grid cell (the major ones)
    tg::optional<float> customGridSize;

    //
    // Interaction settings
    //

    /// any GLFW key in this list will cause the viewer to close (and report which key was pressed)
    std::vector<int> closeKeys;

    size_t computeHash() const;
};
}
