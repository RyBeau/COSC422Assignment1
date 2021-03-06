#version 400
layout (triangles) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat4 mvMatrix;
uniform mat4 norMatrix;
uniform mat4 mvpMatrix;
uniform vec4 lightPos;

out vec3 oPosition;
out vec2 TexCoord;
out vec4 lgtVec;
out vec4 normalEye;
out vec4 halfVec;

/*
    Calculates the normal vector for the triangle.
*/
vec4 calculateNormal(){
    vec3 vector1 = gl_in[0].gl_Position.xyz - gl_in[2].gl_Position.xyz;
    vec3 vector2 = gl_in[1].gl_Position.xyz - gl_in[2].gl_Position.xyz;

    return vec4(normalize(cross(vector1, vector2)), 0);
}

/*
    Calculates all the vectors needed for the lighting calculations in the fragment shader.
*/
void lightingCalculations(vec4 position){
    vec4 normal = calculateNormal();
    vec4 posnEye = mvMatrix * position;
    normalEye = norMatrix * normal;
    lgtVec = normalize(lightPos - posnEye);
    vec4 viewVec = normalize(vec4(-posnEye.xyz, 0));
    halfVec = normalize(lgtVec + viewVec);
}

/*
    Calls the support function for each vertex to calculate the lighting vectors. Also calculates the texture coordinates
    to be used in the fragment shader. Finally transforms the vertices into clip space.
*/
void main(){
    float xmin = -45, xmax = +45, zmin = 0, zmax = -100;

    for(int i = 0; i < gl_in.length(); i++)
    {
        lightingCalculations(gl_in[i].gl_Position);
        TexCoord.s = (gl_in[i].gl_Position.x -  xmin)/(xmax - xmin);
        TexCoord.t = (gl_in[i].gl_Position.z - zmin)/ (zmax - zmin);
        oPosition = gl_in[i].gl_Position.xyz;
        gl_Position = mvpMatrix * gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}