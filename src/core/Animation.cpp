#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include "Animation.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>

namespace ASIS {

static inline glm::mat4 convertMatrixToGLM(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

static inline glm::vec3 convertVectorToGLM(const aiVector3D& from) {
    return glm::vec3(from.x, from.y, from.z);
}

static inline glm::quat convertQuatToGLM(const aiQuaternion& from) {
    return glm::quat(from.w, from.x, from.y, from.z);
}

// ============================================================================
// BoneChannel
// ============================================================================

BoneChannel::BoneChannel(const std::string& name, int ID, const aiNodeAnim* channel)
    : m_name(name), m_id(ID), m_localTransform(1.0f)
{
    if (!channel) return;

    m_positions.reserve(channel->mNumPositionKeys);
    for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i) {
        KeyPosition data;
        data.position = convertVectorToGLM(channel->mPositionKeys[i].mValue);
        data.timeStamp = static_cast<float>(channel->mPositionKeys[i].mTime);
        m_positions.push_back(data);
    }

    m_rotations.reserve(channel->mNumRotationKeys);
    for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i) {
        KeyRotation data;
        data.orientation = convertQuatToGLM(channel->mRotationKeys[i].mValue);
        data.timeStamp = static_cast<float>(channel->mRotationKeys[i].mTime);
        m_rotations.push_back(data);
    }

    m_scales.reserve(channel->mNumScalingKeys);
    for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i) {
        KeyScale data;
        data.scale = convertVectorToGLM(channel->mScalingKeys[i].mValue);
        data.timeStamp = static_cast<float>(channel->mScalingKeys[i].mTime);
        m_scales.push_back(data);
    }
}

int BoneChannel::getPositionIndex(float animationTime) const {
    for (size_t index = 0; index < m_positions.size() - 1; ++index) {
        if (animationTime < m_positions[index + 1].timeStamp)
            return static_cast<int>(index);
    }
    return 0;
}

int BoneChannel::getRotationIndex(float animationTime) const {
    for (size_t index = 0; index < m_rotations.size() - 1; ++index) {
        if (animationTime < m_rotations[index + 1].timeStamp)
            return static_cast<int>(index);
    }
    return 0;
}

int BoneChannel::getScaleIndex(float animationTime) const {
    for (size_t index = 0; index < m_scales.size() - 1; ++index) {
        if (animationTime < m_scales[index + 1].timeStamp)
            return static_cast<int>(index);
    }
    return 0;
}

float BoneChannel::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const {
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    if (framesDiff <= 0.0001f) return 0.0f;
    return midWayLength / framesDiff;
}

glm::mat4 BoneChannel::interpolatePosition(float animationTime) const {
    if (m_positions.empty()) return glm::mat4(1.0f);
    if (m_positions.size() == 1) return glm::translate(glm::mat4(1.0f), m_positions[0].position);

    int p0Index = getPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_positions[p0Index].timeStamp, m_positions[p1Index].timeStamp, animationTime);
    glm::vec3 finalPosition = glm::mix(m_positions[p0Index].position, m_positions[p1Index].position, scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 BoneChannel::interpolateRotation(float animationTime) const {
    if (m_rotations.empty()) return glm::mat4(1.0f);
    if (m_rotations.size() == 1) {
        glm::quat rot = glm::normalize(m_rotations[0].orientation);
        return glm::mat4_cast(rot);
    }

    int p0Index = getRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_rotations[p0Index].timeStamp, m_rotations[p1Index].timeStamp, animationTime);
    glm::quat finalRotation = glm::slerp(m_rotations[p0Index].orientation, m_rotations[p1Index].orientation, scaleFactor);
    return glm::mat4_cast(glm::normalize(finalRotation));
}

