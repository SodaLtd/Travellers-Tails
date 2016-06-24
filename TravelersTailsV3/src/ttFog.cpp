#include "ttFog.h"

#define STRINGIFY(a) #a

ttFog::ttFog() {

}
ttFog::~ttFog() {

}
//
//
//
void ttFog::setup() {
	string vertex = STRINGIFY(
		
		varying vec3 n;
		varying vec3 v;
		varying vec3 view_space;

		void main(){
			v = (gl_ModelViewMatrix * gl_Vertex).xyz;       
			n = normalize(gl_NormalMatrix * gl_Normal).xyz;
			view_space =  (gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex).xyz;
			gl_TexCoord[0] = gl_MultiTexCoord0;
			gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
		} 
	);
	string fragment = STRINGIFY(

            uniform sampler2DRect texture;
			uniform vec3 eye_position;
			uniform vec3 light_position;
			uniform vec3 fog_colour;
			uniform float fog_density;
			uniform float fog_near;
			uniform float fog_far;
			uniform vec3 difuse_colour;
			uniform vec3 rim_colour;
			uniform int mode;
			uniform float time;

			varying vec3 n;
			varying vec3 v;
			varying vec3 view_space;

			vec3 maxSkyColor = vec3(0.2235294117647059, 0.4, 0.7568627450980392);
			//vec3 maxSkyColor = vec3(0.5058823529411764, 0.6549019607843137, 0.8392156862745098);
			vec3 minSkyColor = vec3(1., 1., 1.);
			/*
				noise function
			*/
			vec3 mod289(vec3 x) {
				return x - floor(x * (1.0 / 289.0)) * 289.0;
			}

			vec4 mod289(vec4 x) {
				return x - floor(x * (1.0 / 289.0)) * 289.0;
			}

			vec4 permute(vec4 x) {
					return mod289(((x*34.0)+1.0)*x);
			}

			vec4 taylorInvSqrt(vec4 r)
			{
				return 1.79284291400159 - 0.85373472095314 * r;
			}

			float snoise(vec3 v)
				{ 
				const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
				const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

			// First corner
				vec3 i  = floor(v + dot(v, C.yyy) );
				vec3 x0 =   v - i + dot(i, C.xxx) ;

			// Other corners
				vec3 g = step(x0.yzx, x0.xyz);
				vec3 l = 1.0 - g;
				vec3 i1 = min( g.xyz, l.zxy );
				vec3 i2 = max( g.xyz, l.zxy );

				//   x0 = x0 - 0.0 + 0.0 * C.xxx;
				//   x1 = x0 - i1  + 1.0 * C.xxx;
				//   x2 = x0 - i2  + 2.0 * C.xxx;
				//   x3 = x0 - 1.0 + 3.0 * C.xxx;
				vec3 x1 = x0 - i1 + C.xxx;
				vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
				vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

			// Permutations
				i = mod289(i); 
				vec4 p = permute( permute( permute( 
							i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
						+ i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
						+ i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

			// Gradients: 7x7 points over a square, mapped onto an octahedron.
			// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
				float n_ = 0.142857142857; // 1.0/7.0
				vec3  ns = n_ * D.wyz - D.xzx;

				vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

				vec4 x_ = floor(j * ns.z);
				vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

				vec4 x = x_ *ns.x + ns.yyyy;
				vec4 y = y_ *ns.x + ns.yyyy;
				vec4 h = 1.0 - abs(x) - abs(y);

				vec4 b0 = vec4( x.xy, y.xy );
				vec4 b1 = vec4( x.zw, y.zw );

				//vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
				//vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
				vec4 s0 = floor(b0)*2.0 + 1.0;
				vec4 s1 = floor(b1)*2.0 + 1.0;
				vec4 sh = -step(h, vec4(0.0));

				vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
				vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

				vec3 p0 = vec3(a0.xy,h.x);
				vec3 p1 = vec3(a0.zw,h.y);
				vec3 p2 = vec3(a1.xy,h.z);
				vec3 p3 = vec3(a1.zw,h.w);

			//Normalise gradients
				vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
				p0 *= norm.x;
				p1 *= norm.y;
				p2 *= norm.z;
				p3 *= norm.w;

			// Mix final noise value
				vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
				m = m * m;
				return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
											dot(p2,x2), dot(p3,x3) ) );
			}
            void main (void){                                         
                vec3 col;
				if ( mode == 1 ) {
					vec2 coord = gl_TexCoord[0].st;
					if ( coord.s > 0.5 ) coord.s = 1.0 - coord.s;
					vec3 noise = vec3(coord.s*1000.0,coord.t*1000.0,time);
					float noise_val = snoise(noise);
					//col = vec3(noise_val,noise_val,noise_val);
					// TODO: should mix between sea base colour and hilight colour ( probably white )
					//vec3 sea_colour = vec3(0.37*0.75,0.5*0.75,0.37*0.75); 
					vec3 sea_colour = vec3(0.0,(106.0/255.0)*0.75,(78.0/255.0)*0.75); 
					vec3 sea_hilite = vec3(1.0,1.0,1.0);
					//col = vec3(noise_val*0.37,noise_val*0.5,noise_val*0.37);
					col = mix( sea_colour, sea_hilite, noise_val );
				} else if ( mode == 2 ) {
					float height = clamp(view_space.y/1000.0,0.0,1.0); 
					col = mix(minSkyColor, maxSkyColor, height );	
				} else {
					col = texture2DRect(texture, gl_TexCoord[0].st).rgb;
				}
				/*
				// vector from point to light
				vec3 L = normalize( light_position - v);

				// vector from point to eye
				vec3 V = normalize( eye_position - v);

				//diffuse lighting
				vec3 diffuse = difuse_colour * max(dot(L,n), 0.0);

				//rim lighting
				float rim = 1. - max(dot(V, n), 0.0);
				rim = smoothstep(0.6, 1.0, rim);
				vec3 finalRim = rim_colour * vec3(rim, rim, rim);

				//get all lights and texture
				vec3 lightColor = finalRim + diffuse + col;
				*/
				if ( mode == 2 ) {
					gl_FragColor = vec4(col,1);
				} else { 
					vec3 lightColor = col;

					//distance
					float dist = length(view_space);
				
					float be = 0.025 * smoothstep(0.0, fog_near, /*eye_position.y*/ 2.0 - view_space.y);
					float bi = 0.075 * smoothstep(0.0, fog_far, /*eye_position.y*/ 2.0 - view_space.y); 
					float ext = exp(-dist * be);
					float insc = exp(-dist * bi);
 
					vec3 finalColor = lightColor * ext + fog_colour * (1 - insc);		

					gl_FragColor = vec4(finalColor,1);
				}
            }
	);
	setupShaderFromSource(GL_VERTEX_SHADER, vertex);
	setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
	linkProgram();
	//
	//
	//
	begin();
	setUniform3f("fog_colour",0.5,0.5,0.5);
	/*
	setUniform3f("difuse_colour",0.5,0.9,0.7);
	setUniform3f("rim_colour",0.5,0.6,0.7);
	*/
	setUniform3f("difuse_colour",1.0,0.54,0.0);
	setUniform3f("rim_colour",0.9,0.9,0.9);
	setUniform1f("fog_far",80.);
	//setUniform1f("fog_near",20.);
	setUniform1f("fog_near",40.);
	setUniform1f("fog_density",1.);
	setUniform1i("mode",0);
	end();
}
//
//
//
shared_ptr< ttFog > ttFog::s_shared = nullptr;
shared_ptr< ttFog > ttFog::shared() {
	if ( !s_shared ) {
		s_shared = shared_ptr< ttFog >( new ttFog );
		s_shared->setup();
	}
	return s_shared;
}
	

