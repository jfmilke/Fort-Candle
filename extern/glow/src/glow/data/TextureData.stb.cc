#include "SurfaceData.hh"
#include "TextureData.hh"

#include <glow/common/log.hh>
#include <glow/common/profiling.hh>

#include <glow/detail/stb/stb_image.h>
#include <glow/detail/stb/stb_image_write.h>

using namespace glow;

SharedTextureData TextureData::loadWithStb(const std::string& filename, const std::string& /* ending */, ColorSpace colorSpace)
{
    GLOW_ACTION();

    auto tex = std::make_shared<TextureData>();
    auto surface = std::make_shared<SurfaceData>();

    int width = 0;
    int height = 0;
    int channel = 0;

    // HDR path (e.g. Radiance .hdr): stb decodes RGBE to linear float.
    // GL has no sRGB float internal format; HDR files are linear radiance by convention.
    if (glow_stbi_is_hdr(filename.c_str()))
    {
        if (colorSpace == ColorSpace::sRGB)
            warning() << "HDR texture " << filename << " loaded as linear (sRGB requested but not applicable to float HDR)";

        float* fdata = glow_stbi_loadf(filename.c_str(), &width, &height, &channel, 0);
        if (!fdata)
        {
            error() << "Error loading STB HDR texture data from " << filename;
            error() << "  Reason: " << glow_stbi_failure_reason();
            return nullptr;
        }

        auto const* bytes = reinterpret_cast<char const*>(fdata);
        size_t const num_bytes = size_t(width) * size_t(height) * size_t(channel) * sizeof(float);
        surface->setData(std::vector<char>(bytes, bytes + num_bytes));
        glow_stbi_image_free(fdata);

        surface->setType(GL_FLOAT);
        surface->setMipmapLevel(0);
        surface->setWidth(width);
        surface->setHeight(height);

        tex->setWidth(width);
        tex->setHeight(height);
        tex->addSurface(surface);

        switch (channel)
        {
        case 1:
            surface->setFormat(GL_RED);
            tex->setPreferredInternalFormat(GL_R16F);
            break;
        case 2:
            surface->setFormat(GL_RG);
            tex->setPreferredInternalFormat(GL_RG16F);
            break;
        case 3:
            surface->setFormat(GL_RGB);
            tex->setPreferredInternalFormat(GL_RGB16F);
            break;
        case 4:
            surface->setFormat(GL_RGBA);
            tex->setPreferredInternalFormat(GL_RGBA16F);
            break;
        default:
            error() << "Error loading STB HDR texture data from " << filename;
            error() << "  Unsupported number of channels: " << channel;
            return nullptr;
        }

        tex->setAnisotropicFiltering(16.0f);
        tex->setMagFilter(GL_LINEAR);
        tex->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);

        return tex;
    }

    unsigned char* data = glow_stbi_load(filename.c_str(), &width, &height, &channel, 0);

    // error case
    if (!data)
    {
        error() << "Error loading STB texture data from " << filename;
        error() << "  Reason: " << glow_stbi_failure_reason();
        return nullptr;
    }

    // ... process data if not NULL ...
    // ... x = width, y = height, n = # 8-bit components per pixel ...
    // ... replace '0' with '1'..'4' to force that many components per pixel
    // ... but 'n' will always be the number that it would have been if you said 0
    surface->setData(std::vector<char>(data, data + width * height * channel));
    glow_stbi_image_free(data);

    surface->setType(GL_UNSIGNED_BYTE);
    surface->setMipmapLevel(0);
    surface->setWidth(width);
    surface->setHeight(height);

    tex->setWidth(width);
    tex->setHeight(height);
    tex->addSurface(surface);

    GLenum iformatRGB;
    GLenum iformatRGBA;

    switch (colorSpace)
    {
    case ColorSpace::Linear:
        iformatRGB = GL_RGB;
        iformatRGBA = GL_RGBA;
        break;
    case ColorSpace::sRGB:
    default:
        iformatRGB = GL_SRGB;
        iformatRGBA = GL_SRGB_ALPHA;
        break;
    }

    switch (channel)
    {
    case 1:
        surface->setFormat(GL_RGB);
        tex->setPreferredInternalFormat(iformatRGB);
        { // convert grey to rgb
            std::vector<char> newData;
            auto const& oldData = surface->getData();
            auto oldSize = oldData.size();
            newData.resize(oldData.size() * 3);
            for (auto i = 0u; i < oldSize; ++i)
            {
                auto d = oldData[i];
                newData[i * 3 + 0] = d;
                newData[i * 3 + 1] = d;
                newData[i * 3 + 2] = d;
            }
            surface->setData(newData);
        }
        break;
    case 2:
        surface->setFormat(GL_RGBA);
        tex->setPreferredInternalFormat(iformatRGBA);
        { // convert grey-alpha to rgba
            std::vector<char> newData;
            auto const& oldData = surface->getData();
            auto oldSize = oldData.size();
            newData.resize(oldData.size() * 2);
            for (auto i = 0u; i < oldSize; i += 2)
            {
                auto d = oldData[i + 0];
                auto a = oldData[i + 1];
                newData[i * 2 + 0] = d;
                newData[i * 2 + 1] = d;
                newData[i * 2 + 2] = d;
                newData[i * 2 + 3] = a;
            }
            surface->setData(newData);
        }
        break;
    case 3:
        tex->setPreferredInternalFormat(iformatRGB);
        surface->setFormat(GL_RGB);
        break;
    case 4:
        tex->setPreferredInternalFormat(iformatRGBA);
        surface->setFormat(GL_RGBA);
        break;
    default:
        error() << "Error loading STB texture data from " << filename;
        error() << "  Unsupported number of channels: " << channel;
        return nullptr;
    }

    // high-quality default parameters:
    tex->setAnisotropicFiltering(16.0f);
    tex->setMagFilter(GL_LINEAR);
    tex->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);

    return tex;
}

