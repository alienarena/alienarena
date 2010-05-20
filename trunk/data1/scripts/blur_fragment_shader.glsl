uniform vec2 ScaleU;
uniform sampler2D textureSource;
 
void main()
{
   vec4 sum = vec4(0.0);

   // take nine samples
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - 4.0*ScaleU.x, gl_TexCoord[0].y - 4.0*ScaleU.y)) * 0.05;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - 3.0*ScaleU.x, gl_TexCoord[0].y - 3.0*ScaleU.y)) * 0.09;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - 2.0*ScaleU.x, gl_TexCoord[0].y - 2.0*ScaleU.y)) * 0.12;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - ScaleU.x, gl_TexCoord[0].y - ScaleU.y)) * 0.15;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y)) * 0.16;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + ScaleU.x, gl_TexCoord[0].y + ScaleU.y)) * 0.15;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + 2.0*ScaleU.x, gl_TexCoord[0].y + 2.0*ScaleU.y)) * 0.12;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + 3.0*ScaleU.x, gl_TexCoord[0].y + 3.0*ScaleU.y)) * 0.09;
   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + 4.0*ScaleU.x, gl_TexCoord[0].y + 4.0*ScaleU.y)) * 0.05;
 
   gl_FragColor = sum;
}
