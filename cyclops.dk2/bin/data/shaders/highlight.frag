#version 110
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect frame;
uniform vec2 frameSize;
uniform vec2 dir;
uniform float fov;
uniform vec3 highlight;
uniform float linewidth;

const float pi = 3.1415926536;

vec3 polarToCart(float phi, float theta) {
    return vec3(
        sin(theta) * sin(phi),
        cos(theta),
        sin(theta) * cos(phi)
    );
}

void main(){
    gl_FragColor = texture2DRect( frame, gl_TexCoord[0].st );
    
    vec2 P = gl_TexCoord[0].st / frameSize;
    float phi   = (P.x) * pi * 2.0;
    float theta = (1.0 - P.y) * pi;
    
    vec3 cartPixel = polarToCart(phi, theta);
    vec3 cartTarget = polarToCart(dir.x, dir.y - pi/2.);
    
    float angle = acos(dot(cartPixel, cartTarget));
    
    if(abs(angle-fov/2.)<linewidth) {
        gl_FragColor.rgb = highlight;   
    }
}