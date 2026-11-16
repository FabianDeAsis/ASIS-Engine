#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <assimp/scene.h>

namespace ASIS {

struct BoneInfo {
    int id;
    glm::mat4 offset;
};

struct KeyPosition {
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation {
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale {
    glm::vec3 scale;
    float timeStamp;
};

class BoneChannel {
public:
    BoneChannel(const std::string& name, int ID, const aiNodeAnim* channel);

    void update(float animationTime);
    glm::mat4 getLocalTransform() const { return m_localTransform; }
    const std::string& getBoneName() const { return m_name; }
    int getBoneID() const { return m_id; }

private:
    std::string m_name;
    int m_id;
    glm::mat4 m_localTransform;

    std::vector<KeyPosition> m_positions;
    std::vector<KeyRotation> m_rotations;
    std::vector<KeyScale> m_scales;

    int getPositionIndex(float animationTime) const;
    int getRotationIndex(float animationTime) const;
    int getScaleIndex(float animationTime) const;

    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;
    glm::mat4 interpolatePosition(float animationTime) const;
    glm::mat4 interpolateRotation(float animationTime) const;
    glm::mat4 interpolateScaling(float animationTime) const;
};

struct AssimpNodeData {
    glm::mat4 transformation;
    std::string name;
    std::vector<AssimpNodeData> children;
};

class SkeletalAnimation {
public:
    SkeletalAnimation();
    SkeletalAnimation(const std::string& animationPath, const aiScene* scene);
    ~SkeletalAnimation() = default;

    BoneChannel* findBone(const std::string& name);

    float getTicksPerSecond() const { return m_ticksPerSecond; }
    float getDuration() const { return m_duration; }
    const AssimpNodeData& getRootNode() const { return m_rootNode; }
    const std::map<std::string, BoneInfo>& getBoneInfoMap() const { return m_boneInfoMap; }
    std::map<std::string, BoneInfo>& getBoneInfoMapMutable() { return m_boneInfoMap; }
    int getBoneCount() const { return m_boneCounter; }
    const std::string& getName() const { return m_name; }

private:
    std::string m_name;
    float m_duration;
    float m_ticksPerSecond;
    std::vector<BoneChannel> m_bones;
    AssimpNodeData m_rootNode;
    std::map<std::string, BoneInfo> m_boneInfoMap;
    int m_boneCounter;

    void readMissingBones(const aiAnimation* animation);
    void readHierarchyData(AssimpNodeData& dest, const aiNode* src);
};

class Animator {
public:
    Animator();
    ~Animator() = default;

    void updateAnimation(float dt);
    void playAnimation(std::shared_ptr<SkeletalAnimation> animation);
    void setPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
    float getPlaybackSpeed() const { return m_playbackSpeed; }
    void setPlaying(bool play) { m_isPlaying = play; }
    bool isPlaying() const { return m_isPlaying; }
    void setLoop(bool loop) { m_loop = loop; }
    bool isLoop() const { return m_loop; }
    void setTime(float time);
    float getCurrentTime() const { return m_currentTime; }
    float getDuration() const { return m_currentAnimation ? m_currentAnimation->getDuration() : 0.0f; }

    const std::vector<glm::mat4>& getFinalBoneMatrices() const { return m_finalBoneMatrices; }
    bool hasActiveAnimation() const { return m_currentAnimation != nullptr; }
    std::shared_ptr<SkeletalAnimation> getCurrentAnimation() const { return m_currentAnimation; }

private:
    std::vector<glm::mat4> m_finalBoneMatrices;
    std::shared_ptr<SkeletalAnimation> m_currentAnimation;
    float m_currentTime;
    float m_playbackSpeed;
    bool m_isPlaying;
    bool m_loop;

    void calculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform);
};

} // namespace ASIS
