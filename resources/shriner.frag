#version 120

uniform float threshold;
uniform float decayrate;
uniform sampler2D basetex;
uniform sampler2D lighttex;
uniform float saturation;
uniform vec4 fakeColor;


vec3 RGBToHSL(vec3 color)
{
	vec3 hsl; // init to 0 to avoid warnings ? (and reverse if + remove first part)
	
	float fmin = min(min(color.r, color.g), color.b);    //Min. value of RGB
	float fmax = max(max(color.r, color.g), color.b);    //Max. value of RGB
	float delta = fmax - fmin;             //Delta RGB value

	hsl.z = (fmax + fmin) / 2.0; // Luminance

	if (delta == 0.0)		//This is a gray, no chroma...
	{
		hsl.x = 0.0;	// Hue
		hsl.y = 0.0;	// Saturation
	}
	else                                    //Chromatic data...
	{
		if (hsl.z < 0.5)
			hsl.y = delta / (fmax + fmin); // Saturation
		else
			hsl.y = delta / (2.0 - fmax - fmin); // Saturation
		
		float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;
		float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;
		float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;

		if (color.r == fmax )
			hsl.x = deltaB - deltaG; // Hue
		else if (color.g == fmax)
			hsl.x = (1.0 / 3.0) + deltaR - deltaB; // Hue
		else if (color.b == fmax)
			hsl.x = (2.0 / 3.0) + deltaG - deltaR; // Hue

		if (hsl.x < 0.0)
			hsl.x += 1.0; // Hue
		else if (hsl.x > 1.0)
			hsl.x -= 1.0; // Hue
	}

	return hsl;
}

float HueToRGB(float f1, float f2, float hue)
{
	if (hue < 0.0)
		hue += 1.0;
	else if (hue > 1.0)
		hue -= 1.0;
	float res;
	if ((6.0 * hue) < 1.0)
		res = f1 + (f2 - f1) * 6.0 * hue;
	else if ((2.0 * hue) < 1.0)
		res = f2;
	else if ((3.0 * hue) < 2.0)
		res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
	else
		res = f1;
	return res;
}

vec3 HSLToRGB(vec3 hsl)
{
	vec3 rgb;
	
	if (hsl.y == 0.0)
		rgb = vec3(hsl.z); // Luminance
	else
	{
		float f2;
		
		if (hsl.z < 0.5)
			f2 = hsl.z * (1.0 + hsl.y);
		else
			f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);
			
		float f1 = 2.0 * hsl.z - f2;
		
		rgb.r = HueToRGB(f1, f2, hsl.x + (1.0/3.0));
		rgb.g = HueToRGB(f1, f2, hsl.x);
		rgb.b= HueToRGB(f1, f2, hsl.x - (1.0/3.0));
	}
	
	return rgb;
}


// For all settings: 1.0 = 100% 0.5=50% 1.5 = 150%
vec3 ContrastSaturationBrightness(vec3 color, float brt, float sat, float con)
{
	// Increase or decrease theese values to adjust r, g and b color channels seperately
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5;
	const float AvgLumB = 0.5;
	
	const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
	
	vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
	vec3 brtColor = color * brt;
	vec3 intensity = vec3(dot(brtColor, LumCoeff));
	vec3 satColor = mix(intensity, brtColor, sat);
	vec3 conColor = mix(AvgLumin, satColor, con);
	return conColor;
}



const float epsilon = 1e-6;

bool isEven(float n)
{
    float m = mod(floor(n), 2.0);
    if( abs(m) < 0.0001 )
        return true;

    return false;
}

bool isEqual( float a, float b )
{
    float v = abs( a - b );
    
    if( v < 0.0001 )
        return true;
    
    return false;
}

float Luminance( in vec4 Color );

float Luminance( in vec4 Color ) {
    if( Color.r > threshold )
        return Color.r;
    if( Color.g > threshold )
        return Color.g;
    if( Color.b > threshold )
        return Color.b; 

    return(((0.3333 * Color.r) + (0.333 * Color.g)) + (0.333 * Color.b));
}

void main()
{
    vec4 outColor = vec4( 1.0,0.0,0.0,1.0);

    vec4 color0 = texture2D( basetex, gl_TexCoord[0].xy ); 	
    vec4 color1 = texture2D( lighttex, gl_TexCoord[0].xy );

    outColor.xyz =  color0.xyz;
    vec3 hsv1 = RGBToHSL( color1.xyz );
    vec3 hsv0 = RGBToHSL( color0.xyz ); 
    
    if( hsv1.b >= threshold ){
        if( fakeColor.a == 0.0 )
            outColor.xyz = ContrastSaturationBrightness( color1.xyz, 1.0, saturation, 1.0 );
        else
            outColor.xyz = fakeColor.xyz;
    }else{
        outColor.xyz -= decayrate;
        outColor.xyz = clamp( outColor.xyz, 0.0, 1.0 );
    }

    gl_FragColor = outColor; 

}
