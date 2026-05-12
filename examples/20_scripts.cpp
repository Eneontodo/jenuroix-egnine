// ════════════════════════════════════════════════════════════════════════════
//  Example 20 — 30 ready-to-use scripts for Jenuroix Engine
//
//  Categories:
//    A. Transform  (вращение, движение, следование, пружина)
//    B. Физика         (прыжок, платформа, магнит, взрыв, ветер)
//    C. Visual         (мигание, пульс, смена цвета, растворение)
//    D. Audio          (триггер звука, 3D-эхо, ритм)
//    E. Game Logic (таймер, здоровье, сбор, патруль, ИИ)
//    F. Camera         (слежение, тряска, зум)
//
//  Usage:
//    Copy any script and attach via obj.onUpdate(...)
// ════════════════════════════════════════════════════════════════════════════
#include "engine.h"
#include "audio/Audio.h"
#include <cmath>
using namespace eng;

// ── Helper functions ───────────────────────────────────────────────────
static float lerp (float a, float b, float t) { return a + (b-a)*t; }
static glm::vec3 lerp3(glm::vec3 a, glm::vec3 b, float t){ return a + (b-a)*t; }

// ─────────────────────────────────────────────────────────────────────────────
//  A. ТРАНСФОРМАЦИЯ
// ─────────────────────────────────────────────────────────────────────────────

// A1. Вращение вокруг оси Y (карусель)
auto ScriptRotateY(float speed = 90.f) {
    return [speed](GameObject& self, float dt) {
        self.addRot(0, speed * dt, 0);
    };
}

// A2. Вращение по всем трём осям
auto ScriptSpin(float rx = 30.f, float ry = 60.f, float rz = 20.f) {
    return [rx, ry, rz](GameObject& self, float dt) {
        self.addRot(rx*dt, ry*dt, rz*dt);
    };
}

// A3. Движение по синусоиде (висит и покачивается)
auto ScriptHover(float amplitude = 0.5f, float freq = 1.5f) {
    float baseY = 0.f;
    bool  init  = false;
    return [=](GameObject& self, float dt) mutable {
        if (!init) { baseY = self.y(); init = true; }
        float t = (float)glfwGetTime();
        self.y(baseY + std::sin(t * freq * 6.2832f) * amplitude);
    };
}

// A4. Следование за целью (плавное)
auto ScriptFollowTarget(glm::vec3* target, float speed = 3.f) {
    return [target, speed](GameObject& self, float dt) {
        glm::vec3 dir = *target - self.pos();
        float dist = glm::length(dir);
        if (dist > 0.1f)
            self.move(glm::normalize(dir) * std::min(speed * dt, dist));
    };
}

// A5. Пружинное движение к цели
auto ScriptSpring(glm::vec3 target, float stiffness = 8.f, float damping = 0.85f) {
    glm::vec3 vel = {};
    return [=](GameObject& self, float dt) mutable {
        glm::vec3 diff = target - self.pos();
        vel = vel * damping + diff * stiffness * dt;
        self.move(vel * dt);
    };
}

// A6. Орбита вокруг точки
auto ScriptOrbit(glm::vec3 center, float radius = 3.f, float speed = 1.f) {
    return [=](GameObject& self, float dt) {
        float t = (float)glfwGetTime() * speed;
        self.pos(center.x + std::cos(t) * radius,
                 center.y,
                 center.z + std::sin(t) * radius);
    };
}

// A7. Покачивание (маятник)
auto ScriptPendulum(float angle = 30.f, float freq = 1.f) {
    return [=](GameObject& self, float dt) {
        float t = (float)glfwGetTime();
        self.rotZ(std::sin(t * freq * 6.2832f) * angle);
    };
}

