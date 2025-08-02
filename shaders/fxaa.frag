#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D u_Scene;
uniform vec2       u_InvScreen;   

#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_REDUCE_MUL   (1.0/24.0)
#define FXAA_SPAN_MAX     2.0

void main()
{
    vec3 rgbM  = texture(u_Scene, vUV).rgb;
    vec3 rgbN  = texture(u_Scene, vUV + vec2(0.0, u_InvScreen.y)).rgb;
    vec3 rgbS  = texture(u_Scene, vUV + vec2(0.0, -u_InvScreen.y)).rgb;
    vec3 rgbE  = texture(u_Scene, vUV + vec2(u_InvScreen.x, 0.0)).rgb;
    vec3 rgbW  = texture(u_Scene, vUV + vec2(-u_InvScreen.x,0.0)).rgb;

    // luma
    float lM = dot(rgbM, vec3(0.299, 0.587, 0.114));
    float lN = dot(rgbN, vec3(0.299, 0.587, 0.114));
    float lS = dot(rgbS, vec3(0.299, 0.587, 0.114));
    float lE = dot(rgbE, vec3(0.299, 0.587, 0.114));
    float lW = dot(rgbW, vec3(0.299, 0.587, 0.114));

    float lMin = min(lM, min(min(lN,lS), min(lE,lW)));
    float lMax = max(lM, max(max(lN,lS), max(lE,lW)));

    vec2 dir;
    dir.x = -((lN + lS) - 2.0*lM);
    dir.y =  ((lE + lW) - 2.0*lM);

    float dirReduce = max((lN + lS + lE + lW) * (0.25 * FXAA_REDUCE_MUL),
                          FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0/(min(abs(dir.x),abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin * FXAA_SPAN_MAX * u_InvScreen,
                -FXAA_SPAN_MAX*u_InvScreen,
                 FXAA_SPAN_MAX*u_InvScreen);

    vec3 rgbA = 0.5 * (
        texture(u_Scene, vUV + dir * (1.0/3.0 - 0.5)).rgb +
        texture(u_Scene, vUV + dir * (2.0/3.0 - 0.5)).rgb
    );
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(u_Scene, vUV + dir * -0.5).rgb +
        texture(u_Scene, vUV + dir * 0.5).rgb
    );

    float lB = dot(rgbB, vec3(0.299,0.587,0.114));
    FragColor = (lB < lMin || lB > lMax) ? vec4(rgbA,1.0)
                                         : vec4(rgbB,1.0);
}
