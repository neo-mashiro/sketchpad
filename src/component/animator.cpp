#include "pch.h"

#include "core/debug.h"
#include "core/log.h"
#include "component/animator.h"
#include "utils/ext.h"

using namespace utils;

namespace component {

    static constexpr glm::mat4 identity_m = glm::mat4(1.0f);
    static constexpr glm::quat identity_q = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    static inline glm::mat4 AssimpMat2GLM(const aiMatrix4x4& m) {
        return glm::transpose(glm::make_mat4(&m.a1));  // aiMatrix4x4 is in row-major order so we transpose
    }

    Channel::Channel(aiNodeAnim* ai_channel, const std::string& name, int id, float duration)
        : name(name), bone_id(id)
    {
        unsigned int n_positions = ai_channel->mNumPositionKeys;
        unsigned int n_rotations = ai_channel->mNumRotationKeys;
        unsigned int n_scales    = ai_channel->mNumScalingKeys;

        if (n_positions < 1 || n_rotations < 1 || n_scales < 1) {
            throw core::ConditionError("Invalid animation channel, require at least one frame per key...");
        }

        // in order for interpolation to work, at least 2 frames per key are needed, if a
        // key only has one frame at timestamp 0, we shall duplicate it to make an ending
        // frame at timestamp duration, so that there's always a well-defined transition

        positions.resize(n_positions + 1, FT(glm::vec3(0.0f), -1.0f));
        rotations.resize(n_rotations + 1, FR(identity_q, -1.0f));
        scales.resize(n_scales + 1, FS(glm::vec3(1.0f), -1.0f));

        // Assimp guarantees that keyframes will be returned in chronological order and there
        // will be no duplicates, so we don't need to manually sort in the order of timestamp

        for (auto [i, prev_time] = std::tuple(0U, 0.0f); i < n_positions; ++i) {
            auto& frame = ai_channel->mPositionKeys[i];
            auto& value = frame.mValue;
            auto timestamp = static_cast<float>(frame.mTime);

            CORE_ASERT(timestamp >= prev_time, "Assimp failed to return frames in chronological order!");
            positions[i].value = glm::vec3(value.x, value.y, value.z);
            positions[i].timestamp = timestamp;
            prev_time = timestamp;

            if (i == 0) {  // mirror the first frame into the last frame so it loops
                auto& last_frame = positions.back();
                last_frame.value = glm::vec3(value.x, value.y, value.z);
                last_frame.timestamp = duration;
            }
        }

        for (auto [i, prev_time] = std::tuple(0U, 0.0f); i < n_rotations; ++i) {
            auto& frame = ai_channel->mRotationKeys[i];
            auto& value = frame.mValue;
            auto timestamp = static_cast<float>(frame.mTime);

            CORE_ASERT(timestamp >= prev_time, "Assimp failed to return frames in chronological order!");
            rotations[i].value = glm::quat(value.w, value.x, value.y, value.z);
            rotations[i].timestamp = timestamp;
            prev_time = timestamp;

            if (i == 0) {  // mirror the first frame into the last frame so it loops
                auto& last_frame = rotations.back();
                last_frame.value = glm::quat(value.w, value.x, value.y, value.z);
                last_frame.timestamp = duration;
            }
        }

        for (auto [i, prev_time] = std::tuple(0U, 0.0f); i < n_scales; ++i) {
            auto& frame = ai_channel->mScalingKeys[i];
            auto& value = frame.mValue;
            auto timestamp = static_cast<float>(frame.mTime);

            CORE_ASERT(timestamp >= prev_time, "Assimp failed to return frames in chronological order!");
            scales[i].value = glm::vec3(value.x, value.y, value.z);
            scales[i].timestamp = timestamp;
            prev_time = timestamp;

            if (i == 0) {  // mirror the first frame into the last frame so it loops
                auto& last_frame = scales.back();
                last_frame.value = glm::vec3(value.x, value.y, value.z);
                last_frame.timestamp = duration;
            }
        }
    }

    template<typename TFrame>
    std::tuple<int, int> Channel::GetFrameIndex(const std::vector<TFrame>& frames, float time) const {
        if (frames.size() == 1) {
            return std::tuple(0, 0);
        }

        for (int i = 1; i < frames.size(); ++i) {
            auto& [_, timestamp] = frames.at(i);
            if (time < timestamp) {
                return std::tuple(i - 1, i);
            }
        }
        return std::tuple(-1, -1);  // if -1, animation is not looping
    }

