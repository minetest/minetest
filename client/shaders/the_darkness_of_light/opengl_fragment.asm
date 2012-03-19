!!ARBfp1.0

#Input
ATTRIB inTexCoord = fragment.texcoord;      # texture coordinates
ATTRIB inColor = fragment.color.primary; # interpolated diffuse color

#Output
OUTPUT outColor = result.color;

TEMP texelColor;
TXP texelColor, inTexCoord, texture, 2D; 
MUL texelColor, texelColor, inColor;  # multiply with color   
SUB outColor, {1.0,1.0,1.0,1.0}, texelColor;
MOV outColor.w, 1.0;

END

