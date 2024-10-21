#version 300 es
precision highp float;

out vec4 fFragColor;

uniform float iTime;
uniform ivec2 iResolution;

/**
 * SDF circle.
 * pos :: position
 * r   :: radius
 * e   :: edge size
 */
float circle(in vec2 pos, in float r, in float e) {
    float sdf = (length(pos) - r);
    float inv = e - sdf;
    return smoothstep(0., e, inv);
}

/**
 * SDF rings.
 * pos :: position
 * r   :: radius
 * e   :: edge size
 * f   :: ring frequency
 */
float rings(in vec2 pos, in vec2 uv0, in float e, in float f) {
    float sdf = length(pos) * exp(-length(uv0));
    sdf = sin(sdf * f + iTime*2.);
    sdf = abs(sdf);
    float inv = e - sdf;
    return smoothstep(0., e, inv);
}

vec3 palette(float t) {
    const vec3 a = vec3(0.5, 0.5, 0.5);
    const vec3 b = vec3(0.5, 0.5, 0.5);
    const vec3 c = vec3(1.0, 1.0, 1.0);
    const vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

void main() {
    vec2 uv = (gl_FragCoord.xy * 2.0 - iResolution) / iResolution.y;
    vec2 uv0 = uv;
    vec3 finalColor = vec3(0.0);

    for (int i = 0; i < 4; i++) {
        
        uv = fract(uv * 1.5) - 0.5;

        vec3 c = palette(length(uv0) + float(i) * 0.4 + iTime * 0.33);
        float s = 0.111;

        // s += circle(uv, 0.03, 0.2);
        s += rings(uv, uv0, 0.33, 20.0);
        c *= s;
        finalColor += c;
        
    }
    
    fragColor = vec4(finalColor, 1.0);
}