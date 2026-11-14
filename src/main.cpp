#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include "telemetry_parser.h"
#include "sorting_engine.h"

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
GLuint shaderProgram;
GLuint vaoPoints, vboPoints;
GLuint vaoAxes, vboAxes;
int numPoints = 0;
int numAxisVertices = 0;
float rotX = 0.35f;
float rotY = -0.55f;
float zoomLevel = 0.78f;
std::vector<FireData> globalDataset;

// Unified Shader with Smaller Precision Dots (3.5px - 4.8px)
const char* vertexShaderSource = R"(#version 300 es
    layout(location = 0) in vec4 aPosColor;
    uniform float uRotX;
    uniform float uRotY;
    uniform float uZoom;
    out float vType;
    void main() {
        vec3 p = (aPosColor.xyz / 512.0) - 1.0;

        float cy = cos(uRotY), sy = sin(uRotY);
        vec3 ry = vec3(p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy);

        float cx = cos(uRotX), sx = sin(uRotX);
        vec3 rx = vec3(ry.x, ry.y * cx - ry.z * sx, ry.y * sx + ry.z * cx);

        gl_Position = vec4(rx.xy * uZoom, rx.z * 0.5, 1.0);
        // Smaller, high-precision scientific points
        float baseSize = (aPosColor.w > 0.5) ? 4.8 : 3.5;
        gl_PointSize = clamp(baseSize * (uZoom / 0.78), 2.5, 9.0);
        vType = aPosColor.w;
    }
)";

const char* fragmentShaderSource = R"(#version 300 es
    precision mediump float;
    in float vType;
    out vec4 FragColor;
    void main() {
        if (vType >= 0.0) {
            vec2 coord = gl_PointCoord - vec2(0.5);
            if (length(coord) > 0.5) discard;
            if (vType > 0.5) {
                FragColor = vec4(1.0, 0.35, 0.1, 1.0); // Orange/Red Hazard
            } else {
                FragColor = vec4(0.15, 0.85, 1.0, 1.0); // Cyan Nominal
            }
        } else if (vType > -1.5) {
            FragColor = vec4(1.0, 0.35, 0.35, 0.95); // X-Axis (Red: O2 %)
        } else if (vType > -2.5) {
            FragColor = vec4(0.3, 0.95, 0.45, 0.95); // Y-Axis (Green: Pressure kPa)
        } else if (vType > -3.5) {
            FragColor = vec4(0.35, 0.65, 1.0, 0.95); // Z-Axis (Blue: Spread mm/s)
        } else {
            FragColor = vec4(0.30, 0.40, 0.55, 0.45); // Wireframe Cube & Ticks
        }
    }
)";

void build_3d_axes() {
    std::vector<float> lines;
    auto addLine = [&](float x1, float y1, float z1, float x2, float y2, float z2, float type) {
        lines.push_back(x1); lines.push_back(y1); lines.push_back(z1); lines.push_back(type);
        lines.push_back(x2); lines.push_back(y2); lines.push_back(z2); lines.push_back(type);
    };

    const float L = 160.0f;
    const float H = 864.0f;

    addLine(L, L, L, H, L, L, -1.0f);
    addLine(L, L, L, L, H, L, -2.0f);
    addLine(L, L, L, L, L, H, -3.0f);

    addLine(H, L, L, H, H, L, -4.0f);
    addLine(L, H, L, H, H, L, -4.0f);
    addLine(L, L, H, H, L, H, -4.0f);
    addLine(L, L, H, L, H, H, -4.0f);
    addLine(H, L, H, H, H, H, -4.0f);
    addLine(L, H, H, H, H, H, -4.0f);
    addLine(H, L, L, H, L, H, -4.0f);
    addLine(L, H, L, L, H, H, -4.0f);
    addLine(H, H, L, H, H, H, -4.0f);

    for (int i = 1; i <= 3; ++i) {
        float t = L + (H - L) * (i * 0.25f);
        addLine(t, L, L, t, L, H, -4.0f);
        addLine(L, L, t, H, L, t, -4.0f);
        addLine(L - 12.0f, t, L, L + 12.0f, t, L, -2.0f);
    }

    numAxisVertices = static_cast<int>(lines.size() / 4);
    glGenVertexArrays(1, &vaoAxes);
    glGenBuffers(1, &vboAxes);
    glBindVertexArray(vaoAxes);
    glBindBuffer(GL_ARRAY_BUFFER, vboAxes);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STATIC_DRAW);
}

void compile_shaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGenVertexArrays(1, &vaoPoints);
    glGenBuffers(1, &vboPoints);
    build_3d_axes();
}

