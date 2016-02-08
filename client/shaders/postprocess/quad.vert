void main()
{
    gl_Position = gl_Vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    //gl_TexCoord[0].y = gl_TexCoord[0].y;
}

