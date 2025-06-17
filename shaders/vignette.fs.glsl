#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;
uniform float vignetteIntensity;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec4 vignette(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE VIGNETTE EFFECT HERE
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	
	if(vignetteIntensity <= 0.0){
		return in_color;
	}


	vec2 screen_center = vec2(0.5, 0.5);
    float dist = distance(texcoord, screen_center);

	float vignetteMask = smoothstep(0, 0.9, dist);

	vec4 redTint = vec4(1.0, 0.0, 0.0, 1.0);
	vec4 finalColor = mix(in_color, redTint, vignetteMask * 0.5);

	return finalColor;
}

// darken the screen, i.e., fade to black
vec4 fade_color(vec4 in_color) 
{
	if (darken_screen_factor > 0)
		return mix(in_color, vec4(0.0, 0.0, 0.0, 1.0), darken_screen_factor);
	return in_color;
}

void main()
{
   vec4 in_color = texture(screen_texture, texcoord);
	in_color = vignette(in_color);
    color = fade_color(in_color);
}