void TextureData::saveWithStb(TextureData* data, const std::string& filename, const std::string& ending)
{
    GLOW_ACTION();

    for (auto const& surf : data->getSurfaces())
    {
        if (surf->getMipmapLevel() == 0)
        {
            if (surf->getType() != GL_UNSIGNED_BYTE)
            {
                error() << "Error saving STB texture data to " << filename;
                error() << "  Reason: type " << surf->getType() << " is not supported with STB (yet).";
                return;
            }

            int comps = -1;
            SharedSurfaceData tmpS = surf;

            switch (surf->getFormat())
            {
            case GL_RGBA:
                comps = 4;
                break;

            case GL_BGRA:
                comps = 4;
                tmpS = surf->convertTo(GL_RGBA, surf->getType());
                break;

            case GL_RGB:
                comps = 3;
                break;

            case GL_RED:
            case GL_RG:
            case GL_BGR:
                comps = 3;
                tmpS = surf->convertTo(GL_RGB, surf->getType());
                break;

            default:
                error() << "Error saving STB texture data to " << filename;
                error() << "  Reason: format " << surf->getFormat() << " is not supported with STB.";
                break;
            }

            int ok;
            if (ending == ".png")
                ok = glow_stbi_write_png(filename.c_str(), surf->getWidth(), surf->getHeight(), comps, surf->getFlippedData().data(), comps * surf->getWidth());
            else if (ending == ".bmp")
                ok = glow_stbi_write_bmp(filename.c_str(), surf->getWidth(), surf->getHeight(), comps, surf->getFlippedData().data());
            else if (ending == ".tga")
                ok = glow_stbi_write_tga(filename.c_str(), surf->getWidth(), surf->getHeight(), comps, surf->getFlippedData().data());
            else
            {
                error() << "Error saving STB texture data to " << filename;
                error() << "  Reason: ending " << ending << " is not supported with stb.";
                return;
            }

            if (!ok)
            {
                error() << "Error saving STB texture data to " << filename;
                error() << "  Reason: " << glow_stbi_failure_reason();
            }

            return;
        }
    }

    error() << "Error saving STB texture data to " << filename;
    error() << "  Reason: No supported level 0 surface found.";
}
