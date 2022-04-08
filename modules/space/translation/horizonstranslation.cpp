/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2022                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <modules/space/translation/horizonstranslation.h>

#include <openspace/documentation/documentation.h>
#include <openspace/documentation/verifier.h>
#include <openspace/util/updatestructures.h>
#include <ghoul/fmt.h>
#include <ghoul/filesystem/cachemanager.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/lua/ghoul_lua.h>
#include <ghoul/lua/lua_helper.h>
#include <filesystem>
#include <fstream>
#include <type_traits>

namespace {
    constexpr const char* _loggerCat = "HorizonsTranslation";
    constexpr int8_t CurrentCacheVersion = 2;
} // namespace

namespace {
    constexpr openspace::properties::Property::PropertyInfo HorizonsTextFileInfo = {
        "HorizonsTextFile",
        "Horizons Text File",
        "This value is the path to the file or files generated by Horizons with "
        "either a Vector table or an Observer table with the correct settings."
    };

    struct [[codegen::Dictionary(HorizonsTranslation)]] Parameters {
        // [[codegen::verbatim(HorizonsTextFileInfo.description)]]
        std::variant<std::string, std::vector<std::string>> horizonsTextFile;
    };
#include "horizonstranslation_codegen.cpp"
} // namespace

namespace openspace {

documentation::Documentation HorizonsTranslation::Documentation() {
    return codegen::doc<Parameters>("base_transform_translation_horizons");
}

HorizonsTranslation::HorizonsTranslation()
    : _horizonsTextFiles(HorizonsTextFileInfo)
{
    addProperty(_horizonsTextFiles);

    _horizonsTextFiles.onChange([&](){
        requireUpdate();
        notifyObservers();
        loadData();
    });
}

HorizonsTranslation::HorizonsTranslation(const ghoul::Dictionary& dictionary)
    : HorizonsTranslation()
{
    const Parameters p = codegen::bake<Parameters>(dictionary);

    if (std::holds_alternative<std::string>(p.horizonsTextFile)) {
        std::string file = std::get<std::string>(p.horizonsTextFile);
        if (!std::filesystem::is_regular_file(absPath(file))) {
            LWARNING(fmt::format("The Horizons text file '{}' could not be found", file));
            return;
        }

        std::vector<std::string> files;
        files.push_back(file);
        _horizonsTextFiles.set(files);
    }
    else if (std::holds_alternative<std::vector<std::string>>(p.horizonsTextFile)) {
        std::vector<std::string> files =
            std::get<std::vector<std::string>>(p.horizonsTextFile);

        for (const std::string& file : files) {
            if (!std::filesystem::is_regular_file(absPath(file))) {
                LWARNING(fmt::format(
                    "The Horizons text file '{}' could not be found", file
                ));
                return;
            }
        }
        _horizonsTextFiles = files;
    }
    else {
        throw ghoul::MissingCaseException();
    }
}

glm::dvec3 HorizonsTranslation::position(const UpdateData& data) const {
    glm::dvec3 interpolatedPos = glm::dvec3(0.0);

    const Keyframe<glm::dvec3>* lastBefore =
        _timeline.lastKeyframeBefore(data.time.j2000Seconds(), true);
    const Keyframe<glm::dvec3>* firstAfter =
        _timeline.firstKeyframeAfter(data.time.j2000Seconds(), false);

    if (lastBefore && firstAfter) {
        // We're inbetween first and last value.
        double timelineDiff = firstAfter->timestamp - lastBefore->timestamp;
        double timeDiff = data.time.j2000Seconds() - lastBefore->timestamp;
        double diff = (timelineDiff > DBL_EPSILON) ? timeDiff / timelineDiff : 0.0;

        glm::dvec3 dir = firstAfter->data - lastBefore->data;
        interpolatedPos = lastBefore->data + dir * diff;
    }
    else if (lastBefore) {
        // Requesting a time after last value. Return last known position.
        interpolatedPos = lastBefore->data;
    }
    else if (firstAfter) {
        // Requesting a time before first value. Return last known position.
        interpolatedPos = firstAfter->data;
    }

    return interpolatedPos;
}

void HorizonsTranslation::loadData() {
    for (const std::string& filePath : _horizonsTextFiles.value()) {
        std::filesystem::path file = absPath(filePath);
        if (!std::filesystem::is_regular_file(file)) {
            LWARNING(fmt::format("The Horizons text file '{}' could not be found", file));
            return;
        }

        std::filesystem::path cachedFile = FileSys.cacheManager()->cachedFilename(file);
        bool hasCachedFile = std::filesystem::is_regular_file(cachedFile);
        if (hasCachedFile) {
            LINFO(fmt::format(
                "Cached file '{}' used for Horizon file '{}'", cachedFile, file
            ));

            if (loadCachedFile(cachedFile)) {
                continue;
            }
            else {
                FileSys.cacheManager()->removeCacheFile(file);
                // Intentional fall-through to the 'else' computation to generate the
                // cache file for the next run
            }
        }
        else {
            LINFO(fmt::format("Cache for Horizon file '{}' not found", file));
        }
        LINFO(fmt::format("Loading Horizon file '{}'", file));

        HorizonsFile horizonsFile(file);
        if (!readHorizonsTextFile(horizonsFile)) {
            LERROR(fmt::format("Could not read data from Horizons file '{}'", file));
            return;
        }

        LINFO("Saving cache");
        saveCachedFile(cachedFile);
    }
}

bool HorizonsTranslation::readHorizonsTextFile(HorizonsFile& horizonsFile) {
    HorizonsResult result = readHorizonsFile(horizonsFile.file());
    if (result.errorCode != HorizonsResultCode::Valid) {
        horizonsFile.displayErrorMessage(result.errorCode);
        return false;
    }

    for (HorizonsKeyframe& keyframe : result.data) {
        // Search if the keyframe already exist in the timeline
        auto it = std::find_if(
            _timeline.keyframes().begin(),
            _timeline.keyframes().end(),
            [keyframe](const Keyframe<glm::dvec3>& kf) {
                return kf.timestamp == keyframe.time;
            }
        );

        // If it doesn't exist in the timeline then add it, prevent duplicates
        if (it == _timeline.keyframes().end()) {
            _timeline.addKeyframe(keyframe.time, std::move(keyframe.position));
        }
    }
    return true;
}

bool HorizonsTranslation::loadCachedFile(const std::filesystem::path& file) {
    std::ifstream fileStream(file, std::ifstream::binary);

    if (!fileStream.good()) {
        LERROR(fmt::format("Error opening file {} for loading cache file", file));
        return false;
    }

    // Check the caching version
    int8_t version = 0;
    fileStream.read(reinterpret_cast<char*>(&version), sizeof(int8_t));
    if (version != CurrentCacheVersion) {
        LINFO("The format of the cached file has changed: deleting old cache");
        fileStream.close();
        FileSys.cacheManager()->removeCacheFile(file);
        return false;
    }

    // Read how many keyframes to read
    int32_t nKeyframes = 0;

    fileStream.read(reinterpret_cast<char*>(&nKeyframes), sizeof(int32_t));
    if (nKeyframes == 0) {
        throw ghoul::RuntimeError("Error reading cache: No values were loaded");
    }

    // Read all data in one go
    std::vector<CacheKeyframe> cacheKeyframes;
    cacheKeyframes.reserve(nKeyframes);
    fileStream.read(reinterpret_cast<char*>(cacheKeyframes.data()), sizeof(CacheKeyframe) * nKeyframes);

    // Extract the data from the cache Keyframe vector
    for (int i = 0; i < nKeyframes; ++i) {
        // Add keyframe in timeline
        _timeline.addKeyframe(std::move(cacheKeyframes[i].timestamp), std::move(cacheKeyframes[i].position));
    }

    return fileStream.good();
}

void HorizonsTranslation::saveCachedFile(const std::filesystem::path& file) const {
    std::ofstream fileStream(file, std::ofstream::binary);
    if (!fileStream.good()) {
        LERROR(fmt::format("Error opening file {} for save cache file", file));
        return;
    }

    // Write which version of caching that is used
    fileStream.write(
        reinterpret_cast<const char*>(&CurrentCacheVersion),
        sizeof(int8_t)
    );

    // Write how many keyframes are to be written
    int32_t nKeyframes = static_cast<int32_t>(_timeline.nKeyframes());
    if (nKeyframes == 0) {
        throw ghoul::RuntimeError("Error writing cache: No values were loaded");
    }
    fileStream.write(reinterpret_cast<const char*>(&nKeyframes), sizeof(int32_t));

    // Transfer all data to a cache key frame vector, write it all in one go
    std::deque<Keyframe<glm::dvec3>> keyframes = _timeline.keyframes();
    std::vector<CacheKeyframe> cachKeyframes;
    cachKeyframes.reserve(nKeyframes);
    for (int i = 0; i < nKeyframes; i++) {
        CacheKeyframe cacheKeyframe;
        cacheKeyframe.timestamp = keyframes[i].timestamp;
        cacheKeyframe.position = keyframes[i].data;

        cachKeyframes.push_back(cacheKeyframe);
    }

    // Write of entire vector will only work if the data is plain old data type,
    // is_pod is depricated in C++20 and replaced with both is_trivial and
    // is_standard_layout
    assert(std::is_trivial<CacheKeyframe>::value);
    assert(std::is_standard_layout<CacheKeyframe>::value);

    // Write data
    fileStream.write(
        reinterpret_cast<const char*>(cachKeyframes.data()),
        sizeof(CacheKeyframe) * nKeyframes
    );
}
} // namespace openspace
