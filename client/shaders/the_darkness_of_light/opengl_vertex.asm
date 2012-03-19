!!ARBvp1.0

#input
ATTRIB InPos = vertex.position;
ATTRIB InColor = vertex.color;
ATTRIB InNormal = vertex.normal;
ATTRIB InTexCoord = vertex.texcoord;

#output
OUTPUT OutPos = result.position;
OUTPUT OutColor = result.color;
OUTPUT OutTexCoord = result.texcoord;

PARAM MVP[4] = { state.matrix.mvp }; # modelViewProjection matrix.
TEMP Temp;
TEMP TempColor;
TEMP TempCompare;

#transform position to clip space 
DP4 Temp.x, MVP[0], InPos;
DP4 Temp.y, MVP[1], InPos;
DP4 Temp.z, MVP[2], InPos;
DP4 Temp.w, MVP[3], InPos;

# check if normal.y > 0.5
SLT TempCompare, InNormal, {0.5,0.5,0.5,0.5};
MUL TempCompare.z, TempCompare.y, 0.5;
SUB TempCompare.x, 1.0, TempCompare.z;
MOV TempCompare.y, TempCompare.x;
MOV TempCompare.z, TempCompare.x;

# calculate light color
MUL OutColor, InColor, TempCompare;
MOV OutColor.w, 1.0;          # we want alpha to be always 1
MOV OutTexCoord, InTexCoord; # store texture coordinate
MOV OutPos, Temp;

END
