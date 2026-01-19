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
        const float stepTime = 0.005f;
        float remainingTime = deltaTime;
        float carRadius = 1.0f; // 赛车的碰撞体积大小

        while (remainingTime > 0.0f) {
            float dt = glm::min(remainingTime, stepTime);
            glm::vec3 oldPos = pos;

            // 尝试前进
            pos.x += glm::sin(glm::radians(heading)) * speed * dt;
            pos.z += glm::cos(glm::radians(heading)) * speed * dt;
            pos.y = terrain.getHeightWorld(pos.x, pos.z);

            // 检查这一小步是否撞到了猫
            if (checkCollision(pos, carRadius)) {
                pos = oldPos;         // 撞到了，退回上一步的位置
                speed = -speed * 0.5f; // 速度反向（反弹效果）
                break;                // 停止本帧后续的小步移动
            }
            remainingTime -= dt;
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