#version 450 core
layout(location=0) out float FragAO;
in vec2 TexCoords;

uniform sampler2D ssaoInput;

void main() {
    vec2 texel = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel;
            result += texture(ssaoInput, TexCoords + offset).r;
        }
    }
    FragAO = result / (4.0 * 4.0);
}
