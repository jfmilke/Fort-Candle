
uniform sampler2DRect uTexImage;

in vec2 vTexCoord;

out vec4 fColor;
  
uniform bool horizontal_pass;

const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{

    vec2 tex_offset = 1.0 / textureSize(uTexImage); // gets size of single texel
    vec3 result = texture(uTexImage, gl_FragCoord.xy).rgb * weight[0]; // current fragment's contribution
    if(horizontal_pass)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(uTexImage, gl_FragCoord.xy + vec2(i, 0.0)).rgb * weight[i];
            result += texture(uTexImage, gl_FragCoord.xy - vec2(i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(uTexImage, gl_FragCoord.xy + vec2(0.0, i)).rgb * weight[i];
            result += texture(uTexImage, gl_FragCoord.xy - vec2(0.0, i)).rgb * weight[i];
        }
    }

    fColor = vec4(result, 1.0);
}

