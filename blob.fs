#version 430 core

layout(location = 0) out vec4 out_color;

layout(location = 0) uniform float iGlobalTime;
layout(location = 1) uniform vec2 iResolution;
layout(location = 2) uniform vec4 iMouse;
layout(location = 3) uniform vec3 iPos;
//------------------------------------------------------------------------------
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// A list of usefull distance function to simple primitives, and an example on how to
// do some interesting boolean operations, repetition and displacement.
//
// More info here: http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
//------------------------------------------------------------------------------
float sdPlane( vec3 p )
{
    return p.y+0.1*sin(iGlobalTime)*sin(10.0*sin(p.z)*cos(p.x));
	return p.y;
}
//------------------------------------------------------------------------------
float sdSphere( vec3 p, float s )
{
    return length(p)-s;
}
//------------------------------------------------------------------------------
float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) +
         length(max(d,0.0));
}
//------------------------------------------------------------------------------
float udRoundBox( vec3 p, vec3 b, float r )
{
  return length(max(abs(p)-b,0.0))-r;
}
//------------------------------------------------------------------------------
float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}
//------------------------------------------------------------------------------
float sdHexPrism( vec3 p, vec2 h )
{
    vec3 q = abs(p);
    return max(q.z-h.y,max(q.x+q.y*0.57735,q.y*1.1547)-h.x);
}
//------------------------------------------------------------------------------
float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
	vec3 pa = p - a;
	vec3 ba = b - a;
	float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );

	return length( pa - ba*h ) - r;
}
//------------------------------------------------------------------------------
float sdTriPrism( vec3 p, vec2 h )
{
    vec3 q = abs(p);
    return max(q.z-h.y,max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5);
}
//------------------------------------------------------------------------------
float sdCylinder( vec3 p, vec2 h )
{
  return max( length(p.xz)-h.x, abs(p.y)-h.y );
}
//------------------------------------------------------------------------------
float sdCone( in vec3 p, in vec3 c )
{
    vec2 q = vec2( length(p.xz), p.y );
	return max( max( dot(q,c.xy), p.y), -p.y-c.z );
}
//------------------------------------------------------------------------------
float length2( vec2 p )
{
	return sqrt( p.x*p.x + p.y*p.y );
}
//------------------------------------------------------------------------------
float length6( vec2 p )
{
	p = p*p*p; p = p*p;
	return pow( p.x + p.y, 1.0/6.0 );
}
//------------------------------------------------------------------------------
float length8( vec2 p )
{
	p = p*p; p = p*p; p = p*p;
	return pow( p.x + p.y, 1.0/8.0 );
}
//------------------------------------------------------------------------------
float sdTorus82( vec3 p, vec2 t )
{
  vec2 q = vec2(length2(p.xz)-t.x,p.y);
  return length8(q)-t.y;
}
//------------------------------------------------------------------------------
float sdTorus88( vec3 p, vec2 t )
{
  vec2 q = vec2(length8(p.xz)-t.x,p.y);
  return length8(q)-t.y;
}
//------------------------------------------------------------------------------
float sdCylinder6( vec3 p, vec2 h )
{
  return max( length6(p.xz)-h.x, abs(p.y)-h.y );
}
//------------------------------------------------------------------------------
vec2 smin( vec2 a, vec2 b )
{
    float k = 0.1;
    float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
    return vec2(mix( b.x, a.x, h ) - k*h*(1.0-h), mix( b.y, a.y, h ) - k*h*(1.0-h));
}
//------------------------------------------------------------------------------
float opS( float d1, float d2 )
{
    return max(-d2,d1);
}
//------------------------------------------------------------------------------
vec2 opU( vec2 d1, vec2 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}
//------------------------------------------------------------------------------
vec3 opRep( vec3 p, vec3 c )
{
    return mod(p,c)-0.5*c;
}
//------------------------------------------------------------------------------
vec3 opTwist( vec3 p )
{
    float  c = cos(10.0*p.y+10.0);
    float  s = sin(10.0*p.y+10.0);
    mat2   m = mat2(c,-s,s,c);
    return vec3(m*p.xz,p.y);
}
//------------------------------------------------------------------------------
vec2 opBlend(vec2 p1, vec2 p2)
{
    return smin(p1, p2);
}
//------------------------------------------------------------------------------
vec2 map(in vec3 pos)
{
    vec2 res = opU(
                    vec2(sdPlane(pos-vec3(0.0, -0.25, 0.0)), 1.0),
                        opBlend(
                            vec2(sdSphere(pos-vec3(0.0, 0.0, 0.0), 0.25 ), 86.6),
                            opBlend(
                                vec2(sdSphere(pos-vec3(0.0+0.3*cos(iGlobalTime), 0.0, 0.0+0.85*sin(iGlobalTime)), 0.25), 26.9),
                                vec2(sdSphere(pos-vec3(0.0, 0.0+0.85*cos(iGlobalTime), 0.0-0.3*sin(iGlobalTime)), 0.25), 66.9)
                                )
                            )
                        );
    return res;
}
//------------------------------------------------------------------------------
vec3 castRay( in vec3 ro, in vec3 rd, in float maxd )
{
	float precis = 0.001;
    float h=precis*2.0;
    float t = 0.0;
    float m = -1.0;
    int i;
    for(i=0; i<64; ++i)
    {
        if( abs(h)<precis||t>maxd ) break;
        t += h;
	    vec2 res = map( ro+rd*t );
        h = res.x;
	    m = res.y;
    }

    if( t>maxd ) m=-1.0;
    return vec3( t, m, i);
}
//------------------------------------------------------------------------------
float softshadow( in vec3 ro, in vec3 rd, in float mint, in float maxt, in float k )
{
	float res = 1.0;
    float dt = 0.02;
    float t = mint;
    for( int i=0; i<200; i++ )
    {
		if( t<maxt )
		{
        float h = map( ro + rd*t ).x;
        res = min( res, k*h/t );
        //t += max( 0.02, dt );
        t += 0.005;
		}
        else
        {
            break;
        }
    }
    return clamp( res, 0.0, 1.0 );
}

