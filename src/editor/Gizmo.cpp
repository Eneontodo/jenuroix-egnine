// Gizmo.cpp — 3D transform handles rendered with OpenGL lines/quads
// Стрелки: X=красный, Y=зелёный, Z=синий (как в Blender)

#include "editor/Gizmo.h"
#include "engine.h"
#include "core/Log.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <cmath>
#include <vector>
#include <array>

// ─────────────────────────────────────────────────────────────────────────────
//  Простой шейдер для рисования цветных линий без освещения
// ─────────────────────────────────────────────────────────────────────────────
static const char* GIZMO_VERT = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos,1.0); }
)";

static const char* GIZMO_FRAG = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main(){ FragColor = uColor; }
)";

// ─────────────────────────────────────────────────────────────────────────────
//  State
// ─────────────────────────────────────────────────────────────────────────────
namespace {

// Shader program
GLuint g_prog  = 0;
GLuint g_vao   = 0;
GLuint g_vbo   = 0;
bool   g_ready = false;

// Drag state
bool       g_dragging   = false;
int        g_dragAxis   = -1;   // 0=X 1=Y 2=Z
GizmoMode  g_dragMode   = GizmoMode::Translate;
glm::vec3  g_dragStart  = {};   // world pos at drag begin
glm::vec3  g_origPos    = {};
glm::vec3  g_origRot    = {};
glm::vec3  g_origScale  = {};
glm::vec2  g_mouseStart = {};
float      g_dragAccum  = 0.f;

// Highlight
int g_hoverAxis = -1;

// Colors
const glm::vec4 COL_X      = {1.f,  0.2f, 0.2f, 1.f};
const glm::vec4 COL_Y      = {0.2f, 1.f,  0.2f, 1.f};
const glm::vec4 COL_Z      = {0.3f, 0.5f, 1.f,  1.f};
const glm::vec4 COL_HOVER  = {1.f,  0.9f, 0.1f, 1.f};
const glm::vec4 COL_SELECT = {1.f,  0.8f, 0.0f, 1.f};

// Gizmo visual scale (constant screen-space size)
constexpr float GIZMO_LEN  = 1.2f;
constexpr float ARROW_HEAD = 0.18f;
constexpr float RING_SEGS  = 48;

// ── Compile shader ────────────────────────────────────────────────────────────
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

static void initGL() {
    if (g_ready) return;

    GLuint vs = compileShader(GL_VERTEX_SHADER,   GIZMO_VERT);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, GIZMO_FRAG);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glBindVertexArray(0);

    g_ready = true;
}

// ── Screen-space size factor so gizmo stays same visual size ─────────────────
static float screenScale(const glm::vec3& worldPos,
                          const glm::mat4& proj,
                          const glm::mat4& view,
                          float screenH)
{
    glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.f);
    float w = std::abs(clip.w);
    return (w / screenH) * 1.8f;  // tuned constant
}

// ── Draw a batch of lines ─────────────────────────────────────────────────────
static void drawLines(const std::vector<glm::vec3>& pts,
                       const glm::vec4& col,
                       const glm::mat4& mvp,
                       float lineW = 2.f)
{
    if (pts.empty()) return;
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

    // Resize if needed
    static size_t g_vboSize = 256;
    if (pts.size() > g_vboSize) {
        g_vboSize = pts.size() * 2;
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(g_vboSize * sizeof(glm::vec3)),
                     nullptr, GL_DYNAMIC_DRAW);
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    (GLsizeiptr)(pts.size() * sizeof(glm::vec3)), pts.data());

    glUseProgram(g_prog);
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uMVP"), 1, GL_FALSE,
                       glm::value_ptr(mvp));
    glUniform4fv(glGetUniformLocation(g_prog, "uColor"), 1,
                 glm::value_ptr(col));

    glLineWidth(lineW);
    glBindVertexArray(g_vao);
    glDrawArrays(GL_LINES, 0, (GLsizei)pts.size());
    glBindVertexArray(0);
}