glm::mat4 BoneChannel::interpolateScaling(float animationTime) const {
    if (m_scales.empty()) return glm::mat4(1.0f);
    if (m_scales.size() == 1) return glm::scale(glm::mat4(1.0f), m_scales[0].scale);

    int p0Index = getScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_scales[p0Index].timeStamp, m_scales[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(m_scales[p0Index].scale, m_scales[p1Index].scale, scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

void BoneChannel::update(float animationTime) {
    glm::mat4 translation = interpolatePosition(animationTime);
    glm::mat4 rotation = interpolateRotation(animationTime);
    glm::mat4 scale = interpolateScaling(animationTime);
    m_localTransform = translation * rotation * scale;
}

// ============================================================================
// SkeletalAnimation
// ============================================================================

SkeletalAnimation::SkeletalAnimation()
    : m_name("Default"), m_duration(0.0f), m_ticksPerSecond(25.0f), m_boneCounter(0)
{
}

SkeletalAnimation::SkeletalAnimation(const std::string& animationPath, const aiScene* scene)
    : m_name("Clip"), m_duration(0.0f), m_ticksPerSecond(25.0f), m_boneCounter(0)
{
    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0) return;

    auto animation = scene->mAnimations[0];
    m_name = animation->mName.C_Str();
    if (m_name.empty()) m_name = "Animation_1";

    m_duration = static_cast<float>(animation->mDuration);
    m_ticksPerSecond = animation->mTicksPerSecond != 0 ? static_cast<float>(animation->mTicksPerSecond) : 25.0f;

    readHierarchyData(m_rootNode, scene->mRootNode);
    readMissingBones(animation);
}

BoneChannel* SkeletalAnimation::findBone(const std::string& name) {
    for (auto& bone : m_bones) {
        if (bone.getBoneName() == name) return &bone;
    }
    return nullptr;
}

void SkeletalAnimation::readMissingBones(const aiAnimation* animation) {
    if (!animation) return;

    for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (m_boneInfoMap.find(boneName) == m_boneInfoMap.end()) {
            m_boneInfoMap[boneName].id = m_boneCounter++;
            m_boneInfoMap[boneName].offset = glm::mat4(1.0f);
        }
        m_bones.push_back(BoneChannel(channel->mNodeName.data, m_boneInfoMap[boneName].id, channel));
    }
}

void SkeletalAnimation::readHierarchyData(AssimpNodeData& dest, const aiNode* src) {
    if (!src) return;

    dest.name = src->mName.data;
    dest.transformation = convertMatrixToGLM(src->mTransformation);
    dest.children.reserve(src->mNumChildren);

    for (unsigned int i = 0; i < src->mNumChildren; ++i) {
        AssimpNodeData childData;
        readHierarchyData(childData, src->mChildren[i]);
        dest.children.push_back(childData);
    }
}

// ============================================================================
// Animator
// ============================================================================

Animator::Animator()
    : m_currentAnimation(nullptr), m_currentTime(0.0f),
      m_playbackSpeed(1.0f), m_isPlaying(true), m_loop(true)
{
    m_finalBoneMatrices.reserve(100);
    for (int i = 0; i < 100; ++i) {
        m_finalBoneMatrices.push_back(glm::mat4(1.0f));
    }
}

void Animator::setTime(float time) {
    m_currentTime = time;
    if (m_currentAnimation) {
        float dur = m_currentAnimation->getDuration();
        if (dur > 0.0f) {
            m_currentTime = std::fmod(m_currentTime, dur);
        }
        calculateBoneTransform(&m_currentAnimation->getRootNode(), glm::mat4(1.0f));
    }
}

void Animator::updateAnimation(float dt) {
    if (!m_currentAnimation || !m_isPlaying) return;

    m_currentTime += m_currentAnimation->getTicksPerSecond() * dt * m_playbackSpeed;
    float dur = m_currentAnimation->getDuration();

    if (dur > 0.0f) {
        if (m_loop) {
            m_currentTime = std::fmod(m_currentTime, dur);
        } else if (m_currentTime > dur) {
            m_currentTime = dur;
            m_isPlaying = false;
        }
    }

    calculateBoneTransform(&m_currentAnimation->getRootNode(), glm::mat4(1.0f));
}

void Animator::playAnimation(std::shared_ptr<SkeletalAnimation> animation) {
    m_currentAnimation = animation;
    m_currentTime = 0.0f;
    m_isPlaying = true;
    if (m_currentAnimation) {
        calculateBoneTransform(&m_currentAnimation->getRootNode(), glm::mat4(1.0f));
    }
}

void Animator::calculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform) {
    if (!node || !m_currentAnimation) return;

    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    BoneChannel* bone = m_currentAnimation->findBone(nodeName);
    if (bone) {
        bone->update(m_currentTime);
        nodeTransform = bone->getLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    const auto& boneInfoMap = m_currentAnimation->getBoneInfoMap();
    auto it = boneInfoMap.find(nodeName);
    if (it != boneInfoMap.end()) {
        int index = it->second.id;
        glm::mat4 offset = it->second.offset;
        if (index >= 0 && index < (int)m_finalBoneMatrices.size()) {
            m_finalBoneMatrices[index] = globalTransformation * offset;
        }
    }

    for (const auto& child : node->children) {
        calculateBoneTransform(&child, globalTransformation);
    }
}

} // namespace ASIS
