#include "Factory.hh"
#include "utility/misc.hh"
#include <glow/common/log.hh>
#include <fstream>

void gamedev::Factory::Init(SharedEngineECS& ecs, SharedGameObjects& obj)
{
    mECS = ecs;
    mObj = obj;
}

void gamedev::Factory::AttachTerrain(std::shared_ptr<Terrain>& terrain)
{
    mT = terrain;
}

void gamedev::Factory::AddPreset(std::string identifier, InstanceHandle presetHandle)
{
    std::string name = lowerString(identifier);
    std::filesystem::path path = name;

    // Check if identifier is a path and add the path as well if it is
    if (path.has_parent_path())
    {
        name = path.filename().string();
        name = name.substr(0, name.find_last_of('.'));

        return AddPreset(name, path, presetHandle);
    }

    // Check if identifier is already in use (just overwrite then)
    if (mObjectNameToID.find(name) != mObjectNameToID.end())
    {
        OverwritePreset(mObjectNameToID[name], presetHandle);
        return;
    }

    // If identifier new add it to the system
    mObjectNameToID.insert({name, mFreeID});
    mFreeID++;
    mIDToPreset.insert({mObjectNameToID[name], presetHandle});
}

void gamedev::Factory::AddPreset(std::string identifier, std::filesystem::path identifying_path, InstanceHandle presetHandle)
{
    std::string name = lowerString(identifier);
    std::string path = lowerString(identifying_path.string());

    bool pathFound = false;
    bool nameFound = false;

    if (mObjectPathToID.find(path) != mObjectPathToID.end())
    {
        pathFound = true;
    }

    if (mObjectNameToID.find(name) != mObjectNameToID.end())
    {
        nameFound = true;
    }

    if (pathFound && nameFound)
    {
        OverwritePreset(mObjectPathToID[path], presetHandle);
    }

    if (!pathFound && nameFound)
    {
        OverwritePreset(mObjectNameToID[name], presetHandle);

        mObjectPathToID.insert({path, mObjectNameToID[name]});
        return;
    }

    if (pathFound && !nameFound)
    {
        OverwritePreset(mObjectPathToID[path], presetHandle);

        mObjectNameToID.insert({name, mObjectPathToID[path]});
        return;
    }

    mObjectNameToID.insert({name, mFreeID});
    mObjectPathToID.insert({path, mFreeID});
    mFreeID++;
    mIDToPreset.insert({mObjectNameToID[name], presetHandle});
}

void gamedev::Factory::OverwritePreset(int presetID, InstanceHandle templateHandle)
{
    glow::info() << "Overwriting Object.";
    auto presetHandle = mIDToPreset[presetID];
    mECS->RemoveAllComponents(presetHandle);
    mECS->CloneComponents(templateHandle, presetHandle);
}

gamedev::InstanceHandle gamedev::Factory::GetPreset(std::string identifier)
{
    std::string name = lowerString(identifier);
    
    if (mObjectNameToID.find(name) != mObjectNameToID.end())
    {
        return mIDToPreset[mObjectNameToID[name]];
    }

    if (mObjectPathToID.find(name) != mObjectPathToID.end())
    {
        return mIDToPreset[mObjectPathToID[name]];
    }

    return {std::uint32_t(-1)};
}