// ── Arrow (line + cone tip as triangle fan) ───────────────────────────────────
static void drawArrow(const glm::vec3& origin,
                       const glm::vec3& dir,      // normalized axis
                       float           len,
                       float           headLen,
                       const glm::vec4& col,
                       const glm::mat4& vp,
                       float            scale)
{
    glm::vec3 tip  = origin + dir * len * scale;
    glm::vec3 base = origin + dir * (len - headLen) * scale;
    float headR    = headLen * 0.35f * scale;

    // Shaft
    drawLines({origin, base}, col, vp, 2.5f);

    // Cone: build a perp basis
    glm::vec3 perp1 = glm::normalize(
        std::abs(dir.x) < 0.9f ? glm::cross(dir, glm::vec3(1,0,0))
                                : glm::cross(dir, glm::vec3(0,1,0)));
    glm::vec3 perp2 = glm::cross(dir, perp1);

    // 8-segment cone ring
    const int N = 8;
    std::vector<glm::vec3> cone;
    cone.reserve(N * 2 + 2);
    for (int i = 0; i < N; ++i) {
        float a0 = (float)i     / N * 6.2832f;
        float a1 = (float)(i+1) / N * 6.2832f;
        glm::vec3 r0 = base + (perp1 * std::cos(a0) + perp2 * std::sin(a0)) * headR;
        glm::vec3 r1 = base + (perp1 * std::cos(a1) + perp2 * std::sin(a1)) * headR;
        // Side edge
        cone.push_back(r0);   cone.push_back(tip);
        // Ring
        cone.push_back(r0);   cone.push_back(r1);
    }
    drawLines(cone, col, vp, 1.5f);

    // Cube handle at tip for scale mode — omitted here, handled separately
}

// ── Rotation ring ─────────────────────────────────────────────────────────────
static void drawRing(const glm::vec3& center,
                      const glm::vec3& normal,  // ring plane normal
                      float            radius,
                      const glm::vec4& col,
                      const glm::mat4& vp,
                      float            scale)
{
    glm::vec3 p1 = glm::normalize(
        std::abs(normal.x) < 0.9f ? glm::cross(normal, glm::vec3(1,0,0))
                                   : glm::cross(normal, glm::vec3(0,1,0)));
    glm::vec3 p2 = glm::cross(normal, p1);
    float r = radius * scale;

    std::vector<glm::vec3> pts;
    pts.reserve((int)RING_SEGS * 2);
    for (int i = 0; i < (int)RING_SEGS; ++i) {
        float a0 = (float)i     / RING_SEGS * 6.2832f;
        float a1 = (float)(i+1) / RING_SEGS * 6.2832f;
        pts.push_back(center + (p1 * std::cos(a0) + p2 * std::sin(a0)) * r);
        pts.push_back(center + (p1 * std::cos(a1) + p2 * std::sin(a1)) * r);
    }
    drawLines(pts, col, vp, 2.5f);
}

// ── Scale cube ────────────────────────────────────────────────────────────────
static void drawScaleCube(const glm::vec3& origin,
                           const glm::vec3& dir,
                           float            len,
                           float            cubeSize,
                           const glm::vec4& col,
                           const glm::mat4& vp,
                           float            scale)
{
    drawLines({origin, origin + dir * len * scale}, col, vp, 2.5f);

    // Cube wireframe at end
    glm::vec3 c   = origin + dir * len * scale;
    float     s   = cubeSize * scale;
    glm::vec3 perp1 = glm::normalize(
        std::abs(dir.x) < 0.9f ? glm::cross(dir, glm::vec3(1,0,0))
                                : glm::cross(dir, glm::vec3(0,1,0)));
    glm::vec3 perp2 = glm::cross(dir, perp1);

    // 8 corners
    auto cn = [&](int sx, int sy, int sz) {
        return c + dir*((float)sz*s) + perp1*((float)sx*s) + perp2*((float)sy*s);
    };
    std::vector<glm::vec3> cube;
    // 4 bottom, 4 top
    for (int a = -1; a <= 1; a += 2)
    for (int b = -1; b <= 1; b += 2) {
        cube.push_back(cn(a, b, -1)); cube.push_back(cn(a, b,  1));
        cube.push_back(cn(a, b, -1)); cube.push_back(cn(-a, b, -1));
        cube.push_back(cn(a, b,  1)); cube.push_back(cn(-a, b,  1));
    }
    drawLines(cube, col, vp, 1.5f);
}