vec3 calcNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    map(pos+eps.xyy).x - map(pos-eps.xyy).x,
	    map(pos+eps.yxy).x - map(pos-eps.yxy).x,
	    map(pos+eps.yyx).x - map(pos-eps.yyx).x );
	return normalize(nor);
}
//------------------------------------------------------------------------------
float calcAO( in vec3 pos, in vec3 nor )
{
	float totao = 0.0;
    float sca = 1.0;
    for( int aoi=0; aoi<5; aoi++ )
    {
        float hr = 0.01 + 0.05*float(aoi);
        vec3 aopos =  nor * hr + pos;
        float dd = map( aopos ).x;
        totao += -(dd-hr)*sca;
        sca *= 0.75;
    }
    return clamp( 1.0 - 4.0*totao, 0.0, 1.0 );
}
//------------------------------------------------------------------------------
vec3 render( in vec3 ro, in vec3 rd )
{
    vec3 col = vec3(0.0);
    vec3 res = castRay(ro,rd,30.0);
    float t = res.x;
	float m = res.y;
    if( m>-0.5 )
    {
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal( pos );

		//col = vec3(0.6) + 0.4*sin( vec3(0.05,0.08,0.10)*(m-1.0) );
		col = vec3(0.6) + 0.4*sin( vec3(0.05,0.08,0.10)*(m-1.0) );

        float ao = calcAO( pos, nor );

        vec3 lig = normalize( vec3(-0.6, 0.7, -0.5) );
		//vec3 lig = normalize( vec3(1.0, 0.1, 0.0) );
		float amb = clamp( 0.5+0.5*nor.y, 0.0, 1.0 );
        float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
        float bac = clamp( dot( nor, normalize(vec3(-lig.x,0.0,-lig.z))), 0.0, 1.0 )*clamp( 1.0-pos.y,0.0,1.0);

		float sh = 1.0;
		if( dif>0.02 ) { sh = softshadow( pos, lig, 0.5, 100.0, 7.0 ); dif *= sh; }

		vec3 brdf = vec3(0.0);
		brdf += 0.20*amb*vec3(0.10,0.11,0.13)*ao;
        brdf += 0.20*bac*vec3(0.15,0.15,0.15)*ao;
        brdf += 1.20*dif*vec3(1.00,0.90,0.70);

		float pp = clamp( dot( reflect(rd,nor), lig ), 0.0, 1.0 );
		float spe = sh*pow(pp,16.0);
		float fre = ao*pow( clamp(1.0+dot(nor,rd),0.0,1.0), 2.0 );

		col = col*brdf + vec3(1.0)*col*spe + 0.2*fre*(0.5+0.5*col);

        col *= exp( -0.01*t*t );
	}
    else
    {
        //col = vec3(0.2, 0.5, 0.8);
    }

	//col *= exp( -0.01*t*t );

    //*
	return vec3( clamp(col,0.0,1.0) );
    /*/
    float val = res.z / 64.0;
    return vec3(val*3.0 - 0.33, val < 0.33 ? val*3.0 : (-val+1.0)*3.0, 1.0 - val*3.0);
    //*/
}
//------------------------------------------------------------------------------
void main( void )
{
	vec2 q = gl_FragCoord.xy/iResolution.xy;
    vec2 p = -1.0+2.0*q;
	p.x *= iResolution.x/iResolution.y;
    vec2 mo = iMouse.xy/iResolution.xy;

	float time = 15.0 + iGlobalTime;

	// camera
	vec3 ro = vec3( 3.2*cos(0.1 + 6.0*mo.x), 2.0*mo.y, 0.5 + 3.2*sin(0.1 + 6.0*mo.x) );
    vec3 ta = vec3( 0.0, 0.0, 0.0 );
	//vec3 ta = vec3( 0.0, -1.0, 0.0 );

	// camera tx
	vec3 cw = normalize( ta-ro );
	vec3 cp = vec3( 0.0, 1.0, 0.0 );
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
	vec3 rd = normalize( p.x*cu + p.y*cv + 2.5*cw );

    vec3 col = render( ro, rd );

	col = sqrt( col );

    // vigneting
    col *= 0.25+0.75*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.15 );

    out_color=vec4( col, 1.0 );
}
//------------------------------------------------------------------------------
