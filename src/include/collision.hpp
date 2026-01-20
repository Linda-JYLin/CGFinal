#ifndef COLLISION_HPP
#define COLLISION_HPP

#include <glm/glm.hpp>
#include <vector>
#include "include/terrain/terrain.hpp"

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

class CollisionSystem {
public:
    // 获取单例障碍物列表
    static std::vector<BoundingBox>& getObstacles() {
        static std::vector<BoundingBox> obstacles;
        return obstacles;
    }

    // 每一帧开始前清空旧的动态碰撞盒
    static void clearObstacles() {
        getObstacles().clear();
    }

    // 核心碰撞检测：增加一个 radius 让赛车有一定体积
    static bool checkCollision(const glm::vec3& pos, float radius) {
        for (const auto& box : getObstacles()) {
            if (pos.x + radius > box.min.x && pos.x - radius < box.max.x &&
                pos.y + radius > box.min.y && pos.y - radius < box.max.y &&
                pos.z + radius > box.min.z && pos.z - radius < box.max.z) {
                return true;
            }
        }
        return false;
    }

    static void updatePositionWithPhysics(glm::vec3& pos, float heading, float& speed, float deltaTime, const Terrain& terrain) {
        float frameTime = glm::min(deltaTime, 0.05f);

        // 1. 计算预测位置
        glm::vec3 nextPos = pos;
        float moveDist = speed * frameTime;
        nextPos.x += glm::sin(glm::radians(heading)) * moveDist;
        nextPos.z += glm::cos(glm::radians(heading)) * moveDist;
        nextPos.y = terrain.getHeightWorld(nextPos.x, nextPos.z);

        // 2. 关键：碰撞检测点上移 (抬高 1.0f)，避免车底卡进地形触发碰撞
        glm::vec3 checkCenter = nextPos + glm::vec3(0.0f, 1.0f, 0.0f);
        float carRadius = 1.2f;

        if (checkCollision(checkCenter, carRadius)) {
            // 碰撞反馈：大幅减速并产生微弱反弹，但不完全切断位移，防止视觉瞬间静止
            speed = -speed * 0.1f;
            // 这里可以选择不更新 pos，或者让 pos 稍微移动一点点
        }
        else {
            pos = nextPos;
        }
    }

    // 相机防入地
    static glm::vec3 resolveCameraCollision(const glm::vec3& targetCamPos, const Terrain& terrain) {
        float h = terrain.getHeightWorld(targetCamPos.x, targetCamPos.z);
        glm::vec3 finalPos = targetCamPos;
        if (finalPos.y < h + 0.8f) finalPos.y = h + 0.8f;
        return finalPos;
    }
};

#endif