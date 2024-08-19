#version 100

precision mediump float;

/* Varyings */
varying vec4 vVertexColor;

void main()
{
	gl_FragColor = vVertexColor;
}