// ── Hit test: does mouse ray intersect axis handle? ───────────────────────────
// Returns distance along axis if hit, -1 otherwise
static float hitTestAxis(const glm::vec3& rayOrig,
                          const glm::vec3& rayDir,
                          const glm::vec3& gizmoPos,
                          const glm::vec3& axis,
                          float            len,
                          float            threshold)
{
    // Closest point between ray and axis segment
    glm::vec3 d1 = rayDir;
    glm::vec3 d2 = axis;
    glm::vec3 r  = rayOrig - gizmoPos;

    float a = glm::dot(d1, d1);
    float e = glm::dot(d2, d2);
    float f = glm::dot(d2, r);
    float b = glm::dot(d1, d2);
    float c = glm::dot(d1, r);
    float det = a * e - b * b;

    if (std::abs(det) < 1e-6f) return -1.f;

    float s = glm::clamp((b*f - c*e) / det, 0.f, 1.f);
    float t = glm::clamp((a*f - b*c) / det, 0.f, len);

    glm::vec3 p1 = rayOrig  + rayDir * s;
    glm::vec3 p2 = gizmoPos + axis   * t;

    return glm::length(p1 - p2) < threshold ? t : -1.f;
}

// ── Ray from screen mouse position ───────────────────────────────────────────
static glm::vec3 screenRay(glm::vec2 mousePos,
                             int w, int h,
                             const glm::mat4& proj,
                             const glm::mat4& view)
{
    float nx = (2.f * mousePos.x) / w - 1.f;
    float ny = 1.f - (2.f * mousePos.y) / h;
    glm::vec4 clip(nx, ny, -1.f, 1.f);
    glm::vec4 eye  = glm::inverse(proj) * clip;
    eye.z = -1.f; eye.w = 0.f;
    return glm::normalize(glm::vec3(glm::inverse(view) * eye));
}

