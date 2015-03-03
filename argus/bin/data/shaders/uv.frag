#version 110
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect map;
uniform vec2 mapSize;

uniform sampler2DRect frame;
uniform vec2 frameSize;

uniform vec2 shaderSize;

void main(){
	vec2 mapCoord = mapSize * gl_TexCoord[0].st / shaderSize;
	vec4 mapPixel = texture2DRect( map, mapCoord );
	vec2 uv = frameSize * mapPixel.rg;
	
	gl_FragColor = (uv != vec2(0.,0.))?texture2DRect( frame, uv ) : vec4(0.,0.,0.,0.);
	
	gl_FragColor.a = mapPixel.a;
}