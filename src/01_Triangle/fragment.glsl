#version 440

out vec4 color;

void main() {
    float lerpValue = gl_FragCoord.y / 400.0f;
    
    color = mix(vec4(0.2f, 0.8f, 0.4f, 1.0f), vec4(0.2f, 0.2f, 0.2f, 1.0f), lerpValue);
}