// A8. Движение по пути (waypoints)
auto ScriptWaypoints(std::vector<glm::vec3> points, float speed = 2.f) {
    int   idx  = 0;
    return [=](GameObject& self, float dt) mutable {
        if (points.empty()) return;
        glm::vec3 target = points[idx % points.size()];
        glm::vec3 dir    = target - self.pos();
        float dist = glm::length(dir);
        if (dist < 0.2f) { idx++; return; }
        self.move(glm::normalize(dir) * speed * dt);
        self.lookAt(target);
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  B. ФИЗИКА
// ─────────────────────────────────────────────────────────────────────────────

// B1. Прыжок по нажатию Space
auto ScriptJump(App& app, float force = 400.f) {
    return [&app, force](GameObject& self, float dt) {
        if (app.keyDown(GLFW_KEY_SPACE))
            self.impulse(0, force, 0);
    };
}

// B2. Движущаяся платформа (кинематика)
auto ScriptMovingPlatform(glm::vec3 posA, glm::vec3 posB, float speed = 1.5f) {
    float t = 0.f;
    int   dir = 1;
    return [=](GameObject& self, float dt) mutable {
        t += dt * speed * dir;
        if (t >= 1.f) { t = 1.f; dir = -1; }
        if (t <= 0.f) { t = 0.f; dir =  1; }
        self.pos(lerp3(posA, posB, t));
    };
}

// B3. Магнит — притягивает объекты вокруг
auto ScriptMagnet(App& app, const std::string& targetTag,
                  float radius = 5.f, float force = 200.f) {
    return [&app, targetTag, radius, force](GameObject& self, float dt) {
        app.world().each<TagComp, TransformComponent>(
            [&](EntityID id, TagComp& tag, TransformComponent& t) {
                if (tag.value != targetTag) return;
                glm::vec3 dir = self.pos() - t.position;
                float dist    = glm::length(dir);
                if (dist < radius && dist > 0.01f)
                    app.physics().applyForce(id,
                        glm::normalize(dir) * force * (1.f - dist/radius));
            });
    };
}

// B4. Взрыв — разовый импульс во все стороны
auto ScriptExplode(App& app, float radius = 5.f, float force = 800.f) {
    bool done = false;
    return [&app, radius, force, done](GameObject& self, float dt) mutable {
        if (done) return;
        done = true;
        app.world().each<RigidBodyComponent, TransformComponent>(
            [&](EntityID id, RigidBodyComponent&, TransformComponent& t) {
                glm::vec3 dir  = t.position - self.pos();
                float     dist = glm::length(dir);
                if (dist < radius && dist > 0.01f)
                    app.physics().applyImpulse(id,
                        glm::normalize(dir) * force * (1.f - dist/radius));
            });
    };
}

// B5. Ветер — постоянная сила в одну сторону
auto ScriptWind(App& app, glm::vec3 direction = {1,0,0}, float strength = 50.f) {
    return [&app, direction, strength](GameObject& self, float dt) {
        if (auto* rb = app.world().get<RigidBodyComponent>(self.id()))
            app.physics().applyForce(self.id(), direction * strength);
    };
}

// B6. Граница — не давать выйти за пределы зоны
auto ScriptBoundary(glm::vec3 center, float radius = 10.f) {
    return [=](GameObject& self, float dt) {
        glm::vec3 diff = self.pos() - center;
        if (glm::length(diff) > radius)
            self.pos(center + glm::normalize(diff) * radius);
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  C. ВИЗУАЛ
// ─────────────────────────────────────────────────────────────────────────────

// C1. Мигание (включение-выключение)
auto ScriptBlink(float interval = 0.5f) {
    float timer = 0.f;
    return [=](GameObject& self, float dt) mutable {
        timer += dt;
        if (timer >= interval) { self.toggle(); timer = 0.f; }
    };
}

// C2. Пульс масштаба
auto ScriptPulse(float minScale = 0.8f, float maxScale = 1.2f, float speed = 2.f) {
    return [=](GameObject& self, float dt) {
        float t = (float)glfwGetTime() * speed;
        float s = minScale + (maxScale - minScale) * (std::sin(t * 6.2832f) * 0.5f + 0.5f);
        self.scale(s);
    };
}

// C3. Смена цвета по времени (радуга)
auto ScriptRainbow(float speed = 1.f) {
    return [=](GameObject& self, float dt) {
        float t = (float)glfwGetTime() * speed;
        float r = std::sin(t)           * 0.5f + 0.5f;
        float g = std::sin(t + 2.094f)  * 0.5f + 0.5f;
        float b = std::sin(t + 4.189f)  * 0.5f + 0.5f;
        self.color(r, g, b);
    };
}

// C4. Растворение (fade out)
auto ScriptFadeOut(float duration = 2.f) {
    float timer = 0.f;
    return [=](GameObject& self, float dt) mutable {
        timer += dt;
        float a = std::max(0.f, 1.f - timer / duration);
        self.alpha(a);
        if (a <= 0.f) self.hide();
    };
}

// C5. Появление (fade in)
auto ScriptFadeIn(float duration = 1.f) {
    float timer = 0.f;
    return [=](GameObject& self, float dt) mutable {
        timer += dt;
        self.alpha(std::min(1.f, timer / duration));
    };
}

// C6. Мерцание эмиссии (горящий объект)
auto ScriptFlicker(float minI = 0.5f, float maxI = 1.5f) {
    return [=](GameObject& self, float dt) {
        float t = (float)glfwGetTime();
        float i = minI + (maxI-minI) * (std::sin(t*7.3f)*0.5f+0.5f)
                                     * (std::sin(t*13.f)*0.5f+0.5f);
        self.emissive(i);
    };
}

// C7. Смена цвета при условии (по расстоянию)
auto ScriptColorByDistance(App& app, EntityID targetId,
                            float nearDist = 2.f, float farDist = 8.f) {
    return [&app, targetId, nearDist, farDist](GameObject& self, float dt) {
        auto* t = app.world().get<TransformComponent>(targetId);
        if (!t) return;
        float d = glm::length(self.pos() - t->position);
        float f = std::clamp((d - nearDist) / (farDist - nearDist), 0.f, 1.f);
        self.color(1.f - f, f, 0.f); // red → green
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  D. АУДИО
// ─────────────────────────────────────────────────────────────────────────────

// D1. Звуковой триггер — играет при приближении
auto ScriptProximitySound(App& app, EntityID listenerId,
                           const std::string& soundPath,
                           float triggerDist = 5.f) {
    bool played = false;
    return [&app, listenerId, soundPath, triggerDist, played]
           (GameObject& self, float dt) mutable {
        auto* lt = app.world().get<TransformComponent>(listenerId);
        if (!lt) return;
        float dist = glm::length(self.pos() - lt->position);
        if (dist < triggerDist && !played) {
            Audio::play(soundPath);
            played = true;
        }
        if (dist >= triggerDist) played = false;
    };
}

// D2.   - 3D positional audio (обновляет позицию каждый кадр)
auto ScriptPositionalAudio(const std::string& path, float vol = 0.8f) {
    SoundHandle handle = INVALID_SOUND;
    return [=](GameObject& self, float dt) mutable {
        if (handle == INVALID_SOUND)
            handle = Audio::loop3D(path, self.pos(), vol, 15.f);
        else
            Audio::setPosition(handle, self.pos());
    };
}

// D3. Звук при скорости (двигатель)
auto ScriptEngineSound(App& app, const std::string& path, EntityID rbId) {
    SoundHandle handle = INVALID_SOUND;
    return [&app, path, rbId, handle](GameObject& self, float dt) mutable {
        if (handle == INVALID_SOUND) handle = Audio::loop(path, 0.f);
        auto* rb = app.world().get<RigidBodyComponent>(rbId);
        if (rb) {
            float speed = glm::length(rb->velocity);
            float vol   = std::min(1.f, speed / 10.f);
            float pitch = 0.8f + speed / 20.f;
            Audio::setVolume(handle, vol);
            Audio::setPitch (handle, pitch);
        }
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  E. ИГРОВАЯ ЛОГИКА
// ─────────────────────────────────────────────────────────────────────────────

// E1. Таймер — вызывает функцию через N секунд
auto ScriptTimer(float delay, std::function<void()> action) {
    float timer = 0.f;
    bool  fired = false;
    return [=](GameObject& self, float dt) mutable {
        if (fired) return;
        timer += dt;
        if (timer >= delay) { action(); fired = true; }
    };
}

// E2. Здоровье и уничтожение при 0
auto ScriptHealth(int maxHp = 100) {
    int hp = maxHp;
    return [=](GameObject& self, float dt) mutable {
        // Expose: call self.world()->get<HealthComp> to reduce hp externally
        // Destroy when dead
        if (hp <= 0) { self.destroy(); }
    };
}

// E3. Сбор предмета (коллектибл)
auto ScriptCollectible(App& app, EntityID playerId,
                        float pickupDist = 1.5f,
                        std::function<void()> onCollect = nullptr) {
    return [&app, playerId, pickupDist, onCollect]
           (GameObject& self, float dt) mutable {
        auto* pt = app.world().get<TransformComponent>(playerId);
        if (!pt) return;
        if (glm::length(self.pos() - pt->position) < pickupDist) {
            Audio::play("assets/sounds/coin.wav");
            if (onCollect) onCollect();
            self.destroy();
        }
    };
}

// E4. Патруль (туда-обратно между двумя точками)
auto ScriptPatrol(glm::vec3 a, glm::vec3 b, float speed = 2.f) {
    bool toB = true;
    return [=](GameObject& self, float dt) mutable {
        glm::vec3 target = toB ? b : a;
        glm::vec3 dir    = target - self.pos();
        float dist = glm::length(dir);
        if (dist < 0.3f) { toB = !toB; return; }
        self.move(glm::normalize(dir) * speed * dt);
        self.lookAt(target);
    };
}

// E5. Простой ИИ — преследование игрока
auto ScriptChasePlayer(App& app, EntityID playerId,
                        float speed = 3.f, float stopDist = 1.5f) {
    return [&app, playerId, speed, stopDist](GameObject& self, float dt) {
        auto* pt = app.world().get<TransformComponent>(playerId);
        if (!pt) return;
        glm::vec3 dir  = pt->position - self.pos();
        float     dist = glm::length(dir);
        if (dist <= stopDist) return;
        self.move(glm::normalize(dir) * speed * dt);
        self.lookAt(pt->position);
    };
}

// E6. Вращение к игроку (глаза)
auto ScriptFacePlayer(App& app, EntityID playerId) {
    return [&app, playerId](GameObject& self, float dt) {
        auto* pt = app.world().get<TransformComponent>(playerId);
        if (pt) self.lookAt(pt->position);
    };
}

// E7. Самоуничтожение через N секунд
auto ScriptLifetime(float seconds) {
    float timer = 0.f;
    return [=](GameObject& self, float dt) mutable {
        timer += dt;
        if (timer >= seconds) self.destroy();
    };
}

// E8. Спавнер — создаёт объекты периодически
auto ScriptSpawner(App& app, std::function<void(App&)> spawnFn, float interval = 2.f) {
    float timer = 0.f;
    return [&app, spawnFn, interval](GameObject& self, float dt) mutable {
        timer += dt;
        if (timer >= interval) { spawnFn(app); timer = 0.f; }
    };
}

// E9. Счётчик очков при подборе
auto ScriptScoreOnPickup(App& app, EntityID playerId, int* score, int value = 10,
                          float dist = 1.5f) {
    return [&app, playerId, score, value, dist](GameObject& self, float dt) {
        auto* pt = app.world().get<TransformComponent>(playerId);
        if (!pt) return;
        if (glm::length(self.pos() - pt->position) < dist) {
            *score += value;
            self.destroy();
        }
    };
}

// E10. Дверь — открывается при нажатии E рядом
auto ScriptDoor(App& app, EntityID playerId, float activeDist = 2.f) {
    bool open = false;
    float openY = 0.f;
    bool  init  = false;
    return [&app, playerId, activeDist, open, openY, init]
           (GameObject& self, float dt) mutable {
        if (!init) { openY = self.y() + 3.f; init = true; }
        auto* pt = app.world().get<TransformComponent>(playerId);
        bool near = pt && glm::length(self.pos() - pt->position) < activeDist;
        if (near && app.keyDown(GLFW_KEY_E)) open = !open;
        float targetY = open ? openY : (openY - 3.f);
        self.y(lerp(self.y(), targetY, dt * 4.f));
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  F. КАМЕРА
// ─────────────────────────────────────────────────────────────────────────────

// F1. Camera следует за объектом (third-person)
auto ScriptCameraFollow(App& app, float dist = 6.f, float height = 3.f,
                          float smoothing = 5.f) {
    return [&app, dist, height, smoothing](GameObject& self, float dt) {
        glm::vec3 target = self.pos()
            - self.forward() * dist
            + glm::vec3(0, height, 0);
        glm::vec3 cur = app.camPos();
        glm::vec3 next = lerp3(cur, target, smoothing * dt);
        app.camPos(next.x, next.y, next.z);
    };
}

// F2. Тряска камеры
auto ScriptCameraShake(App& app, float intensity = 0.3f, float duration = 0.5f) {
    float timer = 0.f;
    glm::vec3 basePos = {};
    bool init = false;
    return [&app, intensity, duration, timer, basePos, init]
           (GameObject& self, float dt) mutable {
        if (!init) { basePos = app.camPos(); init = true; }
        timer += dt;
        if (timer < duration) {
            float f = 1.f - timer / duration;
            float ox = ((float)rand()/RAND_MAX - 0.5f) * intensity * f * 2.f;
            float oy = ((float)rand()/RAND_MAX - 0.5f) * intensity * f * 2.f;
            app.camPos(basePos.x + ox, basePos.y + oy, basePos.z);
        } else {
            app.camPos(basePos.x, basePos.y, basePos.z);
        }
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  ПРИМЕР СЦЕНЫ — все скрипты вместе
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    App app("Jenuroix Engine — Scripts Demo", 1280, 720);
    app.skyColor(0.1f, 0.12f, 0.2f)
       .lightDir(0.4f, 1.f, 0.5f)
       .ambient(0.15f, 0.15f, 0.2f);
    app.camPos(0, 5, 14).camRot(-90.f, -18.f).freeCam(true);

    // Пол
    app.spawn("Floor").cube().scale(20, 0.2f, 20).color(0.15f, 0.15f, 0.2f)
       .pos(0, -0.1f, 0).physStatic();

    // ── A. Transform ──────────────────────────────────────────────────────
    app.spawn("A1_RotateY")
       .cube().color(1.f, 0.3f, 0.3f).pos(-6, 1, 4)
       .onUpdate(ScriptRotateY(90.f));

    app.spawn("A2_Spin")
       .cube().color(1.f, 0.6f, 0.2f).pos(-4, 1, 4)
       .onUpdate(ScriptSpin(20.f, 60.f, 15.f));

    app.spawn("A3_Hover")
       .sphere().color(0.3f, 0.8f, 1.f).pos(-2, 1, 4)
       .onUpdate(ScriptHover(0.6f, 1.2f));

    auto orbitCenter = glm::vec3(0, 1, 4);
    app.spawn("A4_Orbit")
       .sphere().color(1.f, 1.f, 0.2f).scale(0.4f)
       .onUpdate(ScriptOrbit(orbitCenter, 2.f, 0.8f));

    app.spawn("A5_Pendulum")
       .cube().color(0.6f, 0.3f, 1.f).scale(0.3f, 1.5f, 0.3f).pos(2, 2.5f, 4)
       .onUpdate(ScriptPendulum(35.f, 0.7f));

    std::vector<glm::vec3> path = {
        {5, 0.5f, 2}, {5, 0.5f, 6}, {7, 0.5f, 6}, {7, 0.5f, 2}};
    app.spawn("A6_Waypoints")
       .cube().color(0.2f, 0.9f, 0.5f).scale(0.5f)
       .onUpdate(ScriptWaypoints(path, 3.f));

    // ── B. Физика ─────────────────────────────────────────────────────────────
    app.spawn("B1_JumpBox")
       .cube().color(0.9f, 0.2f, 0.2f).pos(-6, 1, 0)
       .physDynamic(2.f).lockRotX().lockRotZ()
       .onUpdate(ScriptJump(app, 350.f));

    app.spawn("B2_Platform")
       .cube().scale(3, 0.3f, 2).color(0.3f, 0.5f, 0.9f).pos(0, 0.5f, 0)
       .physKinematic()
       .onUpdate(ScriptMovingPlatform({0,0.5f,-3}, {0,0.5f,3}, 1.5f));

    app.spawn("B3_Boundary")
       .sphere().color(1.f, 0.9f, 0.2f).scale(0.4f).pos(3, 1, 0)
       .physDynamic(1.f)
       .onUpdate(ScriptBoundary({3,1,0}, 3.f));

    app.spawn("B4_Wind_target")
       .sphere().color(0.5f, 1.f, 0.5f).scale(0.5f).pos(6, 1, 0)
       .physDynamic(0.5f)
       .onUpdate(ScriptWind(app, {1,0,0}, 30.f));

    // ── C. Visual ─────────────────────────────────────────────────────────────
    app.spawn("C1_Blink")
       .cube().color(1.f, 0.3f, 0.3f).pos(-6, 1, -4)
       .onUpdate(ScriptBlink(0.4f));

    app.spawn("C2_Pulse")
       .sphere().color(0.3f, 0.7f, 1.f).pos(-4, 1, -4)
       .onUpdate(ScriptPulse(0.6f, 1.3f, 1.5f));

    app.spawn("C3_Rainbow")
       .cube().pos(-2, 1, -4)
       .onUpdate(ScriptRainbow(0.5f));

    app.spawn("C4_FadeIn")
       .sphere().color(1.f, 1.f, 1.f).pos(0, 1, -4)
       .onUpdate(ScriptFadeIn(2.f));

    app.spawn("C5_Flicker")
       .cube().color(1.f, 0.5f, 0.1f).pos(2, 1, -4)
       .emissive(0.5f)
       .onUpdate(ScriptFlicker(0.3f, 2.f));

    // ── D. Audio ──────────────────────────────────────────────────────────────
    app.spawn("D1_ProximitySound")
       .sphere().color(1.f, 0.8f, 0.2f).scale(0.6f).pos(4, 1, -4)
       .onUpdate(ScriptTimer(0.5f, [](){
           LOG_INFO("Audio: proximity sound ready");
       }));

    // ── E. Game Logic ─────────────────────────────────────────────────────
    auto player = app.spawn("Player")
       .cube().scale(0.6f, 0.9f, 0.6f).color(0.9f, 0.3f, 0.25f).pos(0, 1, 8)
       .physDynamic(70.f).lockRotX().lockRotZ();

    int score = 0;

    // Coins
    for (int i = 0; i < 5; ++i) {
        auto coin = app.spawn("Coin_" + std::to_string(i));
        coin.sphere().scale(0.3f).color(1.f, 0.8f, 0.f)
            .pos(-4.f + i * 2.f, 0.8f, 6);
        int val = 10;
        coin.onUpdate(ScriptScoreOnPickup(app, player.id(), &score, val, 1.2f));
        coin.onUpdate(ScriptRotateY(120.f));
    }

    // Patrol enemy
    app.spawn("Enemy")
       .cube().scale(0.7f).color(0.7f, 0.1f, 0.1f).pos(-5, 0.5f, 6)
       .onUpdate(ScriptPatrol({-7, 0.5f, 6}, {-3, 0.5f, 6}, 2.f));

    // Lifetime bullet
    app.spawn("Bullet")
       .sphere().scale(0.2f).color(1.f, 1.f, 0.f).pos(0, 1, 5)
       .physDynamic(0.1f).velocity(0, 0, -8)
       .onUpdate(ScriptLifetime(3.f));

    // Door
    app.spawn("Door")
       .cube().scale(1, 2.5f, 0.2f).color(0.5f, 0.35f, 0.2f).pos(3, 1.25f, 6)
       .physKinematic()
       .onUpdate(ScriptDoor(app, player.id(), 2.5f));

    // ── F. Camera ─────────────────────────────────────────────────────────────
    // Third-person camera follow (uncomment to use)
    // player.onUpdate(ScriptCameraFollow(app, 6.f, 3.f, 4.f));

    // ── HUD ───────────────────────────────────────────────────────────────────
    app.onRender([&]() {
        if (UI::BeginFixed("##hud", 8, 8, 0, 0,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::LabelColored(UI::WHITE,  "FPS: %.0f", Time::fps());
            UI::LabelColored(UI::YELLOW, "Score: %d", score);
            UI::LabelColored(UI::GRAY,   "Tab=мышь  WASD=camera");
            UI::LabelColored(UI::CYAN,   "E = дверь  Space = прыжок");
            UI::Separator();
            UI::LabelColored(UI::GRAY, "A: вращение, орбита, путь");
            UI::LabelColored(UI::GRAY, "B: физика, платформы, ветер");
            UI::LabelColored(UI::GRAY, "C: мигание, пульс, радуга");
            UI::LabelColored(UI::GRAY, "D: аудио-триггеры");
            UI::LabelColored(UI::GRAY, "E: монеты, враг, дверь, пуля");
        }
        UI::End();
    });

    return app.run();
}