// ── Project world point onto screen ──────────────────────────────────────────
static glm::vec2 worldToScreen(const glm::vec3& p,
                                 const glm::mat4& vp,
                                 int w, int h)
{
    glm::vec4 clip = vp * glm::vec4(p, 1.f);
    clip /= clip.w;
    return { (clip.x * 0.5f + 0.5f) * w,
             (1.f - (clip.y * 0.5f + 0.5f)) * h };
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
//  Gizmo::draw
// ─────────────────────────────────────────────────────────────────────────────
bool Gizmo::draw(eng::App& app, EntityID sel, GizmoMode mode, bool /*localSpace*/)
{
    if (sel == 0) return false;

    auto* t = app.world().get<TransformComponent>(sel);
    if (!t) return false;

    initGL();

    // Matrices
    glm::mat4 view = app.camera().viewMatrix();
    glm::mat4 proj = app.camera().projectionMatrix();
    glm::mat4 vp   = proj * view;

    int W = app.window().width();
    int H = app.window().height();

    // Constant screen-space scale
    glm::vec3 pos   = t->position;
    float     scale = screenScale(pos, proj, view, (float)H) * GIZMO_LEN;

    // Axes in world space (local or world)
    glm::vec3 axX = glm::vec3(1,0,0);
    glm::vec3 axY = glm::vec3(0,1,0);
    glm::vec3 axZ = glm::vec3(0,0,1);

    // Mouse state
    glm::vec2 mouse    = app.mousePos();
    bool      lmbDown  = app.mouse(0);
    static bool s_prevLMB = false;
    bool      lmbClick = lmbDown && !s_prevLMB;
    bool      lmbUp    = !lmbDown && s_prevLMB;
    s_prevLMB = lmbDown;

    // Don't interact if ImGui captures mouse
    ImGuiIO& io = ImGui::GetIO();
    bool mouseBlocked = io.WantCaptureMouse;

    // Ray from camera
    glm::vec3 rayOrig = app.camera().position();
    glm::vec3 rayDir  = screenRay(mouse, W, H, proj, view);

    float hitThresh = scale * 0.12f;
    float axisLen   = scale;

    // ── Hover detection ───────────────────────────────────────────────────────
    if (!g_dragging && !mouseBlocked) {
        g_hoverAxis = -1;
        float bestDist = hitThresh;

        for (int ax = 0; ax < 3; ++ax) {
            glm::vec3 axis = (ax==0 ? axX : ax==1 ? axY : axZ);
            float d = hitTestAxis(rayOrig, rayDir, pos, axis, axisLen, hitThresh);
            if (d >= 0.f && d < bestDist) {
                bestDist   = d;
                g_hoverAxis = ax;
            }
        }
    }

    // ── Drag start ────────────────────────────────────────────────────────────
    if (lmbClick && g_hoverAxis >= 0 && !mouseBlocked) {
        g_dragging   = true;
        g_dragAxis   = g_hoverAxis;
        g_dragMode   = mode;
        g_origPos    = t->position;
        g_origRot    = t->rotation;
        g_origScale  = t->scale;
        g_mouseStart = mouse;
        g_dragAccum  = 0.f;
        LOG_INFO("Gizmo: drag start axis=" << g_dragAxis);
    }

    // ── Drag update ───────────────────────────────────────────────────────────
    if (g_dragging && lmbDown) {
        glm::vec3 axis = (g_dragAxis==0 ? axX : g_dragAxis==1 ? axY : axZ);

        // Project mouse delta onto screen projection of axis
        glm::vec2 ax2d_start = worldToScreen(pos,          vp, W, H);
        glm::vec2 ax2d_end   = worldToScreen(pos + axis,   vp, W, H);
        glm::vec2 axScreen   = ax2d_end - ax2d_start;

        float axLen2 = glm::length2(axScreen);
        if (axLen2 > 1.f) {
            glm::vec2 delta   = mouse - g_mouseStart;
            float     proj1d  = glm::dot(delta, axScreen) / axLen2
                                * glm::length(axScreen);

            // Sensitivity
            float sensitivity = 0.01f;

            switch (g_dragMode) {
            case GizmoMode::Translate:
                t->position = g_origPos + axis * proj1d * sensitivity * 2.f;
                // Sync physics
                if (app.world().has<RigidBodyComponent>(sel))
                    app.physics().setPosition(sel, t->position);
                break;

            case GizmoMode::Rotate: {
                float deg = proj1d * sensitivity * 60.f;
                glm::vec3 rot = g_origRot;
                rot[g_dragAxis] += deg;
                t->rotation = rot;
                break;
            }

            case GizmoMode::Scale: {
                float factor = 1.f + proj1d * sensitivity * 0.5f;
                factor = std::max(0.001f, factor);
                glm::vec3 sc = g_origScale;
                sc[g_dragAxis] *= factor;
                t->scale = sc;
                break;
            }
            }
        }
    }

    // ── Drag end ──────────────────────────────────────────────────────────────
    if (g_dragging && lmbUp) {
        g_dragging  = false;
        g_dragAxis  = -1;
        LOG_INFO("Gizmo: drag end");
    }

    // ── Render (depth test off so gizmo is always visible) ────────────────────
    GLboolean depthWasEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    auto axisColor = [&](int ax) -> glm::vec4 {
        if (g_dragging  && g_dragAxis  == ax) return COL_SELECT;
        if (!g_dragging && g_hoverAxis == ax) return COL_HOVER;
        return ax==0 ? COL_X : ax==1 ? COL_Y : COL_Z;
    };

    for (int ax = 0; ax < 3; ++ax) {
        glm::vec3 axis = (ax==0 ? axX : ax==1 ? axY : axZ);
        glm::vec4 col  = axisColor(ax);

        switch (mode) {
        case GizmoMode::Translate:
            drawArrow(pos, axis, GIZMO_LEN, ARROW_HEAD, col, vp, scale / GIZMO_LEN);
            break;
        case GizmoMode::Rotate:
            drawRing(pos, axis, GIZMO_LEN * 0.8f, col, vp, scale / GIZMO_LEN);
            break;
        case GizmoMode::Scale:
            drawScaleCube(pos, axis, GIZMO_LEN, 0.12f, col, vp, scale / GIZMO_LEN);
            break;
        }
    }

    // Center dot
    {
        std::vector<glm::vec3> dot;
        float s2 = scale * 0.06f;
        dot.push_back(pos + glm::vec3(-s2, 0, 0));
        dot.push_back(pos + glm::vec3( s2, 0, 0));
        dot.push_back(pos + glm::vec3(0, -s2, 0));
        dot.push_back(pos + glm::vec3(0,  s2, 0));
        drawLines(dot, {1,1,1,1}, vp, 2.f);
    }

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
    glLineWidth(1.f);

    return g_dragging;
}

bool Gizmo::isDragging() { return g_dragging; }

void Gizmo::reset() {
    g_dragging  = false;
    g_dragAxis  = -1;
    g_hoverAxis = -1;
}
