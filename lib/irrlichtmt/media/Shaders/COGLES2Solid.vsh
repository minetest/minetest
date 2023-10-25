/* Attributes */

attribute vec3 inVertexPosition;
attribute vec3 inVertexNormal;
attribute vec4 inVertexColor;
attribute vec2 inTexCoord0;

/* Uniforms */

uniform mat4 uWVPMatrix;
uniform mat4 uWVMatrix;
uniform mat4 uNMatrix;
uniform mat4 uTMatrix0;

uniform vec4 uGlobalAmbient;
uniform vec4 uMaterialAmbient;
uniform vec4 uMaterialDiffuse;
uniform vec4 uMaterialEmissive;
uniform vec4 uMaterialSpecular;
uniform float uMaterialShininess;

uniform float uThickness;

/* Varyings */

varying vec2 vTextureCoord0;
varying vec4 vVertexColor;
varying vec4 vSpecularColor;
varying float vFogCoord;

void main()
{
	gl_Position = uWVPMatrix * vec4(inVertexPosition, 1.0);
	gl_PointSize = uThickness;

	vec4 TextureCoord0 = vec4(inTexCoord0.x, inTexCoord0.y, 1.0, 1.0);
	vTextureCoord0 = vec4(uTMatrix0 * TextureCoord0).xy;

	vVertexColor = inVertexColor.bgra;
	vSpecularColor = vec4(0.0, 0.0, 0.0, 0.0);

	vec3 Position = (uWVMatrix * vec4(inVertexPosition, 1.0)).xyz;

	vFogCoord = length(Position);
}
