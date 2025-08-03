#version 450 core
in vec2 texCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;

const float BLOCK_SIZE = 6.0;

void main() {

    vec3 color =  texture(screenTexture, texCoords).rgb;

    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));
    FragColor = vec4(color, 1.0);

    //ivec2 texSize = textureSize(screenTexture, 0);
    //vec2 resolution = vec2(texSize);

    //vec2 uv_px = texCoords * resolution;

    //vec2 block_uv = floor(uv_px / BLOCK_SIZE) * BLOCK_SIZE;

    //vec2 uv_snapped = block_uv / resolution;

   //FragColor = texture(screenTexture, uv_snapped);
}