    glm::mat4 Channel::Interpolate(float time) const {
        // for each key, extract value and timestamp from the previous and the next frame
        auto [i1, i2] = GetFrameIndex<FT>(positions, time);
        auto& [prev_position, prev_ts_t] = positions[i1];
        auto& [next_position, next_ts_t] = positions[i2];

        auto [j1, j2] = GetFrameIndex<FR>(rotations, time);
        auto& [prev_rotation, prev_ts_r] = rotations[j1];
        auto& [next_rotation, next_ts_r] = rotations[j2];

        auto [k1, k2] = GetFrameIndex<FS>(scales, time);
        auto& [prev_scale, prev_ts_s] = scales[k1];
        auto& [next_scale, next_ts_s] = scales[k2];

        // for each key, compute the blending weight between the two frames
        float percent_t = math::LinearPercent(prev_ts_t, next_ts_t, time);
        float percent_r = math::LinearPercent(prev_ts_r, next_ts_r, time);
        float percent_s = math::LinearPercent(prev_ts_s, next_ts_s, time);

        using namespace glm;

        // interpolation of vectors and quaternion
        vec3 new_position = math::Lerp(prev_position, next_position, percent_t);
        quat new_rotation = math::Slerp(prev_rotation, next_rotation, percent_r);
        vec3 new_scale = math::Lerp(prev_scale, next_scale, percent_s);

        // combine into a single transform matrix and return
        mat4 translation = glm::translate(identity_m, new_position);
        mat4 rotation = glm::toMat4(new_rotation);
        mat4 scale = glm::scale(identity_m, new_scale);

        return translation * rotation * scale;
    }

    Animation::Animation(const aiScene* ai_scene, Model* model) : n_channels(0) {
        CORE_ASERT(ai_scene->mNumAnimations > 0, "The input file does not contain animations!");
        aiAnimation* ai_animation = ai_scene->mAnimations[0];

        name     = ai_animation->mName.C_Str();
        duration = static_cast<float>(ai_animation->mDuration);
        speed    = static_cast<float>(ai_animation->mTicksPerSecond);

        channels.resize(model->n_bones);  // match channels with bones, resize instead of reserve
        auto& nodes = model->nodes;

        for (unsigned int i = 0; i < ai_animation->mNumChannels; ++i) {
            aiNodeAnim* ai_channel = ai_animation->mChannels[i];
            std::string bone_name = ai_channel->mNodeName.C_Str();

            auto node = ranges::find_if(nodes, [&bone_name](const Node& node) {
                return node.name == bone_name;
            });

            if (node == nodes.end()) {
                continue;  // drop the channel if there's no matching node in the hierarchy
            }

            if (!node->IsBone()) {
                continue;  // drop the channel if Assimp doesn't think it's a bone
            }

            Channel& channel = channels[node->bid];
            CORE_ASERT(channel.bone_id < 0, "This channel is already filled, duplicate bone!");

            channel = std::move(Channel(ai_channel, bone_name, node->bid, duration));
            nodes[node->nid].alive = true;
            n_channels++;
        }

        unsigned int cnt = ranges::count_if(channels, [](const Channel& c) { return c.bone_id >= 0; });
        CORE_ASERT(n_channels == cnt, "Incorrect channels count, must match bones 1 on 1!");
        CORE_ASERT(n_channels <= model->n_bones, "Invalid channels are not dropped, please clean up!");
    }

    Animator::Animator(Model* model) { Reset(model); }

    void Animator::Reset(Model* model) {
        CORE_ASERT(model->animation, "Model doesn't have animation!");
        bone_transforms.resize(model->n_bones, identity_m);
        current_time = 0.0f;
    }

    void Animator::Update(Model& model, float deltatime) {
        const auto& animation = model.animation;
        current_time += animation->speed * deltatime;
        current_time = fmod(current_time, animation->duration);  // loop the clip

        const auto& channels = animation->channels;
        auto& nodes = model.nodes;

        // on update, we only need to iterate over the nodes vector once, in hierarchical order, so
        // that a parent node is always updated before its children, matrices can be easily chained

        for (auto& node : nodes) {
            int& bone_id = node.bid;
            int& parent_id = node.pid;

            glm::mat4 n2p = node.Animated() ? channels[bone_id].Interpolate(current_time) : node.n2p;
            glm::mat4 p2m = parent_id < 0 ? glm::inverse(nodes[0].n2p) : nodes[parent_id].n2m;
            node.n2m = p2m * n2p;

            if (node.IsBone()) {
                bone_transforms[bone_id] = node.n2m * node.m2n;
            }
        }
    }

}