gamedev::InstanceHandle gamedev::Factory::Register(std::string asset_identifier, float scaling)
{
    std::string a_name = lowerString(asset_identifier);

    if (!mObj->hasObject(a_name))
    {
        glow::error() << "Asset manager has no " << a_name << ".";
        return { std::uint32_t(-1) };
    }

    auto handle = mECS->CreateInstance(mObj->getVAO(a_name), mObj->mTexAlbedoPolyPalette, nullptr, nullptr);
    auto& instance = mECS->GetInstance(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = {scaling, scaling, scaling};

    // Calculate maximum possible bounding box
    auto max_bounds = mObj->getAABB(a_name);
    max_bounds.min, max_bounds.max *= instance.xform.scaling * 1.15;
    instance.max_bounds = MaxBoundingBox(max_bounds);

    PostProcessRegistration(handle);

    AddPreset(a_name, handle);

    return handle;
}

gamedev::InstanceHandle gamedev::Factory::Register(std::string asset_identifier, std::string factory_identifier, float scaling)
{
    std::string a_name = lowerString(asset_identifier);
    std::string f_name = lowerString(factory_identifier);

    if (!mObj->hasObject(a_name))
    {
        glow::error() << "Asset manager has no " << a_name << ".";
        return {std::uint32_t(-1)};
    }

    auto handle = mECS->CreateInstance(mObj->getVAO(a_name), mObj->mTexAlbedoPolyPalette, nullptr, nullptr);
    auto& instance = mECS->GetInstance(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = {scaling, scaling, scaling};
    auto max_bounds = mObj->getAABB(a_name);
    max_bounds.min, max_bounds.max *= instance.xform.scaling * 1.25;
    instance.max_bounds = MaxBoundingBox(max_bounds);

    PostProcessRegistration(handle);

    AddPreset(f_name, handle);

    return handle;
}

gamedev::InstanceHandle gamedev::Factory::Register(std::string asset_identifier, std::string factory_identifier, std::filesystem::path factory_path, float scaling)
{
    std::string a_name = lowerString(asset_identifier);
    std::string f_name = lowerString(factory_identifier);
    std::filesystem::path f_path = lowerString(factory_path.string());

    if (!mObj->hasObject(a_name))
    {
        glow::error() << "Asset manager has no " << a_name << ".";
        return {std::uint32_t(-1)};
    }

    auto handle = mECS->CreateInstance(mObj->getVAO(a_name), mObj->mTexAlbedoPolyPalette, nullptr, nullptr);
    auto& instance = mECS->GetInstance(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = {scaling, scaling, scaling};
    auto max_bounds = mObj->getAABB(a_name);
    max_bounds.min, max_bounds.max *= instance.xform.scaling * 1.25;
    instance.max_bounds = MaxBoundingBox(max_bounds);


    PostProcessRegistration(handle);

    AddPreset(f_name, f_path, handle);

    return handle;
}

gamedev::InstanceHandle gamedev::Factory::Clone(InstanceHandle source_handle)
{
    auto clone_handle = mECS->CloneInstance(source_handle);
    auto& clone = mECS->GetInstance(clone_handle);
    mECS->CloneComponents(source_handle, clone_handle);
    mECS->GetHashMap().AddInstance(clone_handle, clone.xform.translation, clone.max_bounds);
    return clone_handle;
}

gamedev::InstanceHandle gamedev::Factory::Clone(InstanceHandle source_handle, tg::pos3 position)
{
    auto clone_handle = Clone(source_handle);
    auto& clone = mECS->GetInstance(clone_handle);
    clone.xform.translation = position - tg::pos3::zero;
    mECS->GetHashMap().UpdateInstance(clone_handle, clone.xform.translation, clone.max_bounds);

    if (mT)
    {
        clone.xform.translation.y = mT->heightAt(position);
    }

    mECS->GetHashMap().UpdateInstance(clone_handle, clone.xform.translation, clone.max_bounds);
    auto e = Event(EventType::NewPosition, clone_handle, clone_handle);
    mECS->SendFunctionalEvent(e);

    return clone_handle;
}

gamedev::InstanceHandle gamedev::Factory::Clone(InstanceHandle source_handle, tg::pos3 position, tg::quat rotation)
{
    auto clone_handle = Clone(source_handle, position);
    auto& clone = mECS->GetInstance(clone_handle);
    clone.xform.rotation = rotation;

    return clone_handle;
}

gamedev::InstanceHandle gamedev::Factory::Clone(InstanceHandle source_handle, tg::pos3 position, tg::quat rotation, tg::size3 scaling)
{
    auto clone_handle = Clone(source_handle, position);
    auto& clone = mECS->GetInstance(clone_handle);
    clone.xform.rotation = rotation;
    clone.xform.scaling = scaling;

    return clone_handle;
}

gamedev::InstanceHandle gamedev::Factory::Clone(InstanceHandle source_handle, tg::pos3 position, tg::size3 scaling)
{
    auto clone_handle = Clone(source_handle, position);
    auto& clone = mECS->GetInstance(clone_handle);
    clone.xform.scaling = scaling;

    return clone_handle;
}

gamedev::InstanceHandle gamedev::Factory::Create(std::string identifier)
{
    std::string name = lowerString(identifier);

    if (!exists(name))
    {
        glow::error() << "Creating unregistered unit.";
        return { std::uint32_t(-1)};
    }

    auto preset_handle = GetPreset(name);
    auto fresh_handle = mECS->CloneInstance(preset_handle);
    mECS->CloneComponents(preset_handle, fresh_handle);

    mECS->RemoveComponent<gamedev::Prototype>(fresh_handle);

    auto& instance = mECS->GetInstance(fresh_handle);
    mECS->GetHashMap().AddInstance(fresh_handle, instance.xform.translation, instance.max_bounds);
    auto e = Event(EventType::NewPosition, fresh_handle, fresh_handle);
    mECS->SendFunctionalEvent(e);

    mECS->UpdateSystems(fresh_handle);

    mGeneratedObjects.push_back({fresh_handle, name});
    mLastCreated = mGeneratedObjects.back();

    return fresh_handle;
}

gamedev::InstanceHandle gamedev::Factory::Create(std::string object_name, tg::pos3 position)
{
    auto fresh_handle = Create(object_name);
    auto& transform = mECS->GetInstance(fresh_handle).xform;
    transform.translation = tg::vec3(position);

    auto& instance = mECS->GetInstance(fresh_handle);

    if (mT)
    {
        transform.translation.y = mT->heightAt(position);
    }

    mECS->GetHashMap().UpdateInstance(fresh_handle, instance.xform.translation, instance.max_bounds);
    auto e = Event(EventType::NewPosition, fresh_handle, fresh_handle);
    mECS->SendFunctionalEvent(e);

    return fresh_handle;
}

gamedev::InstanceHandle gamedev::Factory::Create(std::string object_name, tg::pos3 position, tg::quat rotation)
{
    auto fresh_handle = Create(object_name, position);
    auto& transform = mECS->GetInstance(fresh_handle).xform;
    transform.rotation = rotation;

    return fresh_handle;
}

gamedev::InstanceHandle gamedev::Factory::Create(std::string object_name, tg::pos3 position, tg::quat rotation, tg::size3 scaling)
{
    auto fresh_handle = Create(object_name, position);
    auto& transform = mECS->GetInstance(fresh_handle).xform;
    transform.rotation = rotation;
    transform.scaling = scaling;

    return fresh_handle;
}

gamedev::InstanceHandle gamedev::Factory::Create(std::string object_name, tg::pos3 position, tg::size3 scaling)
{
    auto fresh_handle = Create(object_name, position);
    auto& transform = mECS->GetInstance(fresh_handle).xform;
    transform.scaling = scaling;

    return fresh_handle;
}

void gamedev::Factory::RegisterAutomatically(std::vector<std::string> asset_identifiers, float scaling)
{
    for (const auto& obj : asset_identifiers)
    {
        Register(obj, scaling);
    }
}

bool gamedev::Factory::exists(std::string object_name)
{
    std::string name = lowerString(object_name);

    if (mObjectNameToID.find(name) != mObjectNameToID.end())
    {
        return true;
    }

    if (mObjectPathToID.find(name) != mObjectPathToID.end())
    {
        return true;
    }

    return false;
}

std::vector<std::string> gamedev::Factory::GetNameIdentifiers()
{
    std::vector<std::string> names;

    for (const auto& name_map : mObjectNameToID)
    {
        names.push_back(name_map.first);
    }

    return names;
}

std::vector<std::string> gamedev::Factory::GetPathIdentifiers()
{
    std::vector<std::string> paths;

    for (const auto& path_map : mObjectNameToID)
    {
        paths.push_back(path_map.first);
    }

    return paths;
}

std::vector<gamedev::InstanceHandle> gamedev::Factory::GetGeneratedHandles()
{
    std::vector<InstanceHandle> handles;
    handles.reserve(mGeneratedObjects.size());

    // clean up list before use
    mGeneratedObjects.remove_if([this](std::pair<InstanceHandle, std::string> obj) { return !mECS->IsLiveHandle(obj.first); });

    for (const auto& object : mGeneratedObjects)
    {
        handles.push_back(object.first);
    }

    return handles;
}

std::vector<std::pair<gamedev::InstanceHandle, std::string>> gamedev::Factory::GetGeneratedIdentifiers()
{
    std::vector<std::pair<InstanceHandle, std::string>> identifiers;
    identifiers.reserve(mGeneratedObjects.size());

    // clean up list before use
    mGeneratedObjects.remove_if([this](std::pair<InstanceHandle, std::string> obj) { return !mECS->IsLiveHandle(obj.first); });

    for (const auto& object : mGeneratedObjects)
    {
        identifiers.push_back(object);
    }

    return identifiers;
}

tg::aabb3 gamedev::Factory::MaxBoundingBox(tg::aabb3& boundingBox)
{
    return boundingBox;

    auto min_scalar = tg::min(boundingBox.min.x, boundingBox.min.z);
    auto max_scalar = tg::min(boundingBox.max.x, boundingBox.max.z);
    return {tg::pos3(min_scalar), tg::pos3(max_scalar)};
}

bool gamedev::Factory::SaveObjectsTo(std::filesystem::path filepath)
{
    std::ofstream file(filepath, std::ios::out | std::ios::binary);

    if (!file)
    {
        glow::error() << "Could not save objects. Error occured on file open.";
        return false;
    }

    // clean up list before use
    mGeneratedObjects.remove_if([this](std::pair<InstanceHandle, std::string> obj) { return !mECS->IsLiveHandle(obj.first); });

    for (auto& object : mGeneratedObjects)
    {
        auto xform = mECS->GetInstance(object.first).xform;

        file.write((char*)object.second.c_str(), object.second.size()); // write string identifier
        file.write("\0", sizeof(char));                                 // write null end for easy reading of string

        file.write((char*)&xform.translation, sizeof(tg::vec3));        // write position
        file.write((char*)&xform.rotation, sizeof(tg::quat));           // write rotation
        file.write((char*)&xform.scaling, sizeof(tg::size3));           // write scaling
    }

    file.close();

    if (!file.good())
    {
        glow::error() << "Could not save objects. Error occured in writing.";
        return false;
    }

    return true;
}

bool gamedev::Factory::SaveMarkedObjectsTo(std::filesystem::path filepath)
{
    std::ofstream file(filepath, std::ios::out | std::ios::binary);

    if (!file)
    {
        glow::error() << "Could not save objects. Error occured on file open.";
        return false;
    }

    // clean up list before use
    mMarkedObjects.remove_if([this](std::pair<InstanceHandle, std::string> obj) { return !mECS->IsLiveHandle(obj.first); });

    for (auto& object : mMarkedObjects)
    {
        auto xform = mECS->GetInstance(object.first).xform;

        file.write((char*)object.second.c_str(), object.second.size()); // write string identifier
        file.write("\0", sizeof(char));                                 // write null end for easy reading of string

        file.write((char*)&xform.translation, sizeof(tg::vec3)); // write position
        file.write((char*)&xform.rotation, sizeof(tg::quat));    // write rotation
        file.write((char*)&xform.scaling, sizeof(tg::size3));    // write scaling
    }

    file.close();

    if (!file.good())
    {
        glow::error() << "Could not save objects. Error occured in writing.";
        return false;
    }

    return true;
}

std::vector<gamedev::InstanceHandle> gamedev::Factory::LoadObjectsFrom(std::filesystem::path filepath, bool marked)
{
    std::vector<InstanceHandle> handles;
    std::ifstream file(filepath, std::ios::in | std::ios::binary);

    if (!file)
    {
        glow::error() << "Could not save objects. Error occured on file open.";
        return handles;
    }

    auto filesize = std::filesystem::file_size(filepath);

    // process file until end
    while (file.tellg() < filesize)
    {
        std::string identifier;
        tg::vec3 translation;
        tg::quat rotation;
        tg::size3 scaling;

        std::getline(file, identifier, '\0');                 // read null ended string
        file.read((char*)&translation, sizeof(tg::vec3));     // read position
        file.read((char*)&rotation, sizeof(tg::quat));        // read rotation
        file.read((char*)&scaling, sizeof(tg::size3));        // read scaling

        auto handle = Create(identifier, tg::pos3(translation), rotation, scaling);   // create instance upon this
        mECS->GetInstanceTransform(handle).translation = translation;

        handles.push_back(handle);

        if (marked)
        {
            MarkLastCreated();
        }

        // ToDo: Maybe catch some errors in this process instead of producing a runtime error

    }

    return handles;
}

void gamedev::Factory::MarkLastCreated() { mMarkedObjects.push_back(mLastCreated); }

bool gamedev::Factory::SaveObjects() { return SaveObjectsTo(mSavefilePath); }
bool gamedev::Factory::SaveMarkedObjects(std::filesystem::path filepath) { return SaveMarkedObjectsTo(filepath); }
bool gamedev::Factory::LoadObjects() { return !LoadObjectsFrom(mSavefilePath).empty(); }
bool gamedev::Factory::LoadObjects(std::filesystem::path filepath)
{
    return !LoadObjectsFrom(mSavefilePath.parent_path().string() + "/" + filepath.string() + ".fac").empty();
}

std::vector<gamedev::InstanceHandle> gamedev::Factory::LoadObjectsMarked(std::filesystem::path filepath)
{
    return LoadObjectsFrom(mSavefilePath.parent_path().string() + "/" + filepath.string() + ".fac", true);
}