void render_loop() {
    int width, height;
    emscripten_get_canvas_element_size("#canvas", &width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0.04f, 0.07f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glUniform1f(glGetUniformLocation(shaderProgram, "uRotX"), rotX);
    glUniform1f(glGetUniformLocation(shaderProgram, "uRotY"), rotY);
    glUniform1f(glGetUniformLocation(shaderProgram, "uZoom"), zoomLevel);

    if (numAxisVertices > 0) {
        glBindVertexArray(vaoAxes);
        glBindBuffer(GL_ARRAY_BUFFER, vboAxes);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glDrawArrays(GL_LINES, 0, numAxisVertices);
    }

    if (numPoints > 0) {
        glBindVertexArray(vaoPoints);
        glBindBuffer(GL_ARRAY_BUFFER, vboPoints);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glDrawArrays(GL_POINTS, 0, numPoints);
    }
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void set_camera_rotation(float rx, float ry) {
        rotX = rx;
        rotY = ry;
    }

    EMSCRIPTEN_KEEPALIVE
    void set_camera_zoom(float z) {
        zoomLevel = z;
    }

    EMSCRIPTEN_KEEPALIVE
    void query_kdtree_nearest(double target_o2, double target_pr) {
        FireData nearest = findNearestNeighborKDTree(globalDataset, target_o2, target_pr);
        char js_cmd[256];
        std::snprintf(js_cmd, sizeof(js_cmd),
            "if(window.onKDTreeResult) window.onKDTreeResult(%.2f, %.2f, %.3f, %u);",
            nearest.oxygen_concentration, nearest.atmospheric_pressure,
            nearest.flame_spread_rate, nearest.spatial_index);
        emscripten_run_script(js_cmd);
    }

    EMSCRIPTEN_KEEPALIVE
    void process_uploaded_csv(const char* csv_content) {
        std::string raw_data(csv_content);
        globalDataset = parseTelemetryData(raw_data);

        AlgorithmMetrics metrics = runFullAlgorithmSuite(globalDataset);

        numPoints = static_cast<int>(globalDataset.size());
        std::vector<float> vertices;
        double totalO2 = 0;
        double maxSpread = 0;
        int hazardCount = 0;

        double minO2 = numPoints > 0 ? globalDataset[0].oxygen_concentration : 0.0;
        double maxO2 = minO2;
        double minPr = numPoints > 0 ? globalDataset[0].atmospheric_pressure : 0.0;
        double maxPr = minPr;
        double minSp = numPoints > 0 ? globalDataset[0].flame_spread_rate : 0.0;
        double maxSp = minSp;

        emscripten_run_script("window.loaded3DPoints = [];");

        for (const auto& data : globalDataset) {
            vertices.push_back((float)data.x);
            vertices.push_back((float)data.y);
            vertices.push_back((float)data.z);
            float isHazard = (data.oxygen_concentration >= 30.0) ? 1.0f : 0.0f;
            vertices.push_back(isHazard);

            totalO2 += data.oxygen_concentration;
            if (data.flame_spread_rate > maxSpread) maxSpread = data.flame_spread_rate;
            if (data.oxygen_concentration >= 30.0) hazardCount++;

            if (data.oxygen_concentration < minO2) minO2 = data.oxygen_concentration;
            if (data.oxygen_concentration > maxO2) maxO2 = data.oxygen_concentration;
            if (data.atmospheric_pressure < minPr) minPr = data.atmospheric_pressure;
            if (data.atmospheric_pressure > maxPr) maxPr = data.atmospheric_pressure;
            if (data.flame_spread_rate < minSp) minSp = data.flame_spread_rate;
            if (data.flame_spread_rate > maxSp) maxSp = data.flame_spread_rate;

            char pt_cmd[256];
            std::snprintf(pt_cmd, sizeof(pt_cmd),
                "window.loaded3DPoints.push({x:%u,y:%u,z:%u,o2:%.2f,pr:%.2f,sp:%.3f,morton:%u});",
                data.x, data.y, data.z,
                data.oxygen_concentration, data.atmospheric_pressure, data.flame_spread_rate, data.spatial_index);
            emscripten_run_script(pt_cmd);
        }

        glBindVertexArray(vaoPoints);
        glBindBuffer(GL_ARRAY_BUFFER, vboPoints);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        double avgO2 = numPoints > 0 ? (totalO2 / numPoints) : 0;

        char js_cmd[640];
        std::snprintf(js_cmd, sizeof(js_cmd),
            "if(window.updateAxisRanges) window.updateAxisRanges(%.1f, %.1f, %.1f, %.1f, %.2f, %.2f);"
            "if(window.generateSIReport) window.generateSIReport(%d, %.2f, %.3f, %d, %.4f, %.4f, %.4f);",
            minO2, maxO2, minPr, maxPr, minSp, maxSp,
            numPoints, avgO2, maxSpread, hazardCount,
            metrics.radix_ms, metrics.timsort_ms, metrics.mergesort_ms);
        emscripten_run_script(js_cmd);
    }
}

int main() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;

    glContext = emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(glContext);

    emscripten_set_canvas_element_size("#canvas", 720, 520);
    compile_shaders();

    emscripten_set_main_loop(render_loop, 35, 1);
    return 0